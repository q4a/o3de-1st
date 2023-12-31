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

#include <AzNetworking/PacketLayer/IPacket.h>
#include <AzNetworking/PacketLayer/IPacketHeader.h>
#include <AzNetworking/Serialization/ISerializer.h>

namespace AzNetworking
{
    //! @class TcpPacketHeader
    //! @brief packet header class.
    class TcpPacketHeader final
        : public IPacketHeader
    {
    public:

        AZ_RTTI(TcpPacketHeader, "{6D92B9BE-C5E4-4571-B0FA-8F29042BE93B}", IPacketHeader);

        //! Construct with a packet type and size.
        //! @param packetType  type of packet
        //! @param packetSize size of the packet in bytes, not including header size
        TcpPacketHeader(PacketType packetType, uint16_t packetSize);

        virtual ~TcpPacketHeader() = default;

        //! IPacketHeader interface.
        // @{
        PacketType GetPacketType() const override;
        PacketId GetPacketId() const override;
        bool IsPacketFlagSet(PacketFlag flag) const override;
        void SetPacketFlag(PacketFlag flag, bool value) override;
        // @}

        //! Gets the size of the packet being received.
        //! @return size of the packet in bytes, not including header size
        uint16_t GetPacketSize() const;

        //! Base serialize method for all serializable structures or classes to implement.
        //! @param serializer ISerializer instance to use for serialization
        //! @return boolean true for success, false for serialization failure
        bool Serialize(ISerializer& serializer);

    private:

        PacketType m_packetType;
        uint16_t   m_packetSize;

        // TCP Packet Flags are serialized with the header as the entire header is never compressed
        PacketFlagBitset m_packetFlags;
    };
}

#include <AzNetworking/TcpTransport/TcpPacketHeader.inl>
