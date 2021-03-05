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

#include <AzCore/Memory/Memory.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzCore/std/string/string.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Color.h>
#include <Atom/Feature/CoreLights/ShadowConstants.h>
#include <Atom/RPI.Reflect/Image/StreamingImageAsset.h>
#include <Atom/Feature/CoreLights/DirectionalLightFeatureProcessorInterface.h>
#include <Atom/RPI.Public/Material/Material.h>

namespace AZ
{
    namespace Render
    {
        class ImageBasedLightFeatureProcessorInterface;
        class SkyBoxFeatureProcessorInterface;
        class ExposureControlSettingsInterface;

        //! ExposureControlConfig describes exposure settings that can be added to a LightingPreset
        struct ExposureControlConfig final
        {
            AZ_TYPE_INFO(AZ::Render::ExposureControlConfig, "{C6FD75F7-58BA-46CE-8FBA-2D64CB4ECFF9}");
            static void Reflect(AZ::ReflectContext* context);

            // Exposure Control Constants...
            enum class ExposureControlType :uint32_t
            {
                ManualOnly = 0,
                EyeAdaptation,
                ExposureControlTypeMax
            };

            uint32_t m_exposureControlType = 0;
            float m_manualCompensationValue = 0.0f;
            float m_autoExposureMin = -10.0;
            float m_autoExposureMax = 10.0f;
            float m_autoExposureSpeedUp = 3.0;
            float m_autoExposureSpeedDown = 1.0;
        };

        //! LightConfig describes a directional light that can be added to a LightingPreset
        struct LightConfig final
        {
            AZ_TYPE_INFO(AZ::Render::LightConfig, "{02644F52-9483-47A8-9028-37671695C34E}");
            static void Reflect(AZ::ReflectContext* context);

            AZ::Vector3 m_direction = AZ::Vector3::CreateAxisY();
            AZ::Color m_color = AZ::Color::CreateOne();
            float m_intensity = 1.0f;
            uint16_t m_shadowCascadeCount = 1;
            float m_shadowRatioLogarithmUniform = 0.5f;
            float m_shadowFarClipDistance = 20.0f;
            ShadowmapSize m_shadowmapSize = ShadowmapSize::Size1024;
            bool m_enableShadowDebugColoring = false;
        };

        //! LightingPreset describes a lighting environment that can be applied to the viewport
        struct LightingPreset final
        {
            AZ_TYPE_INFO(AZ::Render::LightingPreset, "{6EEACBC0-2D97-414C-8E87-088E7BA231A9}");
            AZ_CLASS_ALLOCATOR(LightingPreset, AZ::SystemAllocator, 0);
            static void Reflect(AZ::ReflectContext* context);

            bool m_autoSelect = false;
            AZStd::string m_displayName;
            AZ::Data::Asset<AZ::RPI::StreamingImageAsset> m_skyboxImageAsset;
            AZ::Data::Asset<AZ::RPI::StreamingImageAsset> m_iblSpecularImageAsset;
            AZ::Data::Asset<AZ::RPI::StreamingImageAsset> m_iblDiffuseImageAsset;
            float m_iblExposure = 0.0f;
            float m_skyboxExposure = 0.0f;
            ExposureControlConfig m_exposure;
            AZStd::vector<LightConfig> m_lights;
            float m_shadowCatcherOpacity = 0.5f;

            // Apply the lighting config to the currect scene through feature processors.
            // Shader catcher material is optional.
            void ApplyLightingPreset(
                ImageBasedLightFeatureProcessorInterface* iblFeatureProcessor,
                SkyBoxFeatureProcessorInterface* skyboxFeatureProcessor,
                ExposureControlSettingsInterface* exposureControlSettingsInterface,
                DirectionalLightFeatureProcessorInterface* directionalLightFeatureProcessor,
                const Camera::Configuration& cameraConfig,
                AZStd::vector<DirectionalLightFeatureProcessorInterface::LightHandle>& lightHandles,
                Data::Instance<AZ::RPI::Material> shadowCatcherMaterial = nullptr,
                RPI::MaterialPropertyIndex shadowCatcherOpacityPropertyIndex = RPI::MaterialPropertyIndex()) const;
        };

        using LightingPresetPtr = AZStd::shared_ptr<LightingPreset>;
        using LightingPresetPtrVector = AZStd::vector<LightingPresetPtr>;
    } // namespace Render
} // namespace AZ