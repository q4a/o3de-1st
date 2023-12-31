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

#include <AzCore/Component/EntityId.h>
#include <AzCore/Interface/Interface.h>
#include <AzCore/Math/Aabb.h>

#include <AzFramework/Entity/EntityContextBus.h>
#include <AzFramework/Render/GeometryIntersectionStructures.h>
#include <AzFramework/Render/IntersectorInterface.h>
#include <AzFramework/Render/GeometryIntersectionBus.h>


namespace AzFramework
{

    namespace RenderGeometry
    {
        //! Implementation of IntersectorBus interface, this class contains a cached AABB list
        //! of the connected entities to the intersection bus and calculates performant ray intersections against them
        class Intersector final
            : public IntersectorBus::Handler
            , protected IntersectionNotificationBus::Handler
        {
        public:
            Intersector(AzFramework::EntityContextId contextId);
            ~Intersector();

            // IntersectorBus
            RayResult RayIntersect(const RayRequest& ray) override;

            // IntersectionNotifications
            void OnEntityConnected(AZ::EntityId entityId) override;
            void OnEntityDisconnected(AZ::EntityId entityId) override;
            void OnGeometryChanged(AZ::EntityId entityId) override;

        private:

            struct EntityData
            {
                EntityData(AZ::EntityId id, AZ::Aabb aabb)
                    : m_id(id)
                    , m_aabb(aabb)
                    , m_ref(1) {}

                AZ::EntityId m_id;
                AZ::Aabb m_aabb;
                int m_ref;
            };
            
            class EntityDataList
            {
            public:

                int AddRef(AZ::EntityId entityId);
                int RemoveRef(AZ::EntityId entityId);
                void Update(const EntityData& newData);
                Intersector::EntityData* FindEntityData(AZ::EntityId entityId);

                auto begin()
                {
                    return m_entitiesData.begin();
                }

                auto end()
                {
                    return m_entitiesData.end();
                }

            private:
                AZStd::vector<Intersector::EntityData> m_entitiesData;
                AZStd::unordered_map<AZ::EntityId, int> m_entityIdIndexLookup;
            };

            RayResult CalculateRayAgainstEntityDataList(const RayRequest& ray, EntityDataList& entityDataList);
            void UpdateDirtyEntities();

            EntityDataList m_registeredEntities;
            AZStd::unordered_set<AZ::EntityId> m_dirtyEntities;
            AZStd::vector<AZStd::pair<const EntityData*, float>> m_candidatesSortedByDist;
            AzFramework::EntityContextId m_contextId;
        };
    }
}
