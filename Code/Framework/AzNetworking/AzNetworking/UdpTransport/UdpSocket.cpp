/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#include <AzNetworking/Utilities/NetworkCommon.h>
#include <AzNetworking/UdpTransport/UdpSocket.h>
#include <AzNetworking/ConnectionLayer/IConnection.h>
#include <AzNetworking/Utilities/Endian.h>
#include <AzNetworking/Utilities/NetworkIncludes.h>
#include <AzCore/Console/IConsole.h>
#include <AzCore/Console/ILogger.h>
#include <AzCore/std/algorithm.h>

namespace AzNetworking
{
    AZ_CVAR(int32_t, net_UdpSendBufferSize, 1 * 1024 * 1024, nullptr, AZ::ConsoleFunctorFlags::Null, "Default UDP socket send buffer size");
    AZ_CVAR(int32_t, net_UdpRecvBufferSize, 1 * 1024 * 1024, nullptr, AZ::ConsoleFunctorFlags::Null, "Default UDP socket receive buffer size");
    AZ_CVAR(bool, net_UdpIgnoreWin10054, true, nullptr, AZ::ConsoleFunctorFlags::Null, "If true, will ignore 10054 socket errors on windows");

    UdpSocket::~UdpSocket()
    {
        Close();
    }

    bool UdpSocket::IsEncrypted() const
    {
        return false;
    }

    DtlsEndpoint::ConnectResult UdpSocket::ConnectDtlsEndpoint(DtlsEndpoint&, const IpAddress&, UdpPacketEncodingBuffer&) const
    {
        // No-op, no encryption wrapper required
        return DtlsEndpoint::ConnectResult::Complete;
    }

    DtlsEndpoint::ConnectResult UdpSocket::AcceptDtlsEndpoint(DtlsEndpoint&, const IpAddress&, const UdpPacketEncodingBuffer& dtlsData) const
    {
        if (dtlsData.GetSize() > 0)
        {
            AZLOG_WARN("Encryption is disabled on accepting endpoint, but connector provided a DTLS handshake blob.  Check that encryption is properly disabled on *BOTH* endpoints");
            return DtlsEndpoint::ConnectResult::Failed;
        }
        // No-op, no encryption wrapper required
        return DtlsEndpoint::ConnectResult::Complete;
    }

    bool UdpSocket::Open(uint16_t port, CanAcceptConnections, TrustZone)
    {
        AZ_Assert(!IsOpen(), "Open called on an active socket");

        if (IsOpen())
        {
            return false;
        }

        // Open the socket
        {
            m_socketFd = static_cast<SocketFd>(::socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP));

            if (!IsOpen())
            {
                const int32_t error = GetLastNetworkError();
                AZLOG_ERROR("Failed to create socket (%d:%s)", error, GetNetworkErrorDesc(error));
                m_socketFd = InvalidSocketFd;
                return false;
            }
        }

        // Handle binding
        {
            sockaddr_in hints;
            hints.sin_family = AF_INET;
            hints.sin_addr.s_addr = INADDR_ANY;
            hints.sin_port = htons(port);

            if (::bind(static_cast<int32_t>(m_socketFd), (const sockaddr *)&hints, sizeof(hints)) != 0)
            {
                const int32_t error = GetLastNetworkError();
                AZLOG_ERROR("Failed to bind socket to port %u (%d:%s)", uint32_t(port), error, GetNetworkErrorDesc(error));
                return false;
            }
        }

        if (!SetSocketBufferSizes(m_socketFd, net_UdpSendBufferSize, net_UdpRecvBufferSize))
        {
            return false;
        }

        if (!SetSocketNonBlocking(m_socketFd))
        {
            return false;
        }

        return true;
    }

    void UdpSocket::Close()
    {
        CloseSocket(m_socketFd);
        m_socketFd = InvalidSocketFd;
    }

#ifdef ENABLE_LATENCY_DEBUG
    //! Checks packets deferred by latency debug and sends any that have passed their latency threshold
    void UdpSocket::ProcessDeferredPackets()
    {
        const AZ::TimeMs currTimeMs = AZ::GetElapsedTimeMs();
        while (m_sendBuffer.size() > 0)
        {
            const DeferredData& sendData = m_sendBuffer.front();
            if (sendData.m_timeDeferredMs >= currTimeMs)
            {
                SendInternal(sendData.m_address, sendData.m_dataBuffer.GetBuffer(), sendData.m_dataBuffer.GetSize(), sendData.m_encrypt, *sendData.m_dtlsEndpoint);
                m_sendBuffer.pop_front();
            }
            else
            {
                // Send buffer is sorted on append so all subsequent elements should also have time remaining
                break;
            }
        }
    }
