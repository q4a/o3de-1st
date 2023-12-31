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

#include <limits>
#include <AssetBuilderSDK/AssetBuilderSDK.h>
#include <AzCore/Settings/SettingsRegistryImpl.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <AzFramework/Platform/PlatformDefaults.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <native/InternalBuilders/SettingsRegistryBuilder.h>

namespace AssetProcessor
{
    void SettingsRegistryBuilder::SettingsExporter::WriteName(AZStd::string_view name)
    {
        if (m_includeName)
        {
            m_writer.Key(name.data(), aznumeric_caster(name.length()));
        }
    }

    SettingsRegistryBuilder::SettingsExporter::SettingsExporter(
        rapidjson::StringBuffer& buffer, const AZStd::vector<AZStd::string>& excludes)
        : m_writer(rapidjson::Writer<rapidjson::StringBuffer>(buffer))
        , m_excludes(excludes)
    {
    }

    AZ::SettingsRegistryInterface::VisitResponse SettingsRegistryBuilder::SettingsExporter::Traverse(
        AZStd::string_view path, AZStd::string_view valueName, AZ::SettingsRegistryInterface::VisitAction action,
        AZ::SettingsRegistryInterface::Type type)
    {
        for (const AZStd::string& exclude : m_excludes)
        {
            if (exclude == path)
            {
                return AZ::SettingsRegistryInterface::VisitResponse::Skip;
            }
        }

        if (action == AZ::SettingsRegistryInterface::VisitAction::Begin)
        {
            AZ_Assert(type == AZ::SettingsRegistryInterface::Type::Object || type == AZ::SettingsRegistryInterface::Type::Array,
                "Unexpected type visited: %i.", type);
            WriteName(valueName);
            if (type == AZ::SettingsRegistryInterface::Type::Object)
            {
                m_result = m_result && m_writer.StartObject();
                m_includeNameStack.push(true);
                m_includeName = true;
            }
            else
            {
                m_result = m_result && m_writer.StartArray();
                m_includeNameStack.push(false);
                m_includeName = false;
            }
        }
        else if (action == AZ::SettingsRegistryInterface::VisitAction::End)
        {
            if (type == AZ::SettingsRegistryInterface::Type::Object)
            {
                m_result = m_result && m_writer.EndObject();
            }
            else
            {
                m_result = m_result && m_writer.EndArray();
            }
            AZ_Assert(!m_includeNameStack.empty(), "Attempting to close a json array or object that wasn't started.");
            m_includeNameStack.pop();
            m_includeName = !m_includeNameStack.empty() ? m_includeNameStack.top() : true;
        }
        else if (type == AZ::SettingsRegistryInterface::Type::Null)
        {
            WriteName(valueName);
            m_result = m_result && m_writer.Null();
        }

        return m_result ?
            AZ::SettingsRegistryInterface::VisitResponse::Continue :
            AZ::SettingsRegistryInterface::VisitResponse::Done;
    }

    void SettingsRegistryBuilder::SettingsExporter::Visit(
        AZStd::string_view, AZStd::string_view valueName, AZ::SettingsRegistryInterface::Type, bool value)
    {
        WriteName(valueName);
        m_result = m_result && m_writer.Bool(value);
    }

    void SettingsRegistryBuilder::SettingsExporter::Visit(
        AZStd::string_view, AZStd::string_view valueName, AZ::SettingsRegistryInterface::Type, AZ::s64 value)
    {
        WriteName(valueName);
        m_result = m_result && m_writer.Int64(value);
    }

    void SettingsRegistryBuilder::SettingsExporter::Visit(
        AZStd::string_view, AZStd::string_view valueName, AZ::SettingsRegistryInterface::Type, AZ::u64 value)
    {
        WriteName(valueName);
        m_result = m_result && m_writer.Uint64(value);
    }

    void SettingsRegistryBuilder::SettingsExporter::Visit(
        AZStd::string_view, AZStd::string_view valueName, AZ::SettingsRegistryInterface::Type, double value)
    {
        WriteName(valueName);
        m_result = m_result && m_writer.Double(value);
    }

