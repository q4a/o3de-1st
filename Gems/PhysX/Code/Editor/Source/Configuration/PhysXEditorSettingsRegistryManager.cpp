/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates, or
* a third party where indicated.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#include <Editor/Source/Configuration/PhysXEditorSettingsRegistryManager.h>

#include <AzCore/IO/SystemFile.h>
#include <AzCore/IO/TextStreamWriters.h>
#include <AzCore/JSON/document.h>
#include <AzCore/JSON/pointer.h>
#include <AzCore/JSON/prettywriter.h>
#include <AzCore/Serialization/Json/JsonSerialization.h>
#include <AzCore/Settings/SettingsRegistry.h>
#include <AzToolsFramework/SourceControl/SourceControlAPI.h>

namespace PhysX
{
    namespace Internal
    {
        AZStd::string WriteDocumentToString(const rapidjson::Document& document)
        {
            AZStd::string stringBuffer;
            AZ::IO::ByteContainerStream stringStream(&stringBuffer);
            AZ::IO::RapidJSONStreamWriter stringWriter(&stringStream);
            rapidjson::PrettyWriter writer(stringWriter);
            document.Accept(writer);
            return stringBuffer;
        }

        // Capture the m_physxConfiguration by value to allow the the save to occur successfully
        // even if the SystemComponent is deleted later
        AzToolsFramework::SourceControlResponseCallback GetConfigurationSaveCallback(
            AZStd::string configurationPayload, AZStd::function<void(bool)> postSaveCallback)
        {
            return [payloadBuffer = AZStd::move(configurationPayload), postSaveCB = AZStd::move(postSaveCallback)]
            (bool, const AzToolsFramework::SourceControlFileInfo& info)
            {
                // Save PhysX configuration.
                if (info.IsLockedByOther())
                {
                    AZ_Warning("PhysXEditor", false, R"(The file "%s" already exclusively opened by another user)", info.m_filePath.c_str());
                    return;
                }
                else if (info.IsReadOnly() && AZ::IO::SystemFile::Exists(info.m_filePath.c_str()))
                {
                    AZ_Warning("PhysXEditor", false, R"(The file "%s" is read-only)", info.m_filePath.c_str());
                    return;
                }

                bool saved = false;
                constexpr auto configurationMode
                    = AZ::IO::SystemFile::SF_OPEN_CREATE
                    | AZ::IO::SystemFile::SF_OPEN_CREATE_PATH
                    | AZ::IO::SystemFile::SF_OPEN_WRITE_ONLY;
                if (AZ::IO::SystemFile outputFile; outputFile.Open(info.m_filePath.c_str(), configurationMode))
                {
                    saved = outputFile.Write(payloadBuffer.data(), payloadBuffer.size()) == payloadBuffer.size();
                }

                AZ_Warning("PhysXEditor", saved, "Failed to save PhysX configuration");
                if (postSaveCB)
                {
                    postSaveCB(saved);
                }
            };
        }
    } //namespace Internal

    PhysXEditorSettingsRegistryManager::PhysXEditorSettingsRegistryManager()
        : PhysXSettingsRegistryManager()
    {
        // Resolve path to the .setreg files
        auto* fileIo = AZ::IO::FileIOBase::GetInstance();
        if (fileIo == nullptr)
        {
            AZ_TracePrintf("PhysXSystemEditor", R"(Unable to resolve paths for PhysX configuration settings registry files, FileIOBase is null.\n)");
            return;
        }
        if (!fileIo->ResolvePath(m_physXConfigurationFilePath, "@devassets@/Registry/physxsystemconfiguration.setreg"))
        {
            AZ_TracePrintf("PhysXSystemEditor", R"(Unable to resolve path "%s" to the PhysX configuration settings registry file\n)",
                m_physXConfigurationFilePath.c_str());
            return;
        }
        if (!fileIo->ResolvePath(m_defaultSceneConfigFilePath, "@devassets@/Registry/physxdefaultsceneconfiguration.setreg"))
        {
            AZ_TracePrintf("PhysXSystemEditor", R"(Unable to resolve path "%s" to the Default Scene Configuration settings registry file\n)",
                m_defaultSceneConfigFilePath.c_str());
            return;
        }
        if(!fileIo->ResolvePath(m_debugConfigurationFilePath, "@devassets@/Registry/physxdebugconfiguration.setreg"))
        {
            AZ_TracePrintf("PhysXSystemEditor", R"(Unable to resolve path "%s" to the PhysX Debug Configuration settings registry file\n)",
                m_debugConfigurationFilePath.c_str());
            return;
        }
        m_initialized = true;
    }