#endif

    int32_t UdpSocket::Send
    (
        const IpAddress& address,
        const uint8_t* data,
        uint32_t size,
        bool encrypt,
        DtlsEndpoint& dtlsEndpoint,
        [[maybe_unused]] const ConnectionQuality& connectionQuality
    ) const
    {
        AZ_Assert(size > 0, "Invalid data size for send");
        AZ_Assert(data != nullptr, "NULL data pointer passed to send");

        AZ_Assert(address.GetAddress(ByteOrder::Host) != 0, "Invalid address");
        AZ_Assert(address.GetPort(ByteOrder::Host) != 0, "Invalid address");

#ifdef ENABLE_LATENCY_DEBUG
        if ((connectionQuality.m_latencyMs > AZ::TimeMs{ 0 }) || (connectionQuality.m_varianceMs > AZ::TimeMs{ 0 }))
        {
            const AZ::TimeMs jitterMs = aznumeric_cast<AZ::TimeMs>(m_random.GetRandom()) % (connectionQuality.m_varianceMs / aznumeric_cast<AZ::TimeMs>(2));
            const AZ::TimeMs currTimeMs = AZ::GetElapsedTimeMs();
            const AZ::TimeMs deferTimeMs = (connectionQuality.m_latencyMs / aznumeric_cast<AZ::TimeMs>(2)) + jitterMs;
            m_sendBuffer.push_back(DeferredData(currTimeMs + deferTimeMs, address, data, size, encrypt, dtlsEndpoint));
            std::sort(m_sendBuffer.begin(), m_sendBuffer.end());
        }
#endif

        if (!IsOpen())
        {
            return 0;
        }

#ifdef ENABLE_LATENCY_DEBUG
        if (connectionQuality.m_lossPercentage > 0)
        {
            if (int32_t(m_random.GetRandom() % 100) < (connectionQuality.m_lossPercentage / 2))
            {
                // Pretend we sent, but don't actually send
                return true;
            }
        }
#endif

        int32_t sentBytes = size;

#ifdef ENABLE_LATENCY_DEBUG
        if (connectionQuality.m_latencyMs <= AZ::TimeMs{ 0 })
#endif
        {
            sentBytes = SendInternal(address, data, size, encrypt, dtlsEndpoint);

            if (sentBytes < 0)
            {
                const int32_t error = GetLastNetworkError();

                if (ErrorIsWouldBlock(error)) // Filter would block messages
                {
                    return SocketOpResultSuccess;
                }

                AZLOG_ERROR("Failed to write to socket (%d:%s)", error, GetNetworkErrorDesc(error));
            }
        }

        m_sentPackets++;
        m_sentBytes += sentBytes;

        return sentBytes;
    }

    int32_t UdpSocket::Receive(IpAddress& outAddress, uint8_t* outData, uint32_t size) const
    {
        AZ_Assert(size > 0, "Invalid data size for send");
        AZ_Assert(outData != nullptr, "NULL data pointer passed to send");

        if (!IsOpen())
        {
            return 0;
        }

        sockaddr_in from;
        socklen_t   fromLen = sizeof(from);

        const int32_t receivedBytes = recvfrom(static_cast<int32_t>(m_socketFd), reinterpret_cast<char*>(outData), static_cast<int32_t>(size), 0, (sockaddr*)&from, &fromLen);

        outAddress = IpAddress(ByteOrder::Network, from.sin_addr.s_addr, from.sin_port);

        if (receivedBytes < 0)
        {
            const int32_t error = GetLastNetworkError();

            if (ErrorIsWouldBlock(error)) // Filter would block messages
            {
                return 0;
            }

            bool ignoreForciblyClosedError = false;
            if (ErrorIsForciblyClosed(error, ignoreForciblyClosedError))
            {
                if (ignoreForciblyClosedError)
                {
                    return 0;
                }
                else
                {
                    return SocketOpResultError;
                }
            }

            AZLOG_ERROR("Failed to read from socket (%d:%s)", error, GetNetworkErrorDesc(error));
        }

        if (receivedBytes <= 0)
        {
            return 0;
        }

        m_recvPackets++;
        m_recvBytes += receivedBytes;
        return receivedBytes;
    }

    int32_t UdpSocket::SendInternal(const IpAddress& address, const uint8_t* data, uint32_t size,
        [[maybe_unused]] bool dontEncypt, [[maybe_unused]] DtlsEndpoint& dtlsEndpoint) const
    {
        sockaddr_in destAddr;
        memset(&destAddr, 0, sizeof(destAddr));
        destAddr.sin_family = AF_INET;
        destAddr.sin_addr.s_addr = address.GetAddress(ByteOrder::Network);
        destAddr.sin_port = address.GetPort(ByteOrder::Network);
        return sendto(static_cast<int32_t>(m_socketFd), reinterpret_cast<const char*>(data), size, 0, (sockaddr*)&destAddr, sizeof(destAddr));
    }
}
