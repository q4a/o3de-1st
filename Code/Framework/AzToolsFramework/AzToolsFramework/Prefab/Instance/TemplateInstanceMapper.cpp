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

#include <AzToolsFramework/Prefab/Instance/TemplateInstanceMapper.h>

#include <AzCore/Interface/Interface.h>
#include <AzToolsFramework/Prefab/Instance/Instance.h>

namespace AzToolsFramework
{
    namespace Prefab
    {
        TemplateInstanceMapper::TemplateInstanceMapper()
        {
            AZ::Interface<TemplateInstanceMapperInterface>::Register(this);
        }

        TemplateInstanceMapper::~TemplateInstanceMapper()
        {
            AZ::Interface<TemplateInstanceMapperInterface>::Unregister(this);
        }


        bool TemplateInstanceMapper::RegisterTemplate(const TemplateId& templateId)
        {
            const bool result = m_templateIdToInstancesMap.emplace(templateId, InstanceSet()).second;
            AZ_Assert(result,
                "Prefab - PrefabSystemComponent::RegisterTemplate - "
                "Failed to register Template '%llu' to Template to Instances mapper. "
                "Template may never have been registered or was unregistered early.",
                templateId);

            return result;
        }

        bool TemplateInstanceMapper::UnregisterTemplate(const TemplateId& templateId)
        {
            const bool result = m_templateIdToInstancesMap.erase(templateId) != 0;
            AZ_Assert(result,
                "Prefab - PrefabSystemComponent::UnregisterTemplate - "
                "Failed to unregister Template '%llu' from Template to Instances mapper. "
                "This Template is likely already registered.",
                templateId);

            return result;
        }

        bool TemplateInstanceMapper::RegisterInstanceToTemplate(Instance& instance)
        {
            const TemplateId& templateId = instance.GetTemplateId();
            if (templateId == InvalidTemplateId)
            {
                return false; 
            }
            else
            {
                auto found = m_templateIdToInstancesMap.find(templateId);
                return found != m_templateIdToInstancesMap.end() &&
                    found->second.emplace(&instance).second;
            }
        }

        bool TemplateInstanceMapper::UnregisterInstance(Instance& instance)
        {
            auto found = m_templateIdToInstancesMap.find(instance.GetTemplateId());
            return found != m_templateIdToInstancesMap.end() &&
                found->second.erase(&instance) != 0;
        }

        InstanceSetConstReference TemplateInstanceMapper::FindInstancesOwnedByTemplate(const TemplateId& templateId) const
        {
            auto found = m_templateIdToInstancesMap.find(templateId);

            if (found != m_templateIdToInstancesMap.end())
            {
                return found->second;
            }
            else
            {
                return AZStd::nullopt;
            }
        }
    }
}
