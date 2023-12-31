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

#include <AzCore/Math/Quaternion.h>
#include <Atom/RPI.Public/FeatureProcessor.h>
#include <Atom/RPI.Public/Shader/ShaderResourceGroup.h>
#include <Atom/RPI.Reflect/Image/StreamingImageAsset.h>
#include <Atom/Feature/ImageBasedLights/ImageBasedLightFeatureProcessorInterface.h>

namespace AZ
{
    namespace Render
    {
        //! This feature processor handles image based lights.
        class ImageBasedLightFeatureProcessor final
            : public ImageBasedLightFeatureProcessorInterface
        {
        public:
            AZ_RTTI(AZ::Render::ImageBasedLightFeatureProcessor, "{1206C38B-2143-4EE1-9C83-F876BD465BBB}", AZ::Render::ImageBasedLightFeatureProcessorInterface);

            static void Reflect(AZ::ReflectContext* context);

            ImageBasedLightFeatureProcessor() = default;
            virtual ~ImageBasedLightFeatureProcessor() = default;

            // FeatureProcessor overrides ...
            //! Creates pools, buffers, and buffer views
            void Activate() override;
            //! Releases GPU resources.
            void Deactivate() override;
            //! Updates the images for any ibls that changed.
            void Simulate(const FeatureProcessor::SimulatePacket& packet) override;

            void SetSpecularImage(const Data::Asset<RPI::StreamingImageAsset>& imageAsset) override;
            void SetDiffuseImage(const Data::Asset<RPI::StreamingImageAsset>& imageAsset) override;
            void SetExposure(float exposure) override;
            void SetOrientation(const Quaternion& orientation) override;
            void Reset() override;

        private:

            ImageBasedLightFeatureProcessor(const ImageBasedLightFeatureProcessor&) = delete;

            void LoadDefaultCubeMaps();
            static Data::Instance<RPI::Image> GetInstanceForImage(const Data::Asset<RPI::StreamingImageAsset>& imageAsset, const Data::Instance<RPI::Image>& defaultImage);
            static bool ValidateIsCubemap(Data::Instance<RPI::Image> image);

            Data::Instance<RPI::ShaderResourceGroup> m_sceneSrg;
            RHI::ShaderInputImageIndex m_specularEnvMapIndex;
            RHI::ShaderInputImageIndex m_diffuseEnvMapIndex;
            RHI::ShaderInputConstantIndex m_iblExposureConstantIndex;
            RHI::ShaderInputConstantIndex m_iblOrientationConstantIndex;

            Data::Instance<RPI::Image> m_specular;
            Data::Instance<RPI::Image> m_diffuse;
            Quaternion m_orientation = Quaternion::CreateIdentity();
            float m_exposure = 0;

            Data::Instance<RPI::Image> m_defaultSpecularImage;
            Data::Instance<RPI::Image> m_defaultDiffuseImage;
        };
    }
}