    void PhysXEditorSettingsRegistryManager::SaveSystemConfiguration(const PhysXSystemConfiguration& config, const OnPhysXConfigSaveComplete& saveCallback) const
    {
        if (!m_initialized)
        {
            AZ_Warning("PhysXSystemEditor", false, "Unable to save physX configurations. PhysX Editor Settings Registry Manager could not initialize");
            if (saveCallback)
            {
                saveCallback(config, Result::Failed);
            }
            return;
        }
        // Save configuration to source folder when in edit mode.
        // Use the SourceControl API to make sure the .setreg files
        // are checked out from source control or are writable before attempting to save it
        // The SourceControlCommandBus callbacks must be used as checking out a file is an asynchronous
        // operation that doesn't complete immediately
        bool sourceControlActive = false;
        AzToolsFramework::SourceControlConnectionRequestBus::BroadcastResult(sourceControlActive,
            &AzToolsFramework::SourceControlConnectionRequests::IsActive);
        // If Source Control is active then use it to check out the file before saving
        // otherwise query the file info and save only if the file is not read-only
        auto SourceControlSaveCallback = [sourceControlActive](AzToolsFramework::SourceControlCommands* sourceControlCommands,
            const char* filePath, const AzToolsFramework::SourceControlResponseCallback& configurationSaveCallback)
        {
            if (sourceControlActive)
            {
                sourceControlCommands->RequestEdit(filePath, true, configurationSaveCallback);
            }
            else
            {
                sourceControlCommands->GetFileInfo(filePath, configurationSaveCallback);
            }
        };

        // Save PhysX System Configuration Settings Registry file
        rapidjson::Document physXConfigurationDocument;
        rapidjson::Value& physXConfigurationValue = rapidjson::CreateValueByPointer(physXConfigurationDocument, rapidjson::Pointer(m_settingsRegistryPath.c_str()));
        AZ::JsonSerialization::Store(physXConfigurationValue, physXConfigurationDocument.GetAllocator(), config);

        auto postSaveCallback = [config, saveCallback](bool result)
        {
            if (saveCallback)
            {
                saveCallback(config, result ? Result::Success : Result::Failed);
            }
        };

        AzToolsFramework::SourceControlCommandBus::Broadcast(SourceControlSaveCallback,
            m_physXConfigurationFilePath.c_str(),
            Internal::GetConfigurationSaveCallback(Internal::WriteDocumentToString(physXConfigurationDocument), postSaveCallback));
    }