    void SettingsRegistryBuilder::SettingsExporter::Visit(
        AZStd::string_view, AZStd::string_view valueName, AZ::SettingsRegistryInterface::Type, AZStd::string_view value)
    {
        WriteName(valueName);
        m_result = m_result && m_writer.String(value.data(), aznumeric_caster(value.length()));
    }

    bool SettingsRegistryBuilder::SettingsExporter::Finalize()
    {
        if (!m_includeNameStack.empty())
        {
            AZ_Assert(false, "m_includeNameStack is expected to be empty. This means that there was an object or array what wasn't closed.");
            return false;
        }
        return m_result;
    }

    void SettingsRegistryBuilder::SettingsExporter::Reset(rapidjson::StringBuffer& buffer)
    {
        m_writer.Reset(buffer);
        m_includeName = false;
        m_result = true;
    }



    SettingsRegistryBuilder::SettingsRegistryBuilder()
        : m_builderId("{1BB18B28-2953-4922-A80B-E7375FCD7FC1}")
        , m_assetType("{FEBB3C7B-9C8B-46C3-8AAF-3D132D811087}")
    {
        AssetBuilderSDK::AssetBuilderCommandBus::Handler::BusConnect(m_builderId);
    }

    bool SettingsRegistryBuilder::Initialize()
    {
        AssetBuilderSDK::AssetBuilderDesc   builderDesc;
        builderDesc.m_name = "Settings Registry Builder";
        builderDesc.m_patterns.emplace_back("*/bootstrap.cfg", AssetBuilderSDK::AssetBuilderPattern::PatternType::Wildcard);
        builderDesc.m_builderType = AssetBuilderSDK::AssetBuilderDesc::AssetBuilderType::Internal;
        builderDesc.m_busId = m_builderId;
        builderDesc.m_createJobFunction = AZStd::bind(&SettingsRegistryBuilder::CreateJobs, this, AZStd::placeholders::_1, AZStd::placeholders::_2);
        builderDesc.m_processJobFunction = AZStd::bind(&SettingsRegistryBuilder::ProcessJob, this, AZStd::placeholders::_1, AZStd::placeholders::_2);

        AssetBuilderSDK::AssetBuilderBus::Broadcast(&AssetBuilderSDK::AssetBuilderBusTraits::RegisterBuilderInformation, builderDesc);
        
        return true;
    }

    void SettingsRegistryBuilder::Uninitialize() {}

    void SettingsRegistryBuilder::ShutDown()
    {
        m_isShuttingDown = true;
    }

    void SettingsRegistryBuilder::CreateJobs(
        const AssetBuilderSDK::CreateJobsRequest& request, AssetBuilderSDK::CreateJobsResponse& response)
    {
        if (m_isShuttingDown)
        {
            response.m_result = AssetBuilderSDK::CreateJobsResultCode::ShuttingDown;
            return;
        }

        for (const AssetBuilderSDK::PlatformInfo& info : request.m_enabledPlatforms)
        {
            AssetBuilderSDK::JobDescriptor job;
            job.m_jobKey = "Settings Registry";
            // The settings are the very first thing the game reads so needs to available before anything else.
            job.m_priority = std::numeric_limits<decltype(job.m_priority)>::max();
            job.m_critical = true;
            job.SetPlatformIdentifier(info.m_identifier.c_str());
            response.m_createJobOutputs.push_back(AZStd::move(job));
        }

        response.m_sourceFileDependencyList.emplace_back("*.setreg", AZ::Uuid::CreateNull(),
            AssetBuilderSDK::SourceFileDependency::SourceFileDependencyType::Wildcards);
        response.m_result = AssetBuilderSDK::CreateJobsResultCode::Success;
    }

