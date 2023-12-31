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

#include <AzCore/Component/Component.h>

#include <Atom/RPI.Reflect/Material/MaterialAsset.h>

#include <Atom/Window/ShaderManagementConsoleWindowRequestBus.h>
#include <Source/Window/ShaderManagementConsoleBrowserInteractions.h>
#include <Source/Window/ShaderManagementConsoleWindow.h>

namespace ShaderManagementConsole
{
    //! ShaderManagementConsoleWindowComponent is the entry point for the Shader Management Console gem user interface, and is mainly
    //! used for initialization and registration of other classes, including ShaderManagementConsoleWindow.
    class ShaderManagementConsoleWindowComponent
        : public AZ::Component
        , private ShaderManagementConsoleWindowRequestBus::Handler
    {
    public:
        AZ_COMPONENT(ShaderManagementConsoleWindowComponent, "{03976F19-3C74-49FE-A15F-7D3CADBA616C}");

        static void Reflect(AZ::ReflectContext* context);

        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);

    private:
        // Temporary structure when generating shader variants.
        struct ShaderVariantListInfo
        {
            AZStd::string m_materialFileName;
            AZStd::vector<AZ::RPI::ShaderCollection::Item> m_shaderItems;
        };

        ////////////////////////////////////////////////////////////////////////
        // ShaderManagementConsoleWindowRequestBus::Handler overrides...
        void CreateShaderManagementConsoleWindow() override;
        void DestroyShaderManagementConsoleWindow() override;
        void GenerateShaderVariantListForShaderMaterials(const char* shaderFileName) override;
        ////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Init() override;
        void Activate() override;
        void Deactivate() override;
        ////////////////////////////////////////////////////////////////////////

        void GenerateShaderVariantForMaterials(AZStd::string_view destFilePath, AZStd::string_view shaderFilePath, const AZStd::vector<ShaderVariantListInfo>& shaderVariantListInfoList);

        AZStd::unique_ptr<ShaderManagementConsoleWindow> m_window;
        AZStd::unique_ptr<ShaderManagementConsoleBrowserInteractions> m_assetBrowserInteractions;
    };
}
