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

#include <AzToolsFramework/Prefab/PrefabSystemComponent.h>

#include <AzCore/Component/Entity.h>
#include <AzCore/Serialization/Json/JsonSerialization.h>
#include <AzCore/Serialization/Json/RegistrationContext.h>
#include <AzToolsFramework/Prefab/Instance/InstanceEntityIdMapper.h>
#include <AzToolsFramework/Prefab/Instance/InstanceSerializer.h>
#include <AzToolsFramework/Prefab/PrefabDomUtils.h>

namespace AzToolsFramework
{
    namespace Prefab
    {
        void PrefabSystemComponent::Init()
        {
        }

        void PrefabSystemComponent::Activate()
        {
            AZ::Interface<PrefabSystemComponentInterface>::Register(this);
            m_prefabLoader.RegisterPrefabLoaderInterface();
            m_instanceUpdateExecutor.RegisterInstanceUpdateExecutorInterface();
            m_instanceToTemplatePropagator.RegisterInstanceToTemplateInterface();
        }

        void PrefabSystemComponent::Deactivate()
        {
            m_instanceToTemplatePropagator.UnregisterInstanceToTemplateInterface();
            m_instanceUpdateExecutor.UnregisterInstanceUpdateExecutorInterface();
            m_prefabLoader.UnregisterPrefabLoaderInterface();
            AZ::Interface<PrefabSystemComponentInterface>::Unregister(this);
        }

        void PrefabSystemComponent::Reflect(AZ::ReflectContext* context)
        {
            Instance::Reflect(context);

            AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
            if (serialize)
            {
                serialize->Class<PrefabSystemComponent, AZ::Component>()
                    ;
            }

            AZ::JsonRegistrationContext* jsonRegistration = azrtti_cast<AZ::JsonRegistrationContext*>(context);

            if (jsonRegistration)
            {
                jsonRegistration->Serializer<JsonInstanceSerializer>()->HandlesType<Instance>();
            }
        }

        void PrefabSystemComponent::GetProvidedServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
        }

