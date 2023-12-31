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

#include <AzCore/Interface/Interface.h>
#include <AzCore/Serialization/Json/JsonSerialization.h>
#include <AzCore/Component/Entity.h>

#include <AzToolsFramework/Prefab/Instance/Instance.h>
#include <AzToolsFramework/Prefab/Instance/InstanceEntityIdMapper.h>
#include <AzToolsFramework/Prefab/Instance/InstanceToTemplatePropagator.h>
#include <AzToolsFramework/Prefab/PrefabDomUtils.h>

namespace AzToolsFramework
{
    namespace Prefab
    {
        void InstanceToTemplatePropagator::RegisterInstanceToTemplateInterface()
        {
            AZ::Interface<InstanceToTemplateInterface>::Register(this);

            //get instance id associated with entityId
            m_instanceEntityMapperInterface = AZ::Interface<InstanceEntityMapperInterface>::Get();
            AZ_Assert(m_instanceEntityMapperInterface,
                "Prefab - InstanceToTemplateInterface - "
                "Instance Entity Mapper Interface could not be found. "
                "Check that it is being correctly initialized.");


            //use system component to grab template dom
            m_prefabSystemComponentInterface = AZ::Interface<PrefabSystemComponentInterface>::Get();
            AZ_Assert(m_prefabSystemComponentInterface,
                "Prefab - InstanceToTemplateInterface - "
                "Prefab System Component Interface could not be found. "
                "Check that it is being correctly initialized.");
        }

        void InstanceToTemplatePropagator::UnregisterInstanceToTemplateInterface()
        {
            AZ::Interface<InstanceToTemplateInterface>::Unregister(this);
        }

        bool InstanceToTemplatePropagator::GenerateDomForEntity(PrefabDom& generatedEntityDom, const AZ::Entity& entity)
        {
            //grab the owning instance so we can use the entityIdMapper in settings
            InstanceOptionalReference owningInstance = m_instanceEntityMapperInterface->FindOwningInstance(entity.GetId());

            if (!owningInstance)
            {
                AZ_Error("Prefab", false, "Entity does not belong to an instance");
                return false;
            }
            
            InstanceEntityIdMapper entityIdMapper;
            entityIdMapper.SetStoringInstance(owningInstance->get());

            //create settings so that the serialized entity dom undergoes mapping from entity id to entity alias
            AZ::JsonSerializerSettings settings;
            settings.m_metadata.Add(static_cast<AZ::JsonEntityIdSerializer::JsonEntityIdMapper*>(&entityIdMapper));

            //generate PrefabDom using Json serialization system
            AZ::JsonSerializationResult::ResultCode result = AZ::JsonSerialization::Store(
                generatedEntityDom, generatedEntityDom.GetAllocator(), entity, settings);

            return result.GetOutcome() == AZ::JsonSerializationResult::Outcomes::Success;
        }

        bool InstanceToTemplatePropagator::GenerateDomForInstance(PrefabDom& generatedInstanceDom, const Prefab::Instance& instance)
        {
            return PrefabDomUtils::StoreInstanceInPrefabDom(instance, generatedInstanceDom);
        }

        bool InstanceToTemplatePropagator::GeneratePatch(PrefabDom& generatedPatch, const PrefabDom& initialState,
            const PrefabDom& modifiedState)
        {
            //generate patch using jsonserialization CreatePatch
            AZ::JsonSerializationResult::ResultCode result = AZ::JsonSerialization::CreatePatch(generatedPatch,
                generatedPatch.GetAllocator(), initialState, modifiedState, AZ::JsonMergeApproach::JsonPatch);

            return result.GetProcessing() != AZ::JsonSerializationResult::Processing::Halted;
        }

