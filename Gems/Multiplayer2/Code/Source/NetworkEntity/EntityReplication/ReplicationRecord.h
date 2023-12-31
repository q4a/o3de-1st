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

#include <AzNetworking/DataStructures/FixedSizeVectorBitset.h>
#include <AzNetworking/Serialization/ISerializer.h>
#include <AzNetworking/Utilities/NetworkCommon.h>
#include <Source/MultiplayerTypes.h>

namespace Multiplayer
{
    struct ReplicationRecordStats
    {
        ReplicationRecordStats() = default;
        ReplicationRecordStats
        (
            uint32_t authorityToClientCount,
            uint32_t authorityToServerCount,
            uint32_t authorityToAutonomousCount,
            uint32_t autonomousToAuthorityCount
        );

        uint32_t m_authorityToClientCount = 0;
        uint32_t m_authorityToServerCount = 0;
        uint32_t m_authorityToAutonomousCount = 0;
        uint32_t m_autonomousToAuthorityCount = 0;

        bool operator ==(const ReplicationRecordStats& rhs) const;
        ReplicationRecordStats operator-(const ReplicationRecordStats& rhs) const;
    };

    class ReplicationRecord
    {
    public:
        static constexpr uint32_t MaxRecordBits = 2048;

        ReplicationRecord() = default;
        ReplicationRecord(NetEntityRole netEntityRole);

        void SetNetworkRole(NetEntityRole netEntityRole);
        NetEntityRole GetNetworkRole() const;

        bool AreAllBitsConsumed() const;
        void ResetConsumedBits();

        void Clear();

        void Append(const ReplicationRecord &rhs);
        void Subtract(const ReplicationRecord &rhs);
        bool HasChanges() const;

        bool Serialize(AzNetworking::ISerializer& serializer);

        void ConsumeAuthorityToClientBits(uint32_t consumedBits);
        void ConsumeAuthorityToServerBits(uint32_t consumedBits);
        void ConsumeAuthorityToAutonomousBits(uint32_t consumedBits);
        void ConsumeAutonomousToAuthorityBits(uint32_t consumedBits);

        bool ContainsAuthorityToClientBits() const;
        bool ContainsAuthorityToServerBits() const;
        bool ContainsAuthorityToAutonomousBits() const;
        bool ContainsAutonomousToAuthorityBits() const;

        uint32_t GetRemainingAuthorityToClientBits() const;
        uint32_t GetRemainingAuthorityToServerBits() const;
        uint32_t GetRemainingAuthorityToAutonomousBits() const;
        uint32_t GetRemainingAutonomousToAuthorityBits() const;

        ReplicationRecordStats GetStats() const;

        using RecordBitset = AzNetworking::FixedSizeVectorBitset<MaxRecordBits>;
        RecordBitset m_authorityToClientRecord;
        RecordBitset m_authorityToServerRecord;
        RecordBitset m_authorityToAutonomousRecord;
        RecordBitset m_autonomousToAuthorityRecord;

        uint32_t m_authorityToClientConsumedBits = 0;
        uint32_t m_authorityToServerConsumedBits = 0;
        uint32_t m_authorityToAutonomousConsumedBits = 0;
        uint32_t m_autonomousToAuthorityConsumedBits = 0;

        // Sequence number this ReplicationRecord was sent on
        AzNetworking::PacketId m_sentPacketId = AzNetworking::InvalidPacketId;

        NetEntityRole m_netEntityRole = NetEntityRole::InvalidRole;;
    };
}
