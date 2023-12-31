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
#include <AzCore/Math/Uuid.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/containers/unordered_set.h>
#include <AzCore/std/optional.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/std/string/string.h>
#include <AzToolsFramework/Prefab/PrefabIdTypes.h>

namespace AZ
{
    class Entity;
    class ReflectContext;
}

namespace AzToolsFramework
{
    class PrefabEditorEntityOwnershipService;

    namespace Prefab
    {
        class InstanceEntityMapperInterface;
        class TemplateInstanceMapperInterface;

        using InstanceAlias = AZStd::string;
        using EntityAlias = AZStd::string;

        class Instance;
        using EntityAliasOptionalReference = AZStd::optional<AZStd::reference_wrapper<EntityAlias>>;
        using InstanceOptionalReference = AZStd::optional<AZStd::reference_wrapper<Instance>>;
        using InstanceOptionalConstReference = AZStd::optional<AZStd::reference_wrapper<const Instance>>;
        using InstanceSet = AZStd::unordered_set<Instance*>;
        using InstanceSetConstReference = AZStd::optional<AZStd::reference_wrapper<const InstanceSet>>;

        // A prefab instance is the container for when a Prefab Template is Instantiated.
        class Instance
        {
        public:
            AZ_CLASS_ALLOCATOR(Instance, AZ::SystemAllocator, 0);
            AZ_RTTI(Instance, "{D4219332-A648-4285-9CA6-B7F095987CD3}");

            friend class AzToolsFramework::PrefabEditorEntityOwnershipService;

            using AliasToInstanceMap = AZStd::unordered_map<InstanceAlias, AZStd::unique_ptr<Instance>>;
            using AliasToEntityMap = AZStd::unordered_map<EntityAlias, AZStd::unique_ptr<AZ::Entity>>;
            using EntityList = AZStd::vector<AZ::Entity*>;

            Instance();
            virtual ~Instance();

            Instance(const Instance& rhs) = delete;
            Instance& operator=(const Instance& rhs) = delete;

            static void Reflect(AZ::ReflectContext* context);

            const TemplateId& GetTemplateId() const;
            void SetTemplateId(const TemplateId& templateId);

            const AZStd::string& GetTemplateSourcePath() const;
            void SetTemplateSourcePath(AZStd::string sourcePath);

            bool AddEntity(AZ::Entity& entity);
            AZStd::unique_ptr<AZ::Entity> DetachEntity(const AZ::EntityId& entityId);

            InstanceOptionalReference AddInstance(AZStd::unique_ptr<Instance> instance);
            AZStd::unique_ptr<Instance> DetachNestedInstance(const InstanceAlias& instanceAlias);

            /**
            * Gets the aliases for the entities in the Instance DOM.
            * 
            * @return The list of EntityAliases
            */
            AZStd::vector<EntityAlias> GetEntityAliases();

            /**
            * Gets the ids for the entities in the Instance DOM.  Can recursively trace all nested instances.
            */
            void GetNestedEntityIds(const AZStd::function<bool(const AZ::EntityId&)>& callback);

            void GetEntityIds(const AZStd::function<bool(const AZ::EntityId&)>& callback);

            /**
            * Gets the alias for a given EnitityId in the Instance DOM.
            *
            * @return entityAlias via optional
            */
            AZStd::optional<AZStd::reference_wrapper<EntityAlias>> GetEntityAlias(const AZ::EntityId& id);

            /**
            * Gets the id for a given EnitityAlias in the Instance DOM.
            *
            * @return entityId via optional
            */
            AZStd::optional<AZ::EntityId> GetEntityId(const EntityAlias& alias);


            /**
            * Gets the aliases of all the nested instances, which are sourced by the template with the given id.
            * 
            * @param templateId The source template id of the nested instances.
            * @return The list of aliases of the nested instances.
            */
            AZStd::vector<InstanceAlias> GetNestedInstanceAliases(TemplateId templateId) const;

            /**
            * Initializes all entities, including those in nested entities
            */
            void InitializeNestedEntities();
            void InitializeEntities();

            /**
            * Activates all entities, including those in nested entities
            */
            void ActivateNestedEntities();
            void ActivateEntities();

            InstanceOptionalReference FindNestedInstance(const InstanceAlias& nestedInstanceAlias);

            InstanceOptionalConstReference FindNestedInstance(const InstanceAlias& nestedInstanceAlias) const;

            void SetLinkId(LinkId linkId);

            LinkId GetLinkId() const;

            InstanceOptionalReference GetParentInstance();

            InstanceOptionalConstReference GetParentInstance() const;

            const InstanceAlias& GetInstanceAlias() const;

            bool IsParentInstance(const Instance& instance) const;

        protected:
            /**
            * Gets the entities owned by this instance
            */
            void GetEntities(EntityList& entities, bool includeNestedEntities = false);

        private:

            void ClearEntities();

            bool RegisterEntity(const AZ::EntityId& entityId, const EntityAlias& entityAlias);
            AZStd::unique_ptr<AZ::Entity> DetachEntity(const EntityAlias& entityAlias);

            static EntityAlias GenerateEntityAlias();
            static InstanceAlias GenerateInstanceAlias();

            // Provide access to private data members in the serializer
            friend class JsonInstanceSerializer;
            friend class InstanceEntityIdMapper;

            // A map of loose entities that the prefab instance directly owns.
            AliasToEntityMap m_entities;

            // A map of prefab instance pointers that this prefab instance owns.
            AliasToInstanceMap m_nestedInstances;

            
            // The id of the link that connects the template of this instance to it's source template.
            // This is not unique per instance. It's unique per link. It is invalid for instances that aren't nested under other instances.
            LinkId m_linkId = InvalidLinkId;

            // Maps to translate alias' to ids and back for serialization
            AZStd::unordered_map<EntityAlias, AZ::EntityId> m_templateToInstanceEntityIdMap;
            AZStd::unordered_map<AZ::EntityId, EntityAlias> m_instanceToTemplateEntityIdMap;

            // Alias this instance goes by under its parent if nested
            InstanceAlias m_alias;

            // The source path of the template this instance represents
            AZStd::string m_templateSourcePath;

            // The unique ID of the template this Instance belongs to.
            TemplateId m_templateId = InvalidTemplateId;

            // Pointer to the parent instance if nested
            Instance* m_parent = nullptr;

            // Interface for registering owned entities for external queries
            InstanceEntityMapperInterface* m_instanceEntityMapper = nullptr;

            // Interface for registering the Instance itself for external queries.
            TemplateInstanceMapperInterface* m_templateInstanceMapper = nullptr;
        };
    } // namespace Prefab
} // namespace AzToolsFramework
