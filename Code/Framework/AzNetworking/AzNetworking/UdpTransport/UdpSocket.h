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

#pragma once

#include <AzNetworking/Utilities/IpAddress.h>
#include <AzNetworking/Utilities/NetworkCommon.h>
#include <AzNetworking/ConnectionLayer/IConnection.h>
#include <AzNetworking/UdpTransport/DtlsEndpoint.h>
#include <AzCore/Math/Random.h>
#include <AzCore/std/containers/fixed_vector.h>

#ifndef _RELEASE
#   define ENABLE_LATENCY_DEBUG 1
#endif

namespace AzNetworking
{
    // Forwards
    struct ConnectionQuality;

    //! @class UdpSocket
    //! @brief wrapper class for managing UDP sockets.
    class UdpSocket
    {
    public:

        enum class CanAcceptConnections
        {
            False, // Socket will not able to accept incoming connections, removes any requirement for RSA materials to open an SSL context (no private key file)
            True   // Socket can accept incoming connections and may require a valid certificate and private key file
        };

        UdpSocket() = default;
        virtual ~UdpSocket();

        //! Returns true if this is an encrypted socket, false if not.
        //! @return boolean true if this is an encrypted socket, false if not
        virtual bool IsEncrypted() const;

        //! Creates an encryption socket wrapper.
        //! @param dtlsEndpoint the encryption wrapper instance to create a connection over
        //! @param address      the IP address of the endpoint to connect to
        //! @param outDtlsData  data buffer to store the dtls handshake packet
        //! @return a connect result specifying whether the connection is still pending, failed, or complete
        virtual DtlsEndpoint::ConnectResult ConnectDtlsEndpoint(DtlsEndpoint& dtlsEndpoint, const IpAddress& address, UdpPacketEncodingBuffer& outDtlsData) const;

        //! Accepts an encryption socket wrapper.
        //! @param dtlsEndpoint the encryption wrapper instance to create a connection over
        //! @param address      the IP address of the endpoint to connect to
        //! @param dtlsData     data buffer containing the dtls handshake packet
        //! @return a connect result specifying whether the connection is still pending, failed, or complete
        virtual DtlsEndpoint::ConnectResult AcceptDtlsEndpoint(DtlsEndpoint& dtlsEndpoint, const IpAddress& address, const UdpPacketEncodingBuffer& dtlsData) const;

        //! Opens the UDP socket on the given port.
        //! @param port      the port number to open the UDP socket on, 0 will bind to any available port
        //! @param canAccept if true, the socket will be opened in a way that allows accepting incoming connections
        //! @param trustZone for encrypted connections, the level of trust we associate with this connection (internal or external)
        //! @return boolean true on success
        virtual bool Open(uint16_t port, CanAcceptConnections canAccept, TrustZone trustZone);

        //! Closes an open socket.
        virtual void Close();

#ifdef ENABLE_LATENCY_DEBUG
        //! Checks packets deferred by latency debug and sends any that have passed their latency threshold
        virtual void ProcessDeferredPackets();
#endif

        //! Returns true if the UDP socket is currently in an open state.
        //! @return boolean true if the socket is in a connected state
        bool IsOpen() const;

        //! Sends a single payload over the UDP socket to the connected endpoint.
        //! @param address           the address to send the payload to
        //! @param data              pointer to the data to send
        //! @param size              size of the payload in bytes
        //! @param encrypt           signals that the payload should be encrypted before transmitting if encryption is supported
        //! @param dtlsEndpoint      data required for DTLS encryption
        //! @param connectionQuality debug connection quality parameters
        //! @return number of bytes sent, <= 0 on error
        int32_t Send(const IpAddress& address, const uint8_t* data, uint32_t size, bool encrypt, DtlsEndpoint& dtlsEndpoint, const ConnectionQuality& connectionQuality) const;

        //! Receives a payload from the UDP socket.
        //! @param outAddress on success, the address of the endpoint that sent the data
        //! @param outData    on success, address to write the received data to
        //! @param size      maximum size the output buffer supports for receiving
        //! @return number of bytes received, <= 0 on error
        int32_t Receive(IpAddress& outAddress, uint8_t* outData, uint32_t size) const;

        //! Returns the underlying socket file descriptor.
        //! @return the underlying socket file descriptor
        SocketFd GetSocketFd() const;

        //! Returns the total number of packets sent on this socket.
        //! @return the total number of packets sent on this socket
        uint32_t GetSentPackets() const;

        //! Returns the total number of bytes received on this socket.
        //! @return the total number of bytes received on this socket
        uint32_t GetSentBytes() const;

        //! Returns the total number of packets sent on this socket.
        //! @return the total number of packets sent on this socket
        uint32_t GetRecvPackets() const;

        //! Returns the total number of bytes received on this socket.
        //! @return the total number of bytes received on this socket
        uint32_t GetRecvBytes() const;

    protected:

        virtual int32_t SendInternal(const IpAddress& address, const uint8_t* data, uint32_t size, bool encrypt, DtlsEndpoint& dtlsEndpoint) const;

    private:

        SocketFd m_socketFd = InvalidSocketFd;
        mutable uint32_t m_sentPackets = 0;
        mutable uint32_t m_sentBytes = 0;
        mutable uint32_t m_recvPackets = 0;
        mutable uint32_t m_recvBytes = 0;

#ifdef ENABLE_LATENCY_DEBUG
        struct DeferredData
        {
            DeferredData(AZ::TimeMs timeDeferredMs, const IpAddress& address, const uint8_t* data, uint32_t size, bool encrypt, DtlsEndpoint& dtlsEndpoint)
                : m_timeDeferredMs(timeDeferredMs)
                , m_address(address)
                , m_encrypt(encrypt)
                , m_dtlsEndpoint(&dtlsEndpoint)
            {
                m_dataBuffer.CopyValues(data, size);
            }

            bool operator <(const DeferredData& rhs) const 
            {
                return m_timeDeferredMs < rhs.m_timeDeferredMs;
            }

            AZ::TimeMs m_timeDeferredMs;
            IpAddress m_address;
            bool m_encrypt;
            DtlsEndpoint* m_dtlsEndpoint;
            UdpPacketEncodingBuffer m_dataBuffer;
        };

        mutable AZStd::deque<DeferredData> m_sendBuffer;
        mutable AZStd::deque<DeferredData> m_recvBuffer;
        mutable AZ::SimpleLcgRandom m_random;
#endif
    };
}

#include <AzNetworking/UdpTransport/UdpSocket.inl>
