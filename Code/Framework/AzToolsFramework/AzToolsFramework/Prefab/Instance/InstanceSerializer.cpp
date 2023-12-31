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

#include <AzCore/Component/Entity.h>
#include <AzCore/Serialization/Json/RegistrationContext.h>
#include <AzToolsFramework/Prefab/Instance/Instance.h>
#include <AzToolsFramework/Prefab/Instance/InstanceSerializer.h>
#include <AzToolsFramework/Prefab/Instance/InstanceEntityIdMapper.h>
#include <AzToolsFramework/Prefab/PrefabSystemComponentInterface.h>

namespace AzToolsFramework
{
    namespace Prefab
    {
        AZ_CLASS_ALLOCATOR_IMPL(JsonInstanceSerializer, AZ::SystemAllocator, 0);

        AZ::JsonSerializationResult::Result JsonInstanceSerializer::Store(rapidjson::Value& outputValue, const void* inputValue, const void* defaultValue,
            [[maybe_unused]] const AZ::Uuid& valueTypeId, AZ::JsonSerializerContext& context)
        {
            namespace JSR = AZ::JsonSerializationResult;

            AZ_Assert(azrtti_typeid<Instance>() == valueTypeId, "Unable to Serialize Instance because the provided type is %s",
                valueTypeId.ToString<AZStd::string>().c_str());

            const Instance* instance = reinterpret_cast<const Instance*>(inputValue);
            AZ_Assert(instance, "Input value for JsonInstanceSerializer can't be null.");
            const Instance* defaultInstance = reinterpret_cast<const Instance*>(defaultValue);

            InstanceEntityIdMapper** idMapper = context.GetMetadata().Find<InstanceEntityIdMapper*>();

            if (idMapper && *idMapper)
            {
                (*idMapper)->SetStoringInstance(*instance);
            }

            JSR::ResultCode result(JSR::Tasks::WriteValue);
            {
                AZ::ScopedContextPath subPathSource(context, "m_templateSourcePath");
                const AZStd::string* sourcePath = &instance->GetTemplateSourcePath();
                const AZStd::string* defaultSourcePath = defaultInstance ? &defaultInstance->GetTemplateSourcePath() : nullptr;

                result = ContinueStoringToJsonObjectField(outputValue, "Source", sourcePath, defaultSourcePath, azrtti_typeid<AZStd::string>(), context);
            }

            {
                AZ::ScopedContextPath subPathEntities(context, "m_entities");
                const Instance::AliasToEntityMap* entities = &instance->m_entities;
                const Instance::AliasToEntityMap* defaultEntities = defaultInstance ? &defaultInstance->m_entities : nullptr;

                JSR::ResultCode resultEntities =
                    ContinueStoringToJsonObjectField(outputValue, "Entities",
                        entities, defaultEntities, azrtti_typeid<Instance::AliasToEntityMap>(), context);

                result.Combine(resultEntities);
            }

            {
                AZ::ScopedContextPath subPathInstances(context, "m_nestedInstances");
                const Instance::AliasToInstanceMap* instances = &instance->m_nestedInstances;
                const Instance::AliasToInstanceMap* defaultInstances = defaultInstance ? &defaultInstance->m_nestedInstances : nullptr;

                JSR::ResultCode resultInstances =
                    ContinueStoringToJsonObjectField(outputValue, "Instances",
                        instances, defaultInstances, azrtti_typeid<Instance::AliasToInstanceMap>(), context);

                result.Combine(resultInstances);
            }

            return context.Report(result,
                result.GetProcessing() == JSR::Processing::Completed ? "Successfully stored Instance information for Prefab." :
                "Failed to store Instance information for Prefab.");
        }

        AZ::JsonSerializationResult::Result JsonInstanceSerializer::Load(void* outputValue, [[maybe_unused]] const AZ::Uuid& outputValueTypeId,
            const rapidjson::Value& inputValue, AZ::JsonDeserializerContext& context)
        {
            namespace JSR = AZ::JsonSerializationResult;

            AZ_Assert(azrtti_typeid<Instance>() == outputValueTypeId,
                "Unable to deserialize Prefab Instance from json because the provided type is %s.",
                outputValueTypeId.ToString<AZStd::string>().c_str());

            Instance* instance = reinterpret_cast<Instance*>(outputValue);
            AZ_Assert(instance, "Output value for JsonInstanceSerializer can't be null");

            InstanceEntityIdMapper** idMapper = context.GetMetadata().Find<InstanceEntityIdMapper*>();

            if (idMapper && *idMapper)
            {
                (*idMapper)->SetLoadingInstance(*instance);
            }

            JSR::ResultCode result(JSR::Tasks::ReadField);
            {
                AZ::ScopedContextPath subPathSource(context, "Source");

                JSR::ResultCode sourceLoadResult =
                    ContinueLoadingFromJsonObjectField(&instance->m_templateSourcePath, azrtti_typeid<AZStd::string>(), inputValue, "Source", context);

                if (sourceLoadResult.GetOutcome() == JSR::Outcomes::Success)
                {
                    PrefabSystemComponentInterface* prefabSystemComponentInterface =
                        AZ::Interface<PrefabSystemComponentInterface>::Get();

                    AZ_Assert(prefabSystemComponentInterface,
                        "PrefabSystemComponentInterface could not be found. It is required to load Prefab Instances");

                    TemplateId templateId = prefabSystemComponentInterface->GetTemplateIdFromFilePath(instance->GetTemplateSourcePath());

                    instance->SetTemplateId(templateId);
                }

                result.Combine(sourceLoadResult);
            }

            {
                AZ::ScopedContextPath subPathInstances(context, "Instances");

                // An already filled instance should be cleared if inputValue's Instances member is empty
                // The Json serializer will not do this by default as it will not attempt to load a missing member
                if (!instance->m_nestedInstances.empty() && !inputValue.HasMember("Instances"))
                {
                    instance->m_nestedInstances.clear();
                }

                JSR::ResultCode instanceResult =
                    ContinueLoadingFromJsonObjectField(&instance->m_nestedInstances, azrtti_typeid<Instance::AliasToInstanceMap>(), inputValue, "Instances", context);

                if (instanceResult.GetProcessing() != JSR::Processing::Halted)
                {
                    for (auto& nestedInstance : instance->m_nestedInstances)
                    {
                        nestedInstance.second->m_parent = instance;
                        nestedInstance.second->m_alias = nestedInstance.first;
                    }
                }
                result.Combine(instanceResult);
            }

            {
                AZ::ScopedContextPath subPathEntities(context, "Entities");

                // An already filled instance should be cleared if inputValue's Entities member is empty
                // The Json serializer will not do this by default as it will not attempt to load a missing member
                instance->ClearEntities();

                result.Combine(ContinueLoadingFromJsonObjectField(&instance->m_entities, azrtti_typeid<Instance::AliasToEntityMap>(), inputValue, "Entities", context));
            }

            {
                AZ::ScopedContextPath subPathEntities(context, "LinkId");

                result.Combine(ContinueLoadingFromJsonObjectField(&instance->m_linkId, azrtti_typeid<LinkId>(), inputValue, "LinkId", context));
            }

            return context.Report(result,
                result.GetProcessing() == JSR::Processing::Completed ? "Succesfully loaded instance information for prefab." :
                "Failed to load instance information for prefab");
        }
    }
}