        bool InstanceToTemplatePropagator::GeneratePatchForLink(PrefabDom& generatedPatch, const PrefabDom& initialState,
            const PrefabDom& modifiedState, LinkId linkId)
        {
            AZStd::optional<AZStd::reference_wrapper<Link>> findLinkResult = m_prefabSystemComponentInterface->FindLink(linkId);

            if (!findLinkResult.has_value())
            {
                AZ_Assert(false, "Link with id %llu couldn't be found. Patch cannot be generated.", linkId);
                return false;
            }
            
            Link& link = findLinkResult->get();

            AZ::JsonSerializationResult::ResultCode result = AZ::JsonSerialization::CreatePatch(generatedPatch,
                link.GetLinkDom().GetAllocator(), initialState, modifiedState, AZ::JsonMergeApproach::JsonPatch);

            return result.GetProcessing() != AZ::JsonSerializationResult::Processing::Halted;
        }

        void InstanceToTemplatePropagator::PatchEntityInTemplate(PrefabDomValue& providedPatch, const AZ::EntityId& entityId)
        {
            InstanceOptionalReference instanceOptionalReference = m_instanceEntityMapperInterface->FindOwningInstance(entityId);
            AZ_Error("Prefab", instanceOptionalReference,
                "Failed to find an owning instance for the entity with id %llu.", static_cast<AZ::u64>(entityId));

            //get template space associated with instance
            Instance& instance = instanceOptionalReference->get();
            TemplateId templateId = instance.GetTemplateId();

            //alias entity goes by in template -> get via owning instance map
            AZStd::optional<EntityAlias> entityAlias = instance.GetEntityAlias(entityId);
            AZ_Error("Prefab", entityAlias != AZStd::nullopt,
                "Failed to find an entity alias for the provided entity");

            PrefabDom& templateDomReference = m_prefabSystemComponentInterface->FindTemplateDom(templateId);

            PrefabDom templateDom;
            templateDom.CopyFrom(templateDomReference, templateDom.GetAllocator());

            //query into the template dom for the alias
            PrefabDomValueReference entityList = PrefabDomUtils::FindPrefabDomValue(templateDom, "Entities");

            PrefabDomValueReference entity = PrefabDomUtils::FindPrefabDomValue(entityList->get(), entityAlias->c_str());
            AZ_Error("Prefab", entity != AZStd::nullopt, "Failed to aquire entity value reference")

            //apply patch to section
            AZ::JsonSerializationResult::ResultCode result = AZ::JsonSerialization::ApplyPatch(entity->get(),
                templateDom.GetAllocator(), providedPatch, AZ::JsonMergeApproach::JsonPatch);

            AZ_Error("Prefab", result.GetOutcome() == AZ::JsonSerializationResult::Outcomes::Success,
                "Patch was not successfully applied")

            //update the Dom and trigger propogation
            m_prefabSystemComponentInterface->UpdatePrefabTemplate(templateId, templateDom);
        }

        void InstanceToTemplatePropagator::PatchTemplate(PrefabDomValue& providedPatch, const TemplateId& templateId)
        {
            const PrefabDom& templateDomReference = m_prefabSystemComponentInterface->FindTemplateDom(templateId);

            PrefabDom templateDom;
            templateDom.CopyFrom(templateDomReference, templateDom.GetAllocator());

            //apply patch to template
            AZ::JsonSerializationResult::ResultCode result = AZ::JsonSerialization::ApplyPatch(templateDom,
                templateDom.GetAllocator(), providedPatch, AZ::JsonMergeApproach::JsonPatch);

            AZ_Error("Prefab", result.GetOutcome() == AZ::JsonSerializationResult::Outcomes::Success,
                "Patch was not successfully applied")

            //update the Dom and trigger propogation
            if (result.GetOutcome() == AZ::JsonSerializationResult::Outcomes::Success)
            {
                m_prefabSystemComponentInterface->UpdatePrefabTemplate(templateId, templateDom);
            }
        }

