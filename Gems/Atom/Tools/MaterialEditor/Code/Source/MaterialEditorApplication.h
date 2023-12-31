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


#include <AzCore/Component/Entity.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/UserSettings/UserSettingsProvider.h>
#include <AzCore/Debug/TraceMessageBus.h>

#include <AzFramework/Application/Application.h>
#include <AzFramework/Asset/AssetSystemBus.h>
#include <AzFramework/Logging/LogFile.h>

#include <AzToolsFramework/API/AssetDatabaseBus.h>
#include <AzToolsFramework/API/EditorPythonConsoleBus.h>

#include <Atom/Document/MaterialDocumentSystemRequestBus.h>
#include <Atom/Window/MaterialEditorWindowNotificationBus.h>

#include <QApplication>
#include <QTimer>

namespace MaterialEditor
{
    class MaterialThumbnailRenderer;

    class MaterialEditorApplication
        : public AzFramework::Application
        , public QApplication
        , private AzToolsFramework::AssetDatabase::AssetDatabaseRequestsBus::Handler
        , private MaterialEditorWindowNotificationBus::Handler
        , private AzFramework::AssetSystemStatusBus::Handler
        , private AZ::UserSettingsOwnerRequestBus::Handler
        , private AZ::Debug::TraceMessageBus::Handler
        , private AzToolsFramework::EditorPythonConsoleNotificationBus::Handler
    {
    public:
        AZ_TYPE_INFO(MaterialEditor::MaterialEditorApplication, "{30F90CA5-1253-49B5-8143-19CEE37E22BB}");

        using Base = AzFramework::Application;

        MaterialEditorApplication(int* argc, char*** argv);
        virtual ~MaterialEditorApplication() = default;

        //////////////////////////////////////////////////////////////////////////
        // AzFramework::Application
        void CreateReflectionManager() override;
        void Reflect(AZ::ReflectContext* context) override;
        void RegisterCoreComponents() override;
        AZ::ComponentTypeList GetRequiredSystemComponents() const override;
        void CreateStaticModules(AZStd::vector<AZ::Module*>& outModules) override;
        const char* GetCurrentConfigurationName() const override;
        void StartCommon(AZ::Entity* systemEntity) override;
        void Tick(float deltaOverride = -1.f) override;
        void Stop() override;

    private:
        //////////////////////////////////////////////////////////////////////////
        // AssetDatabaseRequestsBus::Handler overrides...
        bool GetAssetDatabaseLocation(AZStd::string& result) override;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // MaterialEditorWindowNotificationBus::Handler overrides...
        void OnMaterialEditorWindowClosing() override;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // AzFramework::Application overrides...
        void Destroy() override;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // AZ::ComponentApplication overrides...
        void QueryApplicationType(AZ::ApplicationTypeQuery& appType) const override;
        //////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // EditorPythonConsoleNotificationBus::Handler overrides...
        void OnTraceMessage(AZStd::string_view message) override;
        void OnErrorMessage(AZStd::string_view message) override;
        void OnExceptionMessage(AZStd::string_view message) override;
        ////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // AzFramework::AssetSystemStatusBus::Handler overrides...
        void AssetSystemAvailable() override;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // AZ::UserSettingsOwnerRequestBus::Handler overrides...
        void SaveSettings() override;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // AZ::Debug::TraceMessageBus::Handler overrides...
        bool OnOutput(const char* window, const char* message) override;
        //////////////////////////////////////////////////////////////////////////

        void CompileCriticalAssets();

        void ProcessCommandLine();
        void WriteStartupLog();

        void LoadSettings();
        void UnloadSettings();

        bool LaunchDiscoveryService();

        void StartInternal();

        static void PyIdleWaitFrames(uint32_t frames);

        struct LogMessage
        {
            AZStd::string window;
            AZStd::string message;
        };

        AZStd::vector<LogMessage> m_startupLogSink;
        AZStd::unique_ptr<AzFramework::LogFile> m_logFile;

        //! Local user settings are used to store material browser tree expansion state
        AZ::UserSettingsProvider m_localUserSettings;

        //! Are local settings loaded
        bool m_activatedLocalUserSettings = false;

        QTimer m_timer;
    };
} // namespace MaterialEditor
