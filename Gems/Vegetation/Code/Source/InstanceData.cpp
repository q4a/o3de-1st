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
#include <Vegetation/InstanceData.h>

namespace Vegetation
{
    void InstanceData::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->RegisterGenericType<AZStd::vector<InstanceData>>();

            serialize->Class<InstanceData>()
                ->Version(1)
                ->Field("Id", &InstanceData::m_id)
                ->Field("InstanceId", &InstanceData::m_instanceId)
                ->Field("Position", &InstanceData::m_position)
                ->Field("Normal", &InstanceData::m_normal)
                ->Field("Rotation", &InstanceData::m_rotation)
                ->Field("Alignment", &InstanceData::m_alignment)
                ->Field("Scale", &InstanceData::m_scale)
                ->Field("Descriptor", &InstanceData::m_descriptorPtr)
                ;
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<InstanceData>()
                ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::Preview)
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                ->Attribute(AZ::Script::Attributes::Category, "Vegetation")
                ->Attribute(AZ::Script::Attributes::Module, "vegetation")
                ->Constructor()
                ->Property("id", BehaviorValueProperty(&InstanceData::m_id))
                ->Property("instanceId", BehaviorValueProperty(&InstanceData::m_instanceId))
                ->Property("position", BehaviorValueProperty(&InstanceData::m_position))
                ->Property("normal", BehaviorValueProperty(&InstanceData::m_normal))
                ->Property("rotation", BehaviorValueProperty(&InstanceData::m_rotation))
                ->Property("alignment", BehaviorValueProperty(&InstanceData::m_alignment))
                ->Property("scale", BehaviorValueProperty(&InstanceData::m_scale))
                // Return a bare pointer instead of a smart pointer for easier use from scripting languages.
                ->Property("descriptor", [](InstanceData* thisPtr) { return thisPtr->m_descriptorPtr.get(); }, nullptr)
                ;
        }
    }

} // namespace Vegetation