    void SettingsRegistryBuilder::ProcessJob(
        const AssetBuilderSDK::ProcessJobRequest& request, AssetBuilderSDK::ProcessJobResponse& response)
    {
        if (m_isShuttingDown)
        {
            response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Cancelled;
            return;
        }

        response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Failed;

        AZStd::vector<AZStd::string> excludes = ReadExcludesFromRegistry();
        
        AZStd::vector<char> scratchBuffer;
        scratchBuffer.reserve(512 * 1024); // Reserve 512kb to avoid repeatedly resizing the buffer;
        AZStd::fixed_vector<AZStd::string_view, AzFramework::MaxPlatformCodeNames> platformCodes;
        AzFramework::PlatformHelper::AppendPlatformCodeNames(platformCodes, request.m_platformInfo.m_identifier);
        const AZStd::string& assetPlatformIdentifier = request.m_jobDescription.GetPlatformIdentifier();
        // Determines the suffix that will be used for the launcher based on processing server vs non-server assets
        const char* launcherType = assetPlatformIdentifier != AzFramework::PlatformHelper::GetPlatformName(AzFramework::PlatformId::SERVER)
            ? "_GameLauncher" : "_ServerLauncher";
        
        AZ::SettingsRegistryInterface::Specializations specializations[] =
        {
            { AZStd::string_view{"release"}, AZStd::string_view{"game"} },
            { AZStd::string_view{"profile"}, AZStd::string_view{"game"} },
            { AZStd::string_view{"debug"}, AZStd::string_view{"game"} }
        };

        // Add the project specific specializations
        if (auto settingsRegistry = AZ::Interface<AZ::SettingsRegistryInterface>::Get(); settingsRegistry)
        {
            auto projectKey = AZ::SettingsRegistryInterface::FixedValueString::format("%s/sys_game_folder", AZ::SettingsRegistryMergeUtils::BootstrapSettingsRootKey);
            if (AZ::SettingsRegistryInterface::FixedValueString projectName; settingsRegistry->Get(projectName, projectKey))
            {
                for (AZ::SettingsRegistryInterface::Specializations& specialization : specializations)
                {
                    specialization.Append(projectName);
                    // The Game Launcher normally has a build target name of <ProjectName>Launcher
                    // Add that as a specialization to pick up the gem dependencies files that are specialized
                    // on a the Game Launcher target if the asset platform isn't "server"
                    specialization.Append(projectName + launcherType);
                }
            }
        }

        AZStd::string outputPath;
        AzFramework::StringFunc::Path::Join(request.m_tempDirPath.c_str(), "bootstrap.game.", outputPath);
        size_t extensionOffset = outputPath.length();

        rapidjson::StringBuffer outputBuffer;
        outputBuffer.Reserve(512 * 1024); // Reserve 512kb to avoid repeatedly resizing the buffer;
        SettingsExporter exporter(outputBuffer, excludes);

        for (AZStd::string_view platform : platformCodes)
        {
            AZ::u32 productSubID = static_cast<AZ::u32>(AZStd::hash<AZStd::string_view>{}(platform)); // Deliberately ignoring half the bits.
            for (size_t i = 0; i < AZ_ARRAY_SIZE(specializations); ++i)
            {
                if (m_isShuttingDown)
                {
                    response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Cancelled;
                    return;
                }

                AZ::SettingsRegistryImpl registry;

                // Seed the local settings registry using the AssetProcessor settings registry
                if (auto settingsRegistry = AZ::Interface<AZ::SettingsRegistryInterface>::Get(); settingsRegistry != nullptr)
                {
                    AZStd::array settingsToCopy{
                        AZStd::string::format("%s/sys_game_folder", AZ::SettingsRegistryMergeUtils::BootstrapSettingsRootKey),
                        AZStd::string{AZ::SettingsRegistryMergeUtils::FilePathKey_BinaryFolder},
                        AZStd::string{AZ::SettingsRegistryMergeUtils::FilePathKey_EngineRootFolder},
                        AZStd::string{AZ::SettingsRegistryMergeUtils::FilePathKey_SourceGameFolder},
                        AZStd::string{AZ::SettingsRegistryMergeUtils::FilePathKey_CacheRootFolder},
                        AZStd::string{AZ::SettingsRegistryMergeUtils::FilePathKey_CacheGameFolder}
                    };

                    for (const auto& settingsKey : settingsToCopy)
                    {
                        AZ::SettingsRegistryInterface::FixedValueString settingsValue;
                        bool settingsCopied = settingsRegistry->Get(settingsValue, settingsKey)
                            && registry.Set(settingsKey, settingsValue);
                        AZ_Warning("Settings Registry Builder", settingsCopied, "Unable to copy setting %s from AssetProcessor settings registry"
                            " to local settings registry", settingsKey.c_str());
                    }
                }

                AZ::SettingsRegistryMergeUtils::MergeSettingsToRegistry_Bootstrap(registry);
                AZ::SettingsRegistryMergeUtils::MergeSettingsToRegistry_EngineRegistry(registry, platform, specializations[i], &scratchBuffer);
                AZ::SettingsRegistryMergeUtils::MergeSettingsToRegistry_GemRegistries(registry, platform, specializations[i], &scratchBuffer);
                AZ::SettingsRegistryMergeUtils::MergeSettingsToRegistry_ProjectRegistry(registry, platform, specializations[i], &scratchBuffer);

                // Merge the Developer User settings registry only in non-release builds
                if (!specializations->Contains("release"))
                {
                    AZ::SettingsRegistryMergeUtils::MergeSettingsToRegistry_DevRegistry(registry, platform, specializations[i], &scratchBuffer);
                }

                AZ::ComponentApplicationBus::Broadcast([&registry](AZ::ComponentApplicationRequests* appRequests)
                {
                    if (AZ::CommandLine* commandLine = appRequests->GetAzCommandLine(); commandLine != nullptr)
                    {
                        constexpr bool executeRegDumpCommands = false;
                        AZ::SettingsRegistryMergeUtils::MergeSettingsToRegistry_CommandLine(registry, *commandLine, executeRegDumpCommands);
                    }
                });

                if (registry.Visit(exporter, ""))
                {
                    if (!exporter.Finalize())
                    {
                        return;
                    }

                    outputPath += specializations[i].GetSpecialization(0); // Append configuration
                    outputPath += '.';
                    outputPath += platform;
                    outputPath += ".setreg";

                    AZ::IO::SystemFile file;
                    if (!file.Open(outputPath.c_str(),
                        AZ::IO::SystemFile::OpenMode::SF_OPEN_CREATE | AZ::IO::SystemFile::OpenMode::SF_OPEN_WRITE_ONLY))
                    {
                        AZ_Error("Settings Registry Builder", false, R"(Failed to open file "%s" for writing.)", outputPath.c_str());
                        return;
                    }
                    if (file.Write(outputBuffer.GetString(), outputBuffer.GetSize()) != outputBuffer.GetSize())
                    {
                        AZ_Error("Settings Registry Builder", false, R"(Failed to write settings registry to file "%s".)", outputPath.c_str());
                        return;
                    }
                    file.Close();

                    response.m_outputProducts.emplace_back(outputPath, m_assetType, productSubID + aznumeric_cast<AZ::u32>(i));

                    outputPath.erase(extensionOffset);
                }

                outputBuffer.Clear();
                exporter.Reset(outputBuffer);
            }
        }

        response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;
    }

    AZStd::vector<AZStd::string> SettingsRegistryBuilder::ReadExcludesFromRegistry() const
    {
        AZStd::vector<AZStd::string> excludes;
        auto builderRegistry = AZ::SettingsRegistry::Get();
        AZStd::string path = "/Amazon/AssetBuilder/SettingsRegistry/Excludes/";
        size_t offset = path.length();
        size_t counter = 0;
        do
        {
            path += AZStd::to_string(counter);

            AZStd::string exclude;
            if (builderRegistry->Get(exclude, path))
            {
                excludes.push_back(AZStd::move(exclude));
            }
            else
            {
                return excludes;
            }

            counter++;
            path.erase(offset);
        } while (true);
    }
} // namespace AssetProcessor
