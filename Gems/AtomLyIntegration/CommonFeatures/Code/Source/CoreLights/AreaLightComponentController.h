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

#include <AzCore/Component/EntityBus.h>
#include <AzCore/Component/Component.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/Math/Color.h>

#include <AzFramework/Entity/EntityDebugDisplayBus.h>

#include <AtomLyIntegration/CommonFeatures/CoreLights/AreaLightBus.h>
#include <AtomLyIntegration/CommonFeatures/CoreLights/AreaLightComponentConfig.h>

#include <CoreLights/LightDelegateInterface.h>

namespace AZ
{
    namespace Render
    {
        class AreaLightComponentController final
            : private AreaLightRequestBus::Handler
        {
        public:
            friend class EditorAreaLightComponent;

            AZ_TYPE_INFO(AZ::Render::AreaLightComponentController, "{C185C0F7-0923-4EF7-94F7-B41D60FE535B}");
            static void Reflect(AZ::ReflectContext* context);
            static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
            static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
            static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);

            AreaLightComponentController() = default;
            AreaLightComponentController(const AreaLightComponentConfig& config);

            void Activate(EntityId entityId);
            void Deactivate();
            void SetConfiguration(const AreaLightComponentConfig& config);
            const AreaLightComponentConfig& GetConfiguration() const;

            //! Used by the editor to control visibility - the controller must remain active while invisible to handle light unit conversions.
            void SetVisibiliy(bool isVisible);

        private:

            AZ_DISABLE_COPY(AreaLightComponentController);

            // AreaLightRequestBus::Handler overrides ...
            const Color& GetColor() const override;
            void SetColor(const Color& color) override;
            bool GetLightEmitsBothDirections() const override;
            void SetLightEmitsBothDirections(bool value) override;
            bool GetUseFastApproximation() const override;
            void SetUseFastApproximation(bool useFastApproximation) override;
            PhotometricUnit GetIntensityMode() const override;
            float GetIntensity() const override;
            void SetIntensity(float intensity, PhotometricUnit intensityMode) override;
            void SetIntensity(float intensity) override;
            float GetAttenuationRadius() const override;
            void SetAttenuationRadius(float radius) override;
            void SetAttenuationRadiusMode(LightAttenuationRadiusMode attenuationRadiusMode) override;
            void ConvertToIntensityMode(PhotometricUnit intensityMode) override;

            void HandleDisplayEntityViewport(
                const AzFramework::ViewportInfo& viewportInfo,
                AzFramework::DebugDisplayRequests& debugDisplay,
                bool isSelected);

            void ConfigurationChanged();
            void IntensityChanged();
            void ChromaChanged();
            void AttenuationRadiusChanged();
            
            //! Handles calculating the attenuation radius when LightAttenuationRadiusMode is auto
            void AutoCalculateAttenuationRadius();
            //! Handles creating the light shape delegate. Separate function to allow for early returns once the correct shape inteface is found.
            void CreateLightShapeDelegate();

            AZStd::unique_ptr<LightDelegateInterface> m_lightShapeDelegate;
            AreaLightComponentConfig m_configuration;
            EntityId m_entityId;
            bool m_isVisible = true;
        };

    } // namespace Render
} // AZ namespace