        void InstanceToTemplatePropagator::ApplyPatchesToInstance(const AZ::EntityId& entityId, PrefabDom& patches,
            const Instance& instanceToAddPatches)
        {
            InstanceOptionalReference owningInstance = m_instanceEntityMapperInterface->FindOwningInstance(entityId);
            if (!owningInstance)
            {
                AZ_Error("Prefab", false, "Entity does not belong to an instance.");
                return;
            }

            EntityAliasOptionalReference entityAliasOptionalReference = owningInstance->get().GetEntityAlias(entityId);
            if (!entityAliasOptionalReference)
            {
                AZ_Error("Prefab", false, "Entity alias for entity with id %llu cannot be found.", static_cast<AZ::u64>(entityId));
                return;
            }

            // We need to generate a patch prefix so that the path of the patches correctly refelects the hierarchy path
            // from the entity to the instance where the patch needs to be added.
            AZStd::string patchPrefix;
            patchPrefix.insert(0, "/Entities/" + entityAliasOptionalReference->get());
            while (!owningInstance->get().IsParentInstance(instanceToAddPatches))
            {
                patchPrefix.insert(0, "/Instances/" + owningInstance->get().GetInstanceAlias());
                owningInstance = owningInstance->get().GetParentInstance();
                if (!owningInstance.has_value())
                {
                    AZ_Error("Prefab", false, "The Entity couldn't be patched because the instance "
                        "to which the patches must be applied couldn't be found in the hierarchy of the entity");
                    return;
                }
            }
            LinkId linkIdToAddPatches = owningInstance->get().GetLinkId();

            auto findLinkResult = m_prefabSystemComponentInterface->FindLink(linkIdToAddPatches);
            if (!findLinkResult.has_value())
            {
                AZ_Error("Prefab", false,
                    "A valid link corresponding to the instance couldn't be found. Patches won't be applied.");
                return;
            }
            Link& linkToApplyPatches = findLinkResult->get();

            for (auto patchIterator = patches.Begin(); patchIterator != patches.End(); ++patchIterator)
            {
                PrefabDomValueReference patchPathReference = PrefabDomUtils::FindPrefabDomValue(*patchIterator, "path");
                AZStd::string patchPathString = patchPathReference->get().GetString();
                patchPathString.insert(0, patchPrefix);
                patchPathReference->get().SetString(patchPathString.c_str(), linkToApplyPatches.GetLinkDom().GetAllocator());
            }

            AddPatchesToLink(patches, linkToApplyPatches);
            linkToApplyPatches.UpdateTarget();
            m_prefabSystemComponentInterface->PropagateTemplateChanges(linkToApplyPatches.GetTargetTemplateId());
        }

        InstanceOptionalReference InstanceToTemplatePropagator::GetTopMostInstanceInHierarchy(AZ::EntityId entityId)
        {
            InstanceOptionalReference parentInstance = m_instanceEntityMapperInterface->FindOwningInstance(entityId);

            if (!parentInstance)
            {
                AZ_Error("Prefab", false, "Entity does not belong to an instance");
                return AZStd::nullopt;
            }

            while (parentInstance->get().GetParentInstance())
            {
                parentInstance = parentInstance->get().GetParentInstance();
            }

            return parentInstance;
        }

        void InstanceToTemplatePropagator::AddPatchesToLink(PrefabDom& patches, Link& link)
        {
            PrefabDom& linkDom = link.GetLinkDom();
            PrefabDomValueReference linkPatchesReference =
                PrefabDomUtils::FindPrefabDomValue(linkDom, PrefabDomUtils::PatchesName);
            PrefabDom& templateDomReference = m_prefabSystemComponentInterface->FindTemplateDom(link.GetTargetTemplateId());

            // This logic only covers addition of patches. If patches already exists, the given list of patches must be appended to them.
            if (!linkPatchesReference.has_value())
            {
                linkDom.AddMember(rapidjson::StringRef(PrefabDomUtils::PatchesName), patches, linkDom.GetAllocator());
            }
        }
    }
}