        void PrefabSystemComponent::GetRequiredServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& required)
        {
        }

        void PrefabSystemComponent::GetIncompatibleServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& incompatible)
        {
        }

        AZStd::unique_ptr<Instance> PrefabSystemComponent::CreatePrefab(const AZStd::vector<AZ::Entity*>& entities, AZStd::vector<AZStd::unique_ptr<Instance>>&& instancesToConsume, const AZStd::string& filePath)
        {
            if (GetTemplateIdFromFilePath(filePath) != InvalidTemplateId)
            {
                AZ_Error("Prefab", false,
                    "Filepath %s has already been registered with the Prefab System Component",
                    filePath.c_str());

                return nullptr;
            }

            AZStd::unique_ptr<Instance> newInstance{ aznew Instance() };

            for (AZ::Entity* entity : entities)
            {
                AZ_Assert(entity, "Prefab - Null entity passed in during Create Prefab");

                newInstance->AddEntity(*entity);
            }

            for (AZStd::unique_ptr<Instance>& instance : instancesToConsume)
            {
                AZ_Assert(instance, "Prefab - Null instance passed in during Create Prefab");

                newInstance->AddInstance(AZStd::move(instance));
            }

            newInstance->SetTemplateSourcePath(filePath);

            TemplateId newTemplateId = CreateTemplateFromInstance(*newInstance);
            if (newTemplateId == InvalidTemplateId)
            {
                AZ_Error("Prefab", false,
                    "Failed to create a Template associated with file path %s during CreatePrefab.",
                    filePath.c_str());

                newInstance = nullptr;
            }
            else
            {
                newInstance->SetTemplateId(newTemplateId);
            }

            return newInstance;
        }

        void PrefabSystemComponent::PropagateTemplateChanges(TemplateId templateId)
        {
            UpdatePrefabInstances(templateId);
            auto templateIdToLinkIdsIterator = m_templateToLinkIdsMap.find(templateId);
            if (templateIdToLinkIdsIterator != m_templateToLinkIdsMap.end())
            {
                // We need to initialize a queue here because once all linked instances of a template are updated,
                // we will find all the linkIds corresponding to the updated template and add them to this queue again.
                AZStd::queue<LinkIds> linkIdsToUpdateQueue;
                linkIdsToUpdateQueue.push(LinkIds(templateIdToLinkIdsIterator->second.begin(),
                    templateIdToLinkIdsIterator->second.end()));
                UpdateLinkedInstances(linkIdsToUpdateQueue);
            }
        }

        void PrefabSystemComponent::UpdatePrefabTemplate(TemplateId templateId, const PrefabDom& updatedDom)
        {
            PrefabDom& templateDomToUpdate = FindTemplateDom(templateId);
            if (AZ::JsonSerialization::Compare(templateDomToUpdate, updatedDom) != AZ::JsonSerializerCompareResult::Equal)
            {
                templateDomToUpdate.CopyFrom(updatedDom, templateDomToUpdate.GetAllocator());
                PropagateTemplateChanges(templateId);
            }
        }

        void PrefabSystemComponent::UpdatePrefabInstances(const TemplateId& templateId)
        {
            m_instanceUpdateExecutor.AddTemplateInstancesToQueue(templateId);
            const bool updateResult = m_instanceUpdateExecutor.UpdateTemplateInstancesInQueue();
            AZ_Assert(updateResult,
                "Prefab - Error occurred while updating Instances of Template with id '%llu'.",
                templateId);
        }

        void PrefabSystemComponent::UpdateLinkedInstances(AZStd::queue<LinkIds>& linkIdsQueue)
        {
            while (!linkIdsQueue.empty())
            {
                // Fetch the list of linkIds at the head of the queue.
                LinkIds& LinkIdsToUpdate = linkIdsQueue.front();
                TargetTemplateIdToLinkIdMap targetTemplateIdToLinkIdMap;
                BucketLinkIdsByTargetTemplateId(LinkIdsToUpdate, targetTemplateIdToLinkIdMap);

                // Update all the linked instances corresponding to the LinkIds before fetching the next set of linkIds.
                // This will ensure that templates are updated with changes in the same order they are received.
                for (const LinkId& linkIdToUpdate : LinkIdsToUpdate)
                {
                    UpdateLinkedInstance(linkIdToUpdate, targetTemplateIdToLinkIdMap, linkIdsQueue);
                }
                linkIdsQueue.pop();
            }
        }

        void PrefabSystemComponent::BucketLinkIdsByTargetTemplateId(LinkIds& linkIdsToUpdate,
            TargetTemplateIdToLinkIdMap& targetTemplateIdToLinkIdMap)
        {
            for (const LinkId& linkIdToUpdate : linkIdsToUpdate)
            {
                Link& linkToUpdate = m_linkIdMap[linkIdToUpdate];
                TemplateId targetTemplateId = linkToUpdate.GetTargetTemplateId();
                auto templateIdToLinkIdsIterator = targetTemplateIdToLinkIdMap.find(targetTemplateId);
                if (templateIdToLinkIdsIterator == targetTemplateIdToLinkIdMap.end())
                {
                    targetTemplateIdToLinkIdMap.emplace(targetTemplateId, AZStd::make_pair(LinkIdSet{linkIdToUpdate}, false));
                }
                else
                {
                    targetTemplateIdToLinkIdMap[targetTemplateId].first.insert(linkIdToUpdate);
                }
            }
        }

        void PrefabSystemComponent::UpdateLinkedInstance(const LinkId linkIdToUpdate,
            TargetTemplateIdToLinkIdMap& targetTemplateIdToLinkIdMap, AZStd::queue<LinkIds>& linkIdsQueue)
        {
            Link& linkToUpdate = m_linkIdMap[linkIdToUpdate];
            TemplateId targetTemplateId = linkToUpdate.GetTargetTemplateId();
            PrefabDomValue& linkdedInstanceDom = linkToUpdate.GetLinkedInstanceDom();
            PrefabDomValue linkDomBeforeUpdate;
            linkDomBeforeUpdate.CopyFrom(linkdedInstanceDom, m_templateIdMap[targetTemplateId].GetPrefabDom().GetAllocator());
            linkToUpdate.UpdateTarget();

            // If any of the templates links are already updated, we don't need to check whether the linkedInstance DOM differs
            // in content because the template is already marked to be sent for change propagation.
            bool isTemplateUpdated = targetTemplateIdToLinkIdMap[targetTemplateId].second;
            if (isTemplateUpdated ||
                AZ::JsonSerialization::Compare(linkDomBeforeUpdate, linkdedInstanceDom) != AZ::JsonSerializerCompareResult::Equal)
            {
                targetTemplateIdToLinkIdMap[targetTemplateId].second = true;
            }

            if (targetTemplateIdToLinkIdMap.find(targetTemplateId) != targetTemplateIdToLinkIdMap.end())
            {
                targetTemplateIdToLinkIdMap[targetTemplateId].first.erase(linkIdToUpdate);
                UpdateTemplateChangePropagationQueue(targetTemplateIdToLinkIdMap, targetTemplateId, linkIdsQueue);
            }
        }

        void PrefabSystemComponent::UpdateTemplateChangePropagationQueue(
            TargetTemplateIdToLinkIdMap& targetTemplateIdToLinkIdMap,
            const TemplateId targetTemplateId, AZStd::queue<LinkIds>& linkIdsQueue)
        {
            if (targetTemplateIdToLinkIdMap[targetTemplateId].first.empty() &&
                targetTemplateIdToLinkIdMap[targetTemplateId].second)
            {
                UpdatePrefabInstances(targetTemplateId);

                auto templateToLinkIter = m_templateToLinkIdsMap.find(targetTemplateId);
                if (templateToLinkIter != m_templateToLinkIdsMap.end())
                {
                    linkIdsQueue.push(LinkIds(templateToLinkIter->second.begin(),
                        templateToLinkIter->second.end()));
                }
            }
        }

        AZStd::unique_ptr<Instance> PrefabSystemComponent::InstantiatePrefab(const TemplateId& templateId)
        {
            TemplateReference instantiatingTemplate = FindTemplate(templateId);

            if (!instantiatingTemplate.has_value())
            {
                AZ_Error("Prefab", false,
                    "Could not find template using Id %llu during InstantiatePrefab. Unable to proceed",
                    templateId);

                return nullptr;
            }

            Instance* newInstance = aznew Instance();
            if (!PrefabDomUtils::LoadInstanceFromPrefabDom(*newInstance, instantiatingTemplate->get().GetPrefabDom(), false))
            {
                AZ_Error("Prefab", false,
                    "Failed to Load Prefab Template associated with path %s. Instantiation Failed",
                    instantiatingTemplate->get().GetFilePath().c_str());

                delete newInstance;
                return nullptr;
            }

            return AZStd::unique_ptr<Instance>(newInstance);
        }

        TemplateId PrefabSystemComponent::CreateTemplateFromInstance(Instance& instance)
        {
            // We will register the template to match the path the instance has
            const AZStd::string& templateSourcePath = instance.GetTemplateSourcePath();
            if (templateSourcePath.empty())
            {
                AZ_Assert(false,
                    "PrefabSystemComponent::CreateTemplateFromInstance - "
                    "Attempted to create a prefab template from an instance without a source file path. "
                    "Unable to proceed.");

                return InvalidTemplateId;
            }

            // Convert our instance into a serialized template dom
            PrefabDom serializedInstance;
            if (!PrefabDomUtils::StoreInstanceInPrefabDom(instance, serializedInstance))
            {
                return InvalidTemplateId;
            }

            // Generate a new template and store the dom data
            const AZStd::string& instanceSourcePath = instance.GetTemplateSourcePath();
            TemplateId newTemplateId = AddTemplate(instanceSourcePath, AZStd::move(serializedInstance));
            if (newTemplateId == InvalidTemplateId)
            {
                AZ_Error("Prefab", false,
                    "PrefabSystemComponent::CreateTemplateFromInstance - "
                    "Failed to create a template from instance with source file path '%s': "
                    "invalid template id returned.",
                    instanceSourcePath.c_str());

                return InvalidTemplateId;
            }

            if (!GenerateLinksForNewTemplate(newTemplateId, instance))
            {
                // Clear new template and any links associated with it
                RemoveTemplate(newTemplateId);

                return InvalidTemplateId;
            }

            return newTemplateId;
        }

        TemplateReference PrefabSystemComponent::FindTemplate(const TemplateId& id)
        {
            auto found = m_templateIdMap.find(id);
            if (found != m_templateIdMap.end())
            {
                return found->second;
            }
            else
            {
                return AZStd::nullopt;
            }
        }

        PrefabDom& PrefabSystemComponent::FindTemplateDom(TemplateId templateId)
        {
            AZStd::optional<AZStd::reference_wrapper<Template>> findTemplateResult = FindTemplate(templateId);
            AZ_Assert(findTemplateResult.has_value(),
                "PrefabSystemComponent::FindTemplateDom - Unable to retrieve Prefab template with id: '%llu'. "
                "Template could not be found", templateId);

            AZ_Assert(findTemplateResult->get().IsValid(),
                "PrefabSystemComponent::FindTemplateDom - Unable to retrieve Prefab template with id: '%llu'. "
                "Template is invalid", templateId);

            return findTemplateResult->get().GetPrefabDom();
        }

        LinkReference PrefabSystemComponent::FindLink(const LinkId& id)
        {
            auto found = m_linkIdMap.find(id);
            if (found != m_linkIdMap.end())
            {
                return found->second;
            }
            else
            {
                return AZStd::nullopt;
            }
        }

        TemplateId PrefabSystemComponent::AddTemplate(const AZStd::string& filePath, PrefabDom prefabDom)
        {
            TemplateId newTemplateId = CreateUniqueTemplateId();
            Template& newTemplate = m_templateIdMap.emplace(
                AZStd::make_pair(newTemplateId, AZStd::move(Template(filePath, AZStd::move(prefabDom))))).first->second;

            if (!newTemplate.IsValid())
            {
                AZ_Assert(false,
                    "Prefab - PrefabSystemComponent::AddTemplate - "
                    "Can't add this new Template on file path '%s' since it is invalid.",
                    filePath.c_str());

                m_templateIdMap.erase(newTemplateId);
                return InvalidTemplateId;
            }

            if (!m_templateInstanceMapper.RegisterTemplate(newTemplateId))
            {
                m_templateIdMap.erase(newTemplateId);
                return InvalidTemplateId;
            }

            m_templateFilePathToIdMap.emplace(AZStd::make_pair(filePath, newTemplateId));
            
            return newTemplateId;
        }

        void PrefabSystemComponent::RemoveTemplate(const TemplateId& templateId)
        {
            auto findTemplateResult = FindTemplate(templateId);
            if (!findTemplateResult.has_value())
            {
                AZ_Warning("Prefab", false,
                    "PrefabSystemComponent::RemoveTemplate - "
                    "Template associated by given Id '%llu' doesn't exist in PrefabSystemComponent.",
                    templateId);

                return;
            }
            
            //Remove all Links owned by the Template from TemplateToLinkIdsMap.
            Template& templateToDelete = findTemplateResult->get();
            const Template::Links& linkIdsToDelete = templateToDelete.GetLinks();
            bool result;
            for (auto linkId : linkIdsToDelete)
            {
                result = RemoveLinkIdFromTemplateToLinkIdsMap(linkId);
                AZ_Assert(result,
                    "Prefab - PrefabSystemComponent::RemoveTemplate - "
                    "Failed to remove Link with Id '%llu' owned by the Template with Id '%llu' on file path '%s' "
                    "from TemplateToLinkIdsMap.",
                    linkId, templateId, templateToDelete.GetFilePath().c_str());
            }

            //Remove all Links that depend on this source Template from other target Templates.
            //Also remove this Template from TemplateToLinkIdsMap.
            auto templateToLinkIterator = m_templateToLinkIdsMap.find(templateId);
            if (templateToLinkIterator != m_templateToLinkIdsMap.end())
            {
                for (auto linkId : templateToLinkIterator->second)
                {
                    result = RemoveLinkIdFromTargetTemplate(linkId);
                    AZ_Assert(result,
                        "Prefab - PrefabSystemComponent::RemoveTemplate - "
                        "Failed to remove Link with Id '%llu' that depend on the source Template with Id '%llu' on file path '%s'.",
                        linkId, templateId, templateToDelete.GetFilePath().c_str());
                }

                result = m_templateToLinkIdsMap.erase(templateToLinkIterator) != 0;
                AZ_Assert(result,
                    "Prefab - PrefabSystemComponent::RemoveTemplate - "
                    "Failed to remove Template with Id '%llu' on file path '%s' "
                    "from TemplateToLinkIdsMap.",
                    templateId, templateToDelete.GetFilePath().c_str());
            }

            //Remove this Template from the rest of the maps.
            result = m_templateFilePathToIdMap.erase(templateToDelete.GetFilePath()) != 0;
            AZ_Assert(result,
                "Prefab - PrefabSystemComponent::RemoveTemplate - "
                "Failed to remove Template with Id '%llu' on file path '%s' "
                "from Template File Path To Id Map.",
                templateId, templateToDelete.GetFilePath().c_str());

            m_templateInstanceMapper.UnregisterTemplate(templateId);
            
            result = m_templateIdMap.erase(templateId) != 0;
            AZ_Assert(result,
                "Prefab - PrefabSystemComponent::RemoveTemplate - "
                "Failed to remove Template with Id '%llu' on file path '%s' "
                "from Template Id Map.",
                templateId, templateToDelete.GetFilePath().c_str());

            return;
        }

        LinkId PrefabSystemComponent::AddLink(
            const TemplateId& sourceTemplateId,
            const TemplateId& targetTemplateId,
            PrefabDomValue::MemberIterator& instanceIterator,
            InstanceOptionalReference instance)
        {
            TemplateReference sourceTemplateReference = FindTemplate(sourceTemplateId);
            TemplateReference targetTemplateReference = FindTemplate(targetTemplateId);
            if (!sourceTemplateReference.has_value() || !targetTemplateReference.has_value())
            {
                AZ_Error("Prefab", false,
                    "PrefabSystemComponent::AddLink - "
                    "Can't find both source Template '%u' and target Template '%u' in Prefab System Component.",
                    sourceTemplateId, targetTemplateId);
                return false;
            }

            Template& sourceTemplate = sourceTemplateReference->get();
            Template& targetTemplate = targetTemplateReference->get();

            AZStd::string_view instanceName(instanceIterator->name.GetString(), instanceIterator->name.GetStringLength());
            const AZStd::string& targetTemplateFilePath = targetTemplate.GetFilePath();
            const AZStd::string& sourceTemplateFilePath = sourceTemplate.GetFilePath();

            LinkId newLinkId = CreateUniqueLinkId();
            Link newLink(newLinkId);
            if (!ConnectTemplates(newLink, sourceTemplateId, targetTemplateId, instanceIterator))
            {
                AZ_Error("Prefab", false,
                    "PrefabSystemComponent::AddLink - "
                    "New Link to Nested Template Instance '%.*s' connecting source Template '%s' and target Template '%s' failed.",
                    aznumeric_cast<int>(instanceName.size()), instanceName.data(),
                    sourceTemplateFilePath.c_str(),
                    targetTemplateFilePath.c_str());

                return InvalidLinkId;
            }

            if (!newLink.IsValid())
            {
                AZ_Error("Prefab", false,
                    "PrefabSystemComponent::AddLink - "
                    "New Link to Nested Template Instance '%.*s' connecting source Template '%s' and target Template '%s' is invalid.",
                    aznumeric_cast<int>(instanceName.size()), instanceName.data(),
                    sourceTemplateFilePath.c_str(),
                    targetTemplateFilePath.c_str());

                return InvalidLinkId;
            }

            if (instance != AZStd::nullopt)
            {
                instance->get().SetLinkId(newLinkId);
            }

            m_linkIdMap.emplace(AZStd::make_pair(newLinkId, AZStd::move(newLink)));

            targetTemplate.AddLink(newLinkId);
            m_templateToLinkIdsMap[sourceTemplateId].emplace(newLinkId);

            return newLinkId;
        }

        void PrefabSystemComponent::RemoveLink(const LinkId& linkId)
        {
            auto findLinkResult = FindLink(linkId);
            if (!findLinkResult.has_value())
            {
                AZ_Warning("Prefab", false,
                    "PrefabSystemComponent::RemoveLink - "
                    "Link associated by given Id '%llu' doesn't exist in PrefabSystemComponent.",
                    linkId);

                return;
            }

            Link& link = findLinkResult->get();
            bool result;
            result = RemoveLinkIdFromTemplateToLinkIdsMap(linkId, link);
            AZ_Assert(result,
                "Prefab - PrefabSystemComponent::RemoveLink - "
                "Failed to remove Link with Id '%llu' for Instance '%s' of source Template with Id '%llu' "
                "from TemplateToLinkIdsMap.",
                linkId, link.GetSourceTemplateId(), link.GetInstanceName().c_str());

            result = RemoveLinkIdFromTargetTemplate(linkId, link);
            AZ_Assert(result,
                "Prefab - PrefabSystemComponent::RemoveLink - "
                "Failed to remove Link with Id '%llu' for Instance '%s' of source Template with Id '%llu' "
                "from target Template with Id '%llu'.",
                linkId, link.GetSourceTemplateId(), link.GetInstanceName().c_str(), link.GetTargetTemplateId());

            return;
        }

        TemplateId PrefabSystemComponent::GetTemplateIdFromFilePath(AZStd::string_view filePath) const
        {
            auto found = m_templateFilePathToIdMap.find(filePath);
            if (found != m_templateFilePathToIdMap.end())
            {
                return found->second;
            }
            else
            {
                return InvalidTemplateId;
            }
        }

        bool PrefabSystemComponent::ConnectTemplates(
            Link& link,
            TemplateId sourceTemplateId,
            TemplateId targetTemplateId,
            PrefabDomValue::MemberIterator& instanceIterator)
        {
            TemplateReference sourceTemplateReference = FindTemplate(sourceTemplateId);
            TemplateReference targetTemplateReference = FindTemplate(targetTemplateId);
            if (!sourceTemplateReference.has_value() || !targetTemplateReference.has_value())
            {
                AZ_Error("Prefab", false,
                    "PrefabSystemComponent::ConnectTemplates - "
                    "Can't find both source Template '%u' and target Template '%u' in Prefab System Component.",
                    sourceTemplateId, targetTemplateId);
                return false;
            }

            Template& sourceTemplate = sourceTemplateReference->get();
            Template& targetTemplate = targetTemplateReference->get();

            AZStd::string_view instanceName(instanceIterator->name.GetString(), instanceIterator->name.GetStringLength());
            const AZStd::string& targetTemplateFilePath = targetTemplate.GetFilePath();

            link.SetSourceTemplateId(sourceTemplateId);
            link.SetTargetTemplateId(targetTemplateId);
            link.SetInstanceName(instanceName.data());

            PrefabDomValue& instance = instanceIterator->value;

            PrefabDomValueReference patchesReference = PrefabDomUtils::FindPrefabDomValue(instance, PrefabDomUtils::PatchesName);
            if (!patchesReference.has_value())
            {
                PrefabDom& newLinkDom = link.GetLinkDom();

                newLinkDom.SetObject();

                newLinkDom.AddMember(rapidjson::StringRef(PrefabDomUtils::SourceName),
                    rapidjson::StringRef(sourceTemplate.GetFilePath().c_str()), newLinkDom.GetAllocator());
            }
            else
            {
                link.SetTemplatePatches(patchesReference->get());
            }

            if (!link.UpdateTarget())
            {
                AZ_Error("Prefab", false,
                    "PrefabSystemComponent::ConnectTemplates - "
                    "Failed to update the linked instance with source Prefab file '%s' and target Prefab file '%s'.",
                    sourceTemplate.GetFilePath().c_str(), targetTemplate.GetFilePath().c_str());
                return false;
            }

            link.AddLinkIdToInstanceDom(instance);
            return true;
        }

        bool PrefabSystemComponent::GenerateLinksForNewTemplate(const TemplateId& newTemplateId, Instance& instance)
        {
            TemplateReference newTemplateReference = FindTemplate(newTemplateId);
            if (!newTemplateReference.has_value())
            {
                return false;
            }

            Template& newTemplate = newTemplateReference->get();

            // Gather the Instances member from the template DOM
            PrefabDomValueReference instancesReference = newTemplate.GetInstancesValue();

            // No nested instances to perform link operations on
            if (instancesReference == AZStd::nullopt)
            {
                return true;
            }

            PrefabDomValue& instances = instancesReference->get();

            for (PrefabDomValue::MemberIterator instanceIterator = instances.MemberBegin(); instanceIterator != instances.MemberEnd(); ++instanceIterator)
            {
                // Acquire the source member of the nested template so we can get its template id
                // and join it with our new template via a link
                PrefabDomValueReference instanceSourceReference =
                    PrefabDomUtils::FindPrefabDomValue(instanceIterator->value, PrefabDomUtils::SourceName);

                if (!instanceSourceReference.has_value() || !instanceSourceReference->get().IsString())
                {
                    AZ_Error("Prefab", false,
                        "PrefabSystemComponent::GenerateLinksForNewTemplate - "
                        "Failed to acquire the source path value of a nested instance during creation of a new template associated with prefab %s"
                        "Unable to proceed",
                        newTemplate.GetFilePath().c_str());

                    return false;
                }

                const PrefabDomValue& source = instanceSourceReference->get();
                const TemplateId& nestedTemplateId = GetTemplateIdFromFilePath(source.GetString());
                if (nestedTemplateId == InvalidTemplateId)
                {
                    AZ_Error("Prefab", false,
                        "PrefabSystemComponent::GenerateLinksForNewTemplate - "
                        "Nested Template Instance with prefab path %s is not registered with the prefab system component"
                        "Unable to acquire its template id"
                        "Unable to complete creation of Template with prefab path %s",
                        newTemplate.GetFilePath().c_str());

                    return false;
                }

                // Construct and store the new link in our mapping.
                AZStd::string_view nestedTemplatePath(source.GetString(), source.GetStringLength());

                InstanceAlias instanceAlias = instanceIterator->name.GetString();
                LinkId newLinkId = AddLink(nestedTemplateId, newTemplateId, instanceIterator, instance.FindNestedInstance(instanceAlias));
                if (newLinkId == InvalidLinkId)
                {
                    AZ_Error("Prefab", false,
                        "PrefabSystemComponent::GenerateLinksForNewTemplate - "
                        "Failed to add a new Link to Nested Template Instance '%s' which connects source Template '%.*s' and target Template '%s'.",
                        instanceIterator->name.GetString(),
                        aznumeric_cast<int>(nestedTemplatePath.size()), nestedTemplatePath.data(),
                        newTemplate.GetFilePath().c_str());

                    return false;
                }
            }

            return true;
        }

        TemplateId PrefabSystemComponent::CreateUniqueTemplateId()
        {
            return m_templateIdCounter++;
        }

        LinkId PrefabSystemComponent::CreateUniqueLinkId()
        {
            return m_linkIdCounter++;
        }

        bool PrefabSystemComponent::RemoveLinkIdFromTemplateToLinkIdsMap(const LinkId& linkId)
        {
            auto findLinkResult = FindLink(linkId);
            if (!findLinkResult.has_value())
            {
                return false;
            }

            Link& link = findLinkResult->get();
            return RemoveLinkIdFromTemplateToLinkIdsMap(linkId, link);
        }

        bool PrefabSystemComponent::RemoveLinkIdFromTemplateToLinkIdsMap(const LinkId& linkId, const Link& link)
        {
            TemplateId sourceTemplateId = link.GetSourceTemplateId();
            auto templateToLinkIterator = m_templateToLinkIdsMap.find(sourceTemplateId);
            bool removed = false;
            if (templateToLinkIterator != m_templateToLinkIdsMap.end())
            {
                removed = templateToLinkIterator->second.erase(linkId) != 0;
                if (templateToLinkIterator->second.empty())
                {
                    m_templateToLinkIdsMap.erase(templateToLinkIterator);
                }
            }

            return removed;
        }

        bool PrefabSystemComponent::RemoveLinkIdFromTargetTemplate(const LinkId& linkId)
        {
            auto findLinkResult = FindLink(linkId);
            if (!findLinkResult.has_value())
            {
                return false;
            }

            Link& link = findLinkResult->get();
            return RemoveLinkIdFromTargetTemplate(linkId, link);
        }

        bool PrefabSystemComponent::RemoveLinkIdFromTargetTemplate(const LinkId& linkId, const Link& link)
        {
            TemplateId targetTemplateId = link.GetTargetTemplateId();

            auto templateIterator = m_templateIdMap.find(targetTemplateId);
            bool removed = false;
            if (templateIterator != m_templateIdMap.end())
            {
                removed = templateIterator->second.RemoveLink(linkId);
            }

            return removed;
        }

    } // namespace Prefab
} // namespace AzToolsFramework
