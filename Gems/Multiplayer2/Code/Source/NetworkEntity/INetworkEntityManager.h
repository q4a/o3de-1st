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

#include <Source/MultiplayerTypes.h>
#include <Source/NetworkEntity/NetworkEntityHandle.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/EBus/Event.h>

namespace Multiplayer
{
    class NetworkEntityTracker;
    class NetworkEntityAuthorityTracker;
    class NetworkEntityRpcMessage;

    using EntityExitDomainEvent = AZ::Event<const ConstNetworkEntityHandle&>;
    using ControllersActivatedEvent = AZ::Event<const ConstNetworkEntityHandle&, EntityIsMigrating>;
    using ControllersDeactivatedEvent = AZ::Event<const ConstNetworkEntityHandle&, EntityIsMigrating>;

    //! @class INetworkEntityManager
    //! @brief The interface for managing all networked entities.
    class INetworkEntityManager
    {
    public:
        AZ_RTTI(INetworkEntityManager, "{109759DE-9492-439C-A0B1-AE46E6FD029C}");

        using OwnedEntitySet = AZStd::unordered_set<ConstNetworkEntityHandle>;

        virtual ~INetworkEntityManager() = default;

        //! Returns the NetworkEntityTracker for this INetworkEntityManager instance.
        //! @return the NetworkEntityTracker for this INetworkEntityManager instance
        virtual NetworkEntityTracker* GetNetworkEntityTracker() = 0;

        //! Returns the NetworkEntityAuthorityTracker for this INetworkEntityManager instance.
        //! @return the NetworkEntityAuthorityTracker for this INetworkEntityManager instance
        virtual NetworkEntityAuthorityTracker* GetNetworkEntityAuthorityTracker() = 0;

        //! Returns the HostId for this INetworkEntityManager instance.
        //! @return the HostId for this INetworkEntityManager instance
        virtual HostId GetHostId() const = 0;

        // TODO: Spawn methods for entities within slices/prefabs/levels

        //! Returns an ConstEntityPtr for the provided entityId.
        //! @param netEntityId the netEntityId to get an ConstEntityPtr for
        //! @return the requested ConstEntityPtr
        virtual ConstNetworkEntityHandle GetEntity(NetEntityId netEntityId) const = 0;

        //! Returns the total number of entities tracked by this INetworkEntityManager instance.
        //! @return the total number of entities tracked by this INetworkEntityManager instance
        virtual uint32_t GetEntityCount() const = 0;

        //! Adds the provided entity to the internal entity map identified by the provided netEntityId.
        //! @param netEntityId the identifier to use for the added entity
        //! @param entity      the entity to add to the internal entity map
        //! @return a NetworkEntityHandle for the newly added entity
        virtual NetworkEntityHandle AddEntityToEntityMap(NetEntityId netEntityId, AZ::Entity* entity) = 0;

        //! Marks the specified entity for removal and deletion.
        //! @param entityHandle the entity to remove and delete
        virtual void MarkForRemoval(const ConstNetworkEntityHandle& entityHandle) = 0;

        //! Returns true if the indicated entity is marked for removal.
        //! @param entityHandle the entity to test if marked for removal
        //! @return boolean true if the specified entity is marked for removal, false otherwise
        virtual bool IsMarkedForRemoval(const ConstNetworkEntityHandle& entityHandle) const = 0;

        //! Unmarks the specified entity so it will no longer be removed and deleted.
        //! @param entityHandle the entity to unmark for removal and deletion
        virtual void ClearEntityFromRemovalList(const ConstNetworkEntityHandle& entityHandle) = 0;

        //! Clears out and deletes all entities registered with the entity manager.
        virtual void ClearAllEntities() = 0;

        //! Adds an event handler to be invoked when we notify which entities have been marked dirty.
        //! @param entityMarkedDirtyHandle event handler for the dirtied entity
        virtual void AddEntityMarkedDirtyHandler(AZ::Event<>::Handler& entityMarkedDirtyHandle) = 0;

        //! Adds an event handler to be invoked when we notify entities to send their change notifications.
        //! @param entityNotifyChangesHandle event handler for the dirtied entity
        virtual void AddEntityNotifyChangesHandler(AZ::Event<>::Handler& entityNotifyChangesHandle) = 0;

        //! Adds an event handler to be invoked when we notify entities to send their change notifications.
        //! @param entityNotifyChangesHandle event handler for the dirtied entity
        virtual void AddEntityExitDomainHandler(EntityExitDomainEvent::Handler& entityExitDomainHandler) = 0;

        //! Adds an event handler to be invoked when an entities controllers have activated
        //! @param controllersActivatedHandler event handler for the entity
        virtual void AddControllersActivatedHandler(ControllersActivatedEvent::Handler& controllersActivatedHandler) = 0;

        //! Adds an event handler to be invoked when an entities controllers have been deactivated
        //! @param controllersDeactivatedHandler event handler for the entity
        virtual void AddControllersDeactivatedHandler(ControllersDeactivatedEvent::Handler& controllersDeactivatedHandler) = 0;

        //! Notifies entities that they should process their dirty state.
        virtual void NotifyEntitiesDirtied() = 0;

        //! Notifies entities that they should process change notifications.
        virtual void NotifyEntitiesChanged() = 0;

        //! Notifies that an entities controllers have activated.
        //! @param entityHandle handle to the entity whose controllers have activated
        //! @param entityIsMigrating true if the entity is activating after a migration
        virtual void NotifyControllersActivated(const ConstNetworkEntityHandle& entityHandle, EntityIsMigrating entityIsMigrating) = 0;

        //! Notifies that an entities controllers have been deactivated.
        //! @param entityHandle handle to the entity whose controllers have been deactivated
        //! @param entityIsMigrating true if the entity is deactivating due to a migration
        virtual void NotifyControllersDeactivated(const ConstNetworkEntityHandle& entityHandle, EntityIsMigrating entityIsMigrating) = 0;

        //! Handle a local rpc message.
        //! @param entityRpcMessage the local rpc message to handle
        virtual void HandleLocalRpcMessage(NetworkEntityRpcMessage& message) = 0;
    };

    // Convenience helpers
    inline INetworkEntityManager* GetNetworkEntityManager()
    {
        return AZ::Interface<INetworkEntityManager>::Get();
    }

    inline NetworkEntityTracker* GetNetworkEntityTracker()
    {
        return GetNetworkEntityManager()->GetNetworkEntityTracker();
    }

    inline NetworkEntityAuthorityTracker* GetNetworkEntityAuthorityTracker()
    {
        return GetNetworkEntityManager()->GetNetworkEntityAuthorityTracker();
    }
}
