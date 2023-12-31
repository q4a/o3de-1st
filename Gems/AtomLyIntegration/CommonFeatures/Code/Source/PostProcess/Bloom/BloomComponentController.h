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
#include <AzCore/Component/TransformBus.h>

#include <AtomLyIntegration/CommonFeatures/PostProcess/Bloom/BloomBus.h>
#include <AtomLyIntegration/CommonFeatures/PostProcess/Bloom/BloomComponentConfig.h>

#include <Atom/Feature/PostProcess/Bloom/BloomConstants.h>
#include <Atom/Feature/PostProcess/Bloom/BloomSettingsInterface.h>
#include <Atom/Feature/PostProcess/PostProcessSettingsInterface.h>
#include <Atom/Feature/PostProcess/PostProcessFeatureProcessorInterface.h>

namespace AZ
{
    namespace Render
    {
        class BloomComponentController final
            : public BloomRequestBus::Handler
        {
        public:
            friend class EditorBloomComponent;

            AZ_TYPE_INFO(AZ::Render::BloomComponentController, "{502896C1-FF04-4BA7-833B-BA80946FA0DD}");
            static void Reflect(AZ::ReflectContext* context);
            static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
            static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
            static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);

            BloomComponentController() = default;
            BloomComponentController(const BloomComponentConfig& config);

            void Activate(EntityId entityId);
            void Deactivate();
            void SetConfiguration(const BloomComponentConfig& config);
            const BloomComponentConfig& GetConfiguration() const;

            // Auto-gen function override declarations (functions definitions in .cpp)...
#include <Atom/Feature/ParamMacros/StartParamFunctionsOverride.inl>
#include <Atom/Feature/PostProcess/Bloom/BloomParams.inl>
#include <Atom/Feature/ParamMacros/EndParams.inl>

        private:
            AZ_DISABLE_COPY(BloomComponentController);

            void OnConfigChanged();

            PostProcessSettingsInterface* m_postProcessInterface = nullptr;
            BloomSettingsInterface* m_settingsInterface = nullptr;
            BloomComponentConfig m_configuration;
            EntityId m_entityId;
        };
    }
}
