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

#include <AzCore/Component/TickBus.h>
#include <AzCore/Component/TransformBus.h>
#include <AtomCore/Instance/Instance.h>

#include <Atom/RPI.Public/Base.h>
#include <Atom/Document/MaterialDocumentNotificationBus.h>
#include <Atom/Feature/CoreLights/DirectionalLightFeatureProcessorInterface.h>
#include <Atom/Feature/SkyBox/SkyBoxFeatureProcessorInterface.h>
#include <Atom/Viewport/MaterialViewportNotificationBus.h>

#include <AzFramework/Windowing/WindowBus.h>
#include <Viewport/InputController/MaterialEditorViewportInputController.h>

namespace AZ
{
    class Entity;
    class Component;

    namespace RPI
    {
        class SwapChainPass;
        class WindowContext;
    }
}

namespace MaterialEditor
{
    //! Provides backend logic for MaterialViewport
    //! Sets up a scene, camera, loads the model, and applies texture
    class MaterialViewportRenderer
        : public AZ::Data::AssetBus::Handler
        , public AZ::TickBus::Handler
        , public MaterialDocumentNotificationBus::Handler
        , public MaterialViewportNotificationBus::Handler
        , public AZ::TransformNotificationBus::MultiHandler
        , public AzFramework::WindowSystemRequestBus::Handler
    {
    public:
        AZ_CLASS_ALLOCATOR(MaterialViewportRenderer, AZ::SystemAllocator, 0);

        MaterialViewportRenderer(AZStd::shared_ptr<AZ::RPI::WindowContext> windowContext);
        ~MaterialViewportRenderer();

        AZStd::shared_ptr<MaterialEditorViewportInputController> GetController();

    private:

        // MaterialDocumentNotificationBus::Handler interface overrides...
        void OnDocumentOpened(const AZ::Uuid& documentId) override;

        // MaterialViewportNotificationBus::Handler interface overrides...
        void OnLightingPresetSelected(AZ::Render::LightingPresetPtr preset) override;
        void OnLightingPresetChanged(AZ::Render::LightingPresetPtr preset) override;
        void OnModelPresetSelected(AZ::Render::ModelPresetPtr preset) override;
        void OnModelPresetChanged(AZ::Render::ModelPresetPtr preset) override;
        void OnShadowCatcherEnabledChanged(bool enable) override;
        void OnGridEnabledChanged(bool enable) override;

        // AZ::Data::AssetBus::Handler interface overrides...
        void OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset) override;

        // AZ::TickBus::Handler interface overrides...
        void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;

        // AZ::TransformNotificationBus::MultiHandler overrides...
        void OnTransformChanged(const AZ::Transform&, const AZ::Transform&) override;

        // AzFramework::WindowSystemRequestBus::Handler overrides ...
        AzFramework::NativeWindowHandle GetDefaultWindowHandle() override;

        using DirectionalLightHandle = AZ::Render::DirectionalLightFeatureProcessorInterface::LightHandle;

        AZ::Data::Instance<AZ::RPI::SwapChainPass> m_swapChainPass;
        AZStd::string m_defaultPipelineAssetPath = "passes/MainRenderPipeline.azasset";
        AZStd::shared_ptr<AZ::RPI::WindowContext> m_windowContext;
        AZ::RPI::RenderPipelinePtr m_renderPipeline;
        AZ::RPI::ScenePtr m_scene;
        AZ::Render::DirectionalLightFeatureProcessorInterface* m_directionalLightFeatureProcessor = nullptr;

        AZ::Entity* m_cameraEntity = nullptr;
        AZ::Component* m_cameraComponent = nullptr;
        bool m_cameraNeedsFullReset = true;

        AZ::Entity* m_postProcessEntity = nullptr;

        AZ::Entity* m_modelEntity = nullptr;
        AZ::Data::AssetId m_modelAssetId;

        AZ::Entity* m_gridEntity = nullptr;

        AZ::Entity* m_shadowCatcherEntity = nullptr;
        AZ::Data::Instance<AZ::RPI::Material> m_shadowCatcherMaterial;
        AZ::RPI::MaterialPropertyIndex m_shadowCatcherOpacityPropertyIndex;

        AZStd::vector<DirectionalLightHandle> m_lightHandles;

        AZ::Entity* m_iblEntity = nullptr;
        AZ::Render::SkyBoxFeatureProcessorInterface* m_skyboxFeatureProcessor = nullptr;

        float m_simulateTime = 0;

        AZStd::shared_ptr<MaterialEditorViewportInputController> m_viewportController;
    };
} // namespace MaterialEditor
