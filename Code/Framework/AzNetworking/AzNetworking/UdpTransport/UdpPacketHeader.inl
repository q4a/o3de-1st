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

namespace AzNetworking
{
    inline PacketType UdpPacketHeader::GetPacketType() const
    {
        return m_packetType;
    }

    inline PacketId UdpPacketHeader::GetPacketId() const
    {
        AZ_Assert(m_localRolloverCount != InvalidSequenceRolloverCount, "UdpPacketHeader: header was not initialized properly, PacketId is invalid");
        return MakePacketId(m_localRolloverCount, m_localSequence);
    }

    inline bool UdpPacketHeader::IsPacketFlagSet(PacketFlag flag) const
    {
        return m_packetFlags.GetBit(aznumeric_cast<uint32_t>(flag));
    }

    inline void UdpPacketHeader::SetPacketFlag(PacketFlag flag, bool value)
    {
        m_packetFlags.SetBit(aznumeric_cast<uint32_t>(flag), value);
    }

    inline void UdpPacketHeader::SetPacketFlags(PacketFlagBitset flags)
    {
        m_packetFlags = flags;
    }

    inline bool UdpPacketHeader::GetIsReliable() const
    {
        return (m_reliableSequence != InvalidSequenceId);
    }

    inline SequenceId UdpPacketHeader::GetLocalSequenceId() const
    {
        return m_localSequence;
    }

    inline SequenceId UdpPacketHeader::GetRemoteSequenceId() const
    {
        return m_remoteSequence;
    }

    inline SequenceId UdpPacketHeader::GetReliableSequenceId() const
    {
        return m_reliableSequence;
    }

    inline BitsetChunk UdpPacketHeader::GetSequenceWindow() const
    {
        return m_sequenceWindow;
    }

    inline SequenceRolloverCount UdpPacketHeader::GetSequenceRolloverCount() const
    {
        return m_localRolloverCount;
    }

    inline void UdpPacketHeader::SetLocalRolloverCount(SequenceRolloverCount rolloverCount)
    {
        m_localRolloverCount = rolloverCount;
    }
}