    void PhysXEditorSettingsRegistryManager::SaveDefaultSceneConfiguration(const AzPhysics::SceneConfiguration& config, const OnDefaultSceneConfigSaveComplete& saveCallback) const
    {
        if (!m_initialized)
        {
            AZ_Warning("PhysXSystemEditor", false, "Unable to save physX configurations. PhysX Editor Settings Registry Manager could not initialize");
            if (saveCallback)
            {
                saveCallback(config, Result::Failed);
            }
            return;
        }
        
        bool sourceControlActive = false;
        AzToolsFramework::SourceControlConnectionRequestBus::BroadcastResult(sourceControlActive,
            &AzToolsFramework::SourceControlConnectionRequests::IsActive);
        
        auto SourceControlSaveCallback = [sourceControlActive](AzToolsFramework::SourceControlCommands* sourceControlCommands,
            const char* filePath, const AzToolsFramework::SourceControlResponseCallback& configurationSaveCallback)
        {
            if (sourceControlActive)
            {
                sourceControlCommands->RequestEdit(filePath, true, configurationSaveCallback);
            }
            else
            {
                sourceControlCommands->GetFileInfo(filePath, configurationSaveCallback);
            }
        };

        // Save PhysX System Configuration Settings Registry file
        rapidjson::Document configDoc;
        rapidjson::Value& configValue = rapidjson::CreateValueByPointer(configDoc, rapidjson::Pointer(m_defaultSceneConfigSettingsRegistryPath.c_str()));
        AZ::JsonSerialization::Store(configValue, configDoc.GetAllocator(), config);

        auto postSaveCallback = [config, saveCallback](bool result)
        {
            if (saveCallback)
            {
                saveCallback(config, result ? Result::Success : Result::Failed);
            }
        };

        AzToolsFramework::SourceControlCommandBus::Broadcast(SourceControlSaveCallback,
            m_defaultSceneConfigFilePath.c_str(),
            Internal::GetConfigurationSaveCallback(Internal::WriteDocumentToString(configDoc), postSaveCallback));
    }

    void PhysXEditorSettingsRegistryManager::SaveDebugConfiguration(const Debug::DebugConfiguration& config, const OnPhysXDebugConfigSaveComplete& saveCallback) const
    {
        if (!m_initialized)
        {
            AZ_Warning("PhysXSystemEditor", false, "Unable to save physX configurations. PhysX Editor Settings Registry Manager could not initialize");
            if (saveCallback)
            {
                saveCallback(config, Result::Failed);
            }
            return;
        }

        // Save configuration to source folder when in edit mode.
        // Use the SourceControl API to make sure the .setreg files
        // Are checked out from source control or are writable before attempting to save it
        // The SourceControlCommandBus callbacks must be used as checking out a file is an asynchronous
        // operation that doesn't complete immediately
        bool sourceControlActive{};
        AzToolsFramework::SourceControlConnectionRequestBus::BroadcastResult(sourceControlActive,
            &AzToolsFramework::SourceControlConnectionRequests::IsActive);
        // If Source Control is active then use it to check out the file before saving
        // otherwise query the file info and save only if the file is not read-only
        auto SourceControlSaveCallback = [sourceControlActive](AzToolsFramework::SourceControlCommands* sourceControlCommands,
            const char* filePath, const AzToolsFramework::SourceControlResponseCallback& configurationSaveCallback)
        {
            if (sourceControlActive)
            {
                sourceControlCommands->RequestEdit(filePath, true, configurationSaveCallback);
            }
            else
            {
                sourceControlCommands->GetFileInfo(filePath, configurationSaveCallback);
            }
        };

        // Save PhysX debug Configuration Settings Registry file
        rapidjson::Document debugConfigurationDocument;
        rapidjson::Value& debugConfigurationValue = rapidjson::CreateValueByPointer(debugConfigurationDocument, rapidjson::Pointer(m_debugSettingsRegistryPath.c_str()));
        AZ::JsonSerialization::Store(debugConfigurationValue, debugConfigurationDocument.GetAllocator(), config);

        auto postSaveCallback = [config, saveCallback](bool result)
        {
            if (saveCallback)
            {
                saveCallback(config, result ? Result::Success : Result::Failed);
            }
        };
        AzToolsFramework::SourceControlCommandBus::Broadcast(SourceControlSaveCallback,
            m_debugConfigurationFilePath.c_str(),
            Internal::GetConfigurationSaveCallback(Internal::WriteDocumentToString(debugConfigurationDocument), postSaveCallback));
    }
} //namespace PhysX

