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

#include "EditorAssetImporter_precompiled.h"
#include <AssetImporterPlugin.h>
#include <AssetImporterWindow.h>
#include <QtViewPaneManager.h>
#include <SceneAPI/SceneCore/Utilities/Reporting.h>
#include <LyViewPaneNames.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>

AssetImporterPlugin* AssetImporterPlugin::s_instance;

AssetImporterPlugin::AssetImporterPlugin(IEditor* editor)
    : m_editor(editor)
    , m_toolName(LyViewPane::SceneSettings)
    , m_assetBrowserContextProvider()
    , m_sceneSerializationHandler()
{
    s_instance = this;

    m_sceneUIModule = LoadSceneLibrary("SceneUI", true);

    m_sceneSerializationHandler.Activate();
    
    AzToolsFramework::ViewPaneOptions opt;
    opt.isPreview = true;
    opt.showInMenu = false; // this view pane is used to display scene settings, but the user never opens it directly through the Tools menu
    opt.saveKeyName = "Scene Settings (PREVIEW)"; // user settings for this pane were originally saved with PREVIEW, so ensure that's how they are loaded as well, even after the PREVIEW is removed from the name
    AzToolsFramework::RegisterViewPane<AssetImporterWindow>(m_toolName.c_str(), LyViewPane::CategoryTools, opt);
}

void AssetImporterPlugin::Release()
{
    AzToolsFramework::UnregisterViewPane(m_toolName.c_str());
    m_sceneSerializationHandler.Deactivate();

    auto uninit = m_sceneUIModule->GetFunction<AZ::UninitializeDynamicModuleFunction>(AZ::UninitializeDynamicModuleFunctionName);
    if (uninit)
    {
        (*uninit)();
    }
    m_sceneUIModule.reset();
}

AZStd::unique_ptr<AZ::DynamicModuleHandle> AssetImporterPlugin::LoadSceneLibrary(const char* name, bool explicitInit)
{
    using ReflectFunc = void(*)(AZ::SerializeContext*);

    AZStd::unique_ptr<AZ::DynamicModuleHandle> module = AZ::DynamicModuleHandle::Create(name);
    if (module)
    {
        module->Load(false);

        if (explicitInit)
        {
            // Explicitly initialize all modules. Because we're loading them twice (link time, and now-time), we need to explicitly uninit them.
            auto init = module->GetFunction<AZ::InitializeDynamicModuleFunction>(AZ::InitializeDynamicModuleFunctionName);
            if (init)
            {
                (*init)(AZ::Environment::GetInstance());
            }
        }

        ReflectFunc reflect = module->GetFunction<ReflectFunc>("Reflect");
        if (reflect)
        {
            (*reflect)(nullptr);
        }
        return module;
    }
    else
    {
        AZ_TracePrintf(AZ::SceneAPI::Utilities::ErrorWindow, "Failed to initialize library '%s'", name);
        return nullptr;
    }
}

void AssetImporterPlugin::EditImportSettings(const AZStd::string& sourceFilePath)
{
    const QtViewPane* assetImporterPane = GetIEditor()->OpenView(m_toolName.c_str());
    AssetImporterWindow* assetImporterWindow = qobject_cast<AssetImporterWindow*>(assetImporterPane->Widget());
    if (!assetImporterWindow)
    {
        return;
    }

    assetImporterWindow->OpenFile(sourceFilePath);
}
