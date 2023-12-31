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

#include <AzCore/Component/Component.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/std/containers/map.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzNetworking/Serialization/ISerializer.h>
#include <Source/NetworkEntity/EntityReplication/ReplicationRecord.h>
#include <Source/NetworkEntity/NetworkEntityHandle.h>
#include <Source/NetworkInput/IMultiplayerComponentInput.h>
#include <Source/MultiplayerTypes.h>

namespace Multiplayer
{
    class NetworkInput;
    class ReplicationRecord;
    class MultiplayerComponent;

    using EntityStopEvent = AZ::Event<const ConstNetworkEntityHandle&>;
    using EntityDirtiedEvent = AZ::Event<>;

    //! @class NetBindComponent
    //! @brief Component that provides net-binding to a networked entity.
    class NetBindComponent final
        : public AZ::Component
    {
    public:
        AZ_COMPONENT(NetBindComponent, "{DAA076B3-1A1C-4FEF-8583-1DF696971604}");

        static void Reflect(AZ::ReflectContext* context);
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);

        NetBindComponent();
        ~NetBindComponent() override = default;

        //! AZ::Component overrides.
        //! @{
        void Init() override;
        void Activate() override;
        void Deactivate() override;
        //! @}

        NetEntityRole GetNetEntityRole() const;
        bool IsAuthority() const;
        bool HasController() const;
        NetEntityId GetNetEntityId() const;
        const PrefabEntityId& GetPrefabEntityId() const;
        ConstNetworkEntityHandle GetEntityHandle() const;
        NetworkEntityHandle GetEntityHandle();

        MultiplayerComponentInputVector AllocateComponentInputs();
        bool IsProcessingInput() const;
        void CreateInput(NetworkInput& networkInput, float deltaTime);
        void ProcessInput(NetworkInput& networkInput, float deltaTime);
        float GetRewindDistanceForInput(const NetworkInput& networkInput, float deltaTime) const;

        bool HandleRpcMessage(NetEntityRole remoteRole, NetworkEntityRpcMessage& message);
        bool HandlePropertyChangeMessage(AzNetworking::ISerializer& serializer, bool notifyChanges = true);

        RpcSendEvent& GetSendServerAuthorityToClientSimulationRpcEvent();
        RpcSendEvent& GetSendServerAuthorityToClientAutonomousRpcEvent();
        RpcSendEvent& GetSendServerSimulationToServerAuthorityRpcEvent();
        RpcSendEvent& GetSendClientAutonomousToServerAuthorityRpcEvent();

        const ReplicationRecord& GetPredictableRecord() const;

        void MarkDirty();
        void NotifyLocalChanges();

        void AddEntityStopEvent(EntityStopEvent::Handler& eventHandle);
        void AddEntityDirtiedEvent(EntityDirtiedEvent::Handler& eventHandle);

        bool SerializeEntityCorrection(AzNetworking::ISerializer& a_Serializer);

        bool SerializeStateDeltaMessage(ReplicationRecord& replicationRecord, AzNetworking::ISerializer& serializer, ComponentSerializationType componentSerializationType);
        void NotifyStateDeltaChanges(ReplicationRecord& replicationRecord, ComponentSerializationType componentSerializationType);

        void FillReplicationRecord(ReplicationRecord& replicationRecord) const;
        void FillTotalReplicationRecord(ReplicationRecord& replicationRecord) const;

        bool IsMigrationDataValid() const;

    private:

        void SetMigrationDataValid(bool migrationDataValid);
        bool SerializeMigrationData(AzNetworking::ISerializer& serializer);

        void PreInit(AZ::Entity* entity, const PrefabEntityId& prefabEntityId, NetEntityId netEntityId, NetEntityRole netEntityRole);

        void ConstructControllers();
        void DestructControllers();
        void ActivateControllers(EntityIsMigrating entityIsMigrating);
        void DeactivateControllers(EntityIsMigrating entityIsMigrating);

        void OnEntityStateEvent(AZ::Entity::State oldState, AZ::Entity::State newState);

        void NetworkAttach();
        void NetworkDetach();

        void HandleMarkedDirty();
        void HandleLocalServerRpcMessage(NetworkEntityRpcMessage& message);

        void DetermineInputOrdering();

        void StopEntity();

        ReplicationRecord m_currentRecord = NetEntityRole::InvalidRole;
        ReplicationRecord m_totalRecord = NetEntityRole::InvalidRole;
        ReplicationRecord m_predictableRecord = NetEntityRole::InvalidRole;
        ReplicationRecord m_localNotificationRecord = NetEntityRole::InvalidRole;
        PrefabEntityId    m_prefabEntityId;
        AZStd::unordered_map<NetComponentId, MultiplayerComponent*> m_multiplayerComponentMap;
        AZStd::vector<MultiplayerComponent*> m_multiplayerSerializationComponentVector;
        AZStd::vector<MultiplayerComponent*> m_multiplayerInputComponentVector;

        RpcSendEvent m_sendServerAuthorityToClientSimulationRpcEvent;
        RpcSendEvent m_sendServerAuthorityToClientAutonomousRpcEvent;
        RpcSendEvent m_sendServerSimulationtoServerAuthorityRpcEvent;
        RpcSendEvent m_sendClientAutonomousToServerAuthorityRpcEvent;

        EntityStopEvent       m_entityStopEvent;
        EntityDirtiedEvent    m_dirtiedEvent;
        AZ::Event<>           m_onRemove;
        RpcSendEvent::Handler m_handleLocalServerRpcMessageEventHandle;
        AZ::Event<>::Handler  m_handleMarkedDirty;
        AZ::Event<>::Handler  m_handleNotifyChanges;
        AZ::Entity::EntityStateEvent::Handler m_handleEntityStateEvent;

        NetworkEntityHandle   m_netEntityHandle;
        NetEntityRole         m_netEntityRole   = NetEntityRole::InvalidRole;
        NetEntityId           m_netEntityId     = InvalidNetEntityId;

        bool                  m_isProcessingInput    = false;
        bool                  m_isMigrationDataValid = false;
        bool                  m_needsToBeStopped     = false;

        friend class NetworkEntityManager;
        friend class EntityReplicationManager;
    };

    bool NetworkRoleHasController(NetEntityRole networkRole);
}
