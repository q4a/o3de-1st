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

#include <AzCore/AzCoreModule.h>

// Component includes
#include <AzCore/Asset/AssetManagerComponent.h>
#include <AzCore/Debug/FrameProfilerComponent.h>
#include <AzCore/IO/Streamer/StreamerComponent.h>
#include <AzCore/Jobs/JobManagerComponent.h>
#include <AzCore/Serialization/Json/JsonSystemComponent.h>
#include <AzCore/Memory/MemoryComponent.h>
#include <AzCore/NativeUI/NativeUISystemComponent.h>
#include <AzCore/Script/ScriptSystemComponent.h>
#include <AzCore/Slice/SliceComponent.h>
#include <AzCore/Slice/SliceSystemComponent.h>
#include <AzCore/Slice/SliceMetadataInfoComponent.h>
#include <AzCore/Statistics/StatisticalProfilerProxySystemComponent.h>
#include <AzCore/UserSettings/UserSettingsComponent.h>
#include <AzCore/Time/TimeSystemComponent.h>
#include <AzCore/Console/LoggerSystemComponent.h>
#include <AzCore/EBus/EventSchedulerSystemComponent.h>

namespace AZ
{
    AzCoreModule::AzCoreModule()
        : AZ::Module()
    {
        m_descriptors.insert(m_descriptors.end(), {
            MemoryComponent::CreateDescriptor(),
            StreamerComponent::CreateDescriptor(),
            JobManagerComponent::CreateDescriptor(),
            JsonSystemComponent::CreateDescriptor(),
            AssetManagerComponent::CreateDescriptor(),
            UserSettingsComponent::CreateDescriptor(),
            Debug::FrameProfilerComponent::CreateDescriptor(),
            NativeUI::NativeUISystemComponent::CreateDescriptor(),
            SliceComponent::CreateDescriptor(),
            SliceSystemComponent::CreateDescriptor(),
            SliceMetadataInfoComponent::CreateDescriptor(),
            TimeSystemComponent::CreateDescriptor(),
            LoggerSystemComponent::CreateDescriptor(),
            EventSchedulerSystemComponent::CreateDescriptor(),

#if !defined(_RELEASE)
            Statistics::StatisticalProfilerProxySystemComponent::CreateDescriptor(),
#endif // #if !defined(_RELEASE)
#if !defined(AZCORE_EXCLUDE_LUA)
            ScriptSystemComponent::CreateDescriptor(),
#endif // #if !defined(AZCORE_EXCLUDE_LUA)
        });
    }

    AZ::ComponentTypeList AzCoreModule::GetRequiredSystemComponents() const
    {
        return AZ::ComponentTypeList
        {
            azrtti_typeid<TimeSystemComponent>(),
            azrtti_typeid<LoggerSystemComponent>(),
            azrtti_typeid<EventSchedulerSystemComponent>(),

#if !defined(_RELEASE)
            azrtti_typeid<AZ::Statistics::StatisticalProfilerProxySystemComponent>(),
#endif // #if !defined(_RELEASE)
        };
    }
}
