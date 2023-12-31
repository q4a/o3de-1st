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

#include <AzNetworking/UdpTransport/UdpPacketHeader.h>
#include <AzNetworking/UdpTransport/UdpPacketTracker.h>

namespace AzNetworking
{
    UdpPacketHeader::UdpPacketHeader()
        : m_packetType(PacketType{ 0 })
        , m_localSequence(InvalidSequenceId)
        , m_remoteSequence(InvalidSequenceId)
        , m_reliableSequence(InvalidSequenceId)
        , m_sequenceWindow(0)
        , m_localRolloverCount(InvalidSequenceRolloverCount)
    {
        ;
    }

    UdpPacketHeader::UdpPacketHeader(UdpPacketTracker& packetTracker, PacketType packetType, SequenceId reliableSequence)
        : m_packetType(packetType)
        , m_localSequence(InvalidSequenceId)
        , m_remoteSequence(packetTracker.GetLastReceivedSequenceId())
        , m_reliableSequence(reliableSequence)
        , m_sequenceWindow(packetTracker.GetSequencedAckHistory(m_sequenceWindow)) // m_sequenceWindow is being passed in uninitialized, okay here since it's just a uint32_t
        , m_localRolloverCount(InvalidSequenceRolloverCount)
    {
        const PacketId packetId = packetTracker.GetNextPacketId();
        m_localSequence = ToSequenceId(packetId);
        m_localRolloverCount = ToRolloverCount(packetId);
    }

    UdpPacketHeader::UdpPacketHeader(PacketType packetType, PacketId packetId)
        : m_packetType(packetType)
        , m_localSequence(InvalidSequenceId)
        , m_remoteSequence(InvalidSequenceId)
        , m_reliableSequence(InvalidSequenceId)
        , m_sequenceWindow(0)
        , m_localRolloverCount(InvalidSequenceRolloverCount)
    {
        m_localSequence = ToSequenceId(packetId);
        m_localRolloverCount = ToRolloverCount(packetId);
    }

    UdpPacketHeader::UdpPacketHeader
    (
        PacketType  packetType, 
        SequenceId  localSequence,
        SequenceId  remoteSequence,
        SequenceId  reliableSequence,
        BitsetChunk sequenceWindow,
        SequenceRolloverCount localRolloverCount
    )
        : m_packetType(packetType)
        , m_localSequence(localSequence)
        , m_remoteSequence(remoteSequence)
        , m_reliableSequence(reliableSequence)
        , m_sequenceWindow(sequenceWindow)
        , m_localRolloverCount(localRolloverCount)
    {
        ;
    }

    bool UdpPacketHeader::Serialize(ISerializer& serializer)
    {
        bool isReliable = GetIsReliable();

        serializer.Serialize(m_packetType, "PacketType");
        serializer.Serialize(m_localSequence, "LocalSequence");
        serializer.Serialize(m_remoteSequence, "RemoteSequence");
        serializer.Serialize(m_sequenceWindow, "SequenceWindow");
        serializer.Serialize(isReliable, "IsReliable");

        // If the packet is flagged as reliable, serialize the reliable sequence id
        if (isReliable)
        {
            serializer.Serialize(m_reliableSequence, "ReliableSequence");
        }

        return serializer.IsValid();
    }

    bool UdpPacketHeader::SerializePacketFlags(ISerializer& serializer)
    {
        return serializer.Serialize(m_packetFlags, "PacketFlags");
    }
}
