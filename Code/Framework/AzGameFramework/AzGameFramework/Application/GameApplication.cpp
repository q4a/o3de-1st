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

#include "GameApplication.h"
#include <AzCore/IO/Path/Path.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <AzCore/StringFunc/StringFunc.h>
#include <AzCore/std/string/conversions.h>
#include <AzFramework/Driller/RemoteDrillerInterface.h>
#include <AzFramework/Driller/DrillToFileComponent.h>
#include <GridMate/Drillers/CarrierDriller.h>
#include <GridMate/Drillers/ReplicaDriller.h>
#include <AzFramework/TargetManagement/TargetManagementComponent.h>
#include <AzFramework/Metrics/MetricsPlainTextNameRegistration.h>
#include <AzGameFramework/AzGameFrameworkModule.h>

namespace AzGameFramework
{
    GameApplication::GameApplication()
    {
    }
 
    GameApplication::GameApplication(int* argc, char*** argv)
        : Application(argc, argv)
    {
    }

    GameApplication::~GameApplication()
    {
    }

    void GameApplication::StartCommon(AZ::Entity* systemEntity)
    {
        AzFramework::Application::StartCommon(systemEntity);

        if (GetDrillerManager())
        {
            GetDrillerManager()->Register(aznew GridMate::Debug::CarrierDriller());
            GetDrillerManager()->Register(aznew GridMate::Debug::ReplicaDriller());
        }
    }

    void GameApplication::MergeSettingsToRegistry(AZ::SettingsRegistryInterface& registry)
    {
        AZ::SettingsRegistryInterface::Specializations specializations;
        Application::SetSettingsRegistrySpecializations(specializations);
        specializations.Append("game");

        AZStd::vector<char> scratchBuffer;

        AZ::SettingsRegistryMergeUtils::MergeSettingsToRegistry_TargetBuildDependencyRegistry(registry, AZ_TRAIT_OS_PLATFORM_CODENAME, specializations, &scratchBuffer);
#if defined(AZ_DEBUG_BUILD) || defined(AZ_PROFILE_BUILD)
        AZ::SettingsRegistryMergeUtils::MergeSettingsToRegistry_DevRegistry(registry, AZ_TRAIT_OS_PLATFORM_CODENAME, specializations, &scratchBuffer);
        AZ::SettingsRegistryMergeUtils::MergeSettingsToRegistry_CommandLine(registry, m_commandLine, true);
#endif

        // Used the lowercase the platform name since the bootstrap.game.<config>.<platform>.setreg is being loaded
        // from the asset cache root where all the files are in lowercased from regardless of the filesystem case-sensitivity
        static constexpr char filename[] = "bootstrap.game." AZ_BUILD_CONFIGURATION_TYPE "." AZ_TRAIT_OS_PLATFORM_CODENAME_LOWER ".setreg";

        AZ::IO::FixedMaxPath cacheRootPath;
        if (registry.Get(cacheRootPath.Native(), AZ::SettingsRegistryMergeUtils::FilePathKey_CacheRootFolder))
        {
            cacheRootPath /= filename;
            registry.MergeSettingsFile(cacheRootPath.Native(), AZ::SettingsRegistryInterface::Format::JsonMergePatch, "", &scratchBuffer);
        }

#if defined(AZ_DEBUG_BUILD) || defined(AZ_PROFILE_BUILD)
        AZ::SettingsRegistryMergeUtils::MergeSettingsToRegistry_DevRegistry(registry, AZ_TRAIT_OS_PLATFORM_CODENAME, specializations, &scratchBuffer);
        AZ::SettingsRegistryMergeUtils::MergeSettingsToRegistry_CommandLine(registry, m_commandLine, true);
#endif
    }

    AZ::ComponentTypeList GameApplication::GetRequiredSystemComponents() const
    {
        AZ::ComponentTypeList components = Application::GetRequiredSystemComponents();

#if !defined(_RELEASE)
        components.emplace_back(azrtti_typeid<AzFramework::TargetManagementComponent>());
#endif

        // Note that this component is registered by AzFramework.
        // It must be registered here instead of in the module so that existence of AzFrameworkModule is guaranteed.
        components.emplace_back(azrtti_typeid<AzFramework::DrillerNetworkAgentComponent>());

        return components;
    }

    void GameApplication::CreateStaticModules(AZStd::vector<AZ::Module*>& outModules)
    {
        AzFramework::Application::CreateStaticModules(outModules);

        outModules.emplace_back(aznew AzGameFrameworkModule());

        // have to let the metrics system know that it's ok to send back the name of the DrillerNetworkAgentComponent to Amazon as plain text, without hashing
        EBUS_EVENT(AzFramework::MetricsPlainTextNameRegistrationBus, RegisterForNameSending, AZStd::vector<AZ::Uuid>{ azrtti_typeid<AzFramework::DrillerNetworkAgentComponent>() });
    }

    void GameApplication::QueryApplicationType(AZ::ApplicationTypeQuery& appType) const
    { 
        appType.m_maskValue = AZ::ApplicationTypeQuery::Masks::Game;
    };

} // namespace AzGameFramework
