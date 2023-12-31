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

#include <CoreLights/DirectionalLightComponent.h>
#include <Atom/Feature/Utils/EditorRenderComponentAdapter.h>
#include <AtomLyIntegration/CommonFeatures/CoreLights/DirectionalLightComponentConfig.h>
#include <AzFramework/Entity/EntityDebugDisplayBus.h>

namespace AZ
{
    namespace Render
    {
        class EditorDirectionalLightComponent final
            : public EditorRenderComponentAdapter<DirectionalLightComponentController, DirectionalLightComponent, DirectionalLightComponentConfig>
            , private AzFramework::EntityDebugDisplayEventBus::Handler
        {
        public:

            using BaseClass = EditorRenderComponentAdapter<DirectionalLightComponentController, DirectionalLightComponent, DirectionalLightComponentConfig>;
            AZ_EDITOR_COMPONENT(AZ::Render::EditorDirectionalLightComponent, EditorDirectionalLightComponentTypeId, BaseClass);

            static void Reflect(ReflectContext* context);

            EditorDirectionalLightComponent() = default;
            EditorDirectionalLightComponent(const DirectionalLightComponentConfig& config);

            void Activate() override;
            void Deactivate() override;

            //! EditorRenderComponentAdapter overrides...
            AZ::u32 OnConfigurationChanged() override;

            //! AzFramework::EntityDebugDisplayEventBus::Handler overrides...
            void DisplayEntityViewport(
                const AzFramework::ViewportInfo& viewportInfo,
                AzFramework::DebugDisplayRequests& debugDisplay) override;
        };

    } // namespace Render
} // namespace AZ
