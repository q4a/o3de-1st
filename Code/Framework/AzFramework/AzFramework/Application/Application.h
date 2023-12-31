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

#include <AzCore/base.h>
#include <AzCore/Component/ComponentApplication.h>
#include <AzCore/UserSettings/UserSettings.h>
#include <AzCore/Math/Uuid.h>
#include <AzCore/IO/SystemFile.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/std/string/fixed_string.h>

#include <AzFramework/Network/NetSystemBus.h>
#include <AzFramework/CommandLine/CommandLine.h>
#include <AzFramework/API/ApplicationAPI.h>


namespace AZ
{
    class Component;

    namespace Internal
    {
        class ComponentFactoryInterface;
    }

    namespace IO
    {
        class Archive;
        class FileIOBase;
        class LocalFileIO;
    }
}

namespace AzFramework
{
    class Application
        : public AZ::ComponentApplication
        , public AZ::UserSettingsFileLocatorBus::Handler
        , public ApplicationRequests::Bus::Handler
        , public NetSystemRequestBus::Handler
    {
    public:
        // Base class for platform specific implementations of the application.
        class Implementation
        {
        public:
            static Implementation* Create();

            virtual ~Implementation() = default;
            virtual void PumpSystemEventLoopOnce() = 0;
            virtual void PumpSystemEventLoopUntilEmpty() = 0;
            virtual void TerminateOnError(int errorCode) { exit(errorCode); }
        };

        AZ_RTTI(Application, "{0BD2388B-F435-461C-9C84-D0A96CAF32E4}", AZ::ComponentApplication);
        AZ_CLASS_ALLOCATOR(Application, AZ::SystemAllocator, 0);

        // Publicized types & methods from base ComponentApplication.
        using AZ::ComponentApplication::Descriptor;
        using AZ::ComponentApplication::StartupParameters;
        using AZ::ComponentApplication::GetSerializeContext;
        using AZ::ComponentApplication::RegisterComponentDescriptor;

        /**
         * You can pass your command line parameters from main here
         * so that they are thus available in GetCommandLine later, and can be retrieved.
         * see notes in GetArgC() and GetArgV() for details about the arguments.
         */
        Application(int* argc, char*** argv);  ///< recommended:  supply &argc and &argv from void main(...) here.
        Application(); ///< for backward compatibility.  If you call this, GetArgC and GetArgV will return nullptr.
        ~Application();

        /**
         * Executes the AZ:ComponentApplication::Create method and initializes Application constructs.
         * Uses a variant to maintain backwards compatibility with both the Start(const Descriptor& descriptor, const StartupParamters&)
         * and the Start(const char* descriptorFile, const StaartupParameters&) overloads
         */
        virtual void Start(const Descriptor& descriptor, const StartupParameters& startupParameters = StartupParameters());
        /**
         * Executes AZ::ComponentApplication::Destroy, and shuts down Application specific constructs.
         */
        virtual void Stop();

        void Tick(float deltaOverride = -1.f) override;


        AZ::ComponentTypeList GetRequiredSystemComponents() const override;
        void CreateStaticModules(AZStd::vector<AZ::Module*>& outModules) override;

        //////////////////////////////////////////////////////////////////////////
        //! ApplicationRequests::Bus::Handler
        const char* GetAssetRoot() const override;
        const char* GetEngineRoot() const override { return m_engineRoot.c_str(); }
        const char* GetAppRoot() const override;
        void ResolveEnginePath(AZStd::string& engineRelativePath) const override;
        void CalculateBranchTokenForAppRoot(AZStd::string& token) const override;

#pragma push_macro("GetCommandLine")
#undef GetCommandLine
        const CommandLine* GetCommandLine() override { return &m_commandLine; }
#pragma pop_macro("GetCommandLine")
        const CommandLine* GetApplicationCommandLine() override { return &m_commandLine; }

        void SetAssetRoot(const char* assetRoot) override;
        void MakePathRootRelative(AZStd::string& fullPath) override;
        void MakePathAssetRootRelative(AZStd::string& fullPath) override;
        void MakePathRelative(AZStd::string& fullPath, const char* rootPath) override;
        void NormalizePath(AZStd::string& path) override;
        void NormalizePathKeepCase(AZStd::string& path) override;
        void PumpSystemEventLoopOnce() override;
        void PumpSystemEventLoopUntilEmpty() override;
        void PumpSystemEventLoopWhileDoingWorkInNewThread(const AZStd::chrono::milliseconds& eventPumpFrequency,
                                                          const AZStd::function<void()>& workForNewThread,
                                                          const char* newThreadName) override;
        void RunMainLoop() override;
        void ExitMainLoop() override { m_exitMainLoopRequested = true; }
        bool WasExitMainLoopRequested() override { return m_exitMainLoopRequested; }
        void TerminateOnError(int errorCode) override;
        AZ::Uuid GetComponentTypeId(const AZ::EntityId& entityId, const AZ::ComponentId& componentId) override;

        //////////////////////////////////////////////////////////////////////////

        // Convenience function that should be called instead of the standard exit() function to ensure platform requirements are met.
        static void Exit(int errorCode) { ApplicationRequests::Bus::Broadcast(&ApplicationRequests::TerminateOnError, errorCode); }

        //////////////////////////////////////////////////////////////////////////
        //! NetSystemEventBus::Handler
        //////////////////////////////////////////////////////////////////////////
        NetworkContext* GetNetworkContext() override;

    protected:

        /**
         * Called by Start method. Override to add custom startup logic.
         */
        virtual void StartCommon(AZ::Entity* systemEntity);

        /**
         * set the LocalFileIO and ArchiveFileIO instances file aliases if the 
         * FileIOBase environment variable is pointing to the instances owned by
         * the application
         */
        void SetFileIOAliases();

        void PreModuleLoad() override;

        //////////////////////////////////////////////////////////////////////////
        //! AZ::ComponentApplication
        void RegisterCoreComponents() override;
        void Reflect(AZ::ReflectContext* context) override;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        //! UserSettingsFileLocatorBus
        AZStd::string ResolveFilePath(AZ::u32 providerId) override;
        //////////////////////////////////////////////////////////////////////////

        AZ::Component* EnsureComponentAdded(AZ::Entity* systemEntity, const AZ::Uuid& typeId);

        template <typename ComponentType>
        AZ::Component* EnsureComponentAdded(AZ::Entity* systemEntity)
        {
            return EnsureComponentAdded(systemEntity, ComponentType::RTTI_Type());
        }

        virtual const char* GetCurrentConfigurationName() const;

        void CreateReflectionManager() override;

        AZ::StringFunc::Path::FixedString m_configFilePath;

        AZ::StringFunc::Path::FixedString m_assetRoot;
        AZ::StringFunc::Path::FixedString m_engineRoot; ///> Location of the engine root folder that this application is based on

        AZStd::unique_ptr<AZ::IO::LocalFileIO> m_directFileIO; ///> The Direct file IO instance is a LocalFileIO.
        AZStd::unique_ptr<AZ::IO::FileIOBase> m_archiveFileIO; ///> The Default file IO instance is a ArchiveFileIO.
        AZStd::unique_ptr<AZ::IO::Archive> m_archive; ///> The AZ::IO::Instance
        AZStd::unique_ptr<Implementation> m_pimpl;
        bool m_ownsConsole = false;

        bool m_exitMainLoopRequested = false;
        
        enum class RootPathType
        {
            AppRoot,
            AssetRoot,
            EngineRoot
        };
        void SetRootPath(RootPathType type, const char* source);
    };
} // namespace AzFramework

