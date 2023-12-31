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

#include <AzCore/Memory/SystemAllocator.h>
#include <Atom/RPI.Public/Pass/FullscreenTrianglePass.h>
#include <Atom/Feature/ACES/Aces.h>
#include <Atom/Feature/ACES/AcesDisplayMapperFeatureProcessor.h>
#include <Atom/Feature/DisplayMapper/DisplayMapperFullScreenPass.h>

namespace AZ
{
    namespace Render
    {
        /**
         * This pass is used to apply a shaper function and lookup table to the input image attachment.
         * The coordinates on the lookup table are computed by taking the color values of the input image
         * and translating it via a shaper function.
         * Right now, preset shaper functions based of ACES 1.0.3 are being used.
         */
        class ApplyShaperLookupTablePass
            : public DisplayMapperFullScreenPass
        {
            AZ_RPI_PASS(AcesOutputTransformLutPass);

        public:
            AZ_RTTI(ApplyShaperLookupTablePass, "{5C76BE12-307A-4595-91CE-AAA13ED6368C}", DisplayMapperFullScreenPass);
            AZ_CLASS_ALLOCATOR(ApplyShaperLookupTablePass, SystemAllocator, 0);
            virtual ~ApplyShaperLookupTablePass();

            /// Creates a AcesOutputTransformLutPass
            static RPI::Ptr<ApplyShaperLookupTablePass> Create(const RPI::PassDescriptor& descriptor);

            void SetShaperParameters(const ShaperParams& shaperParams);
            void SetLutAssetId(const AZ::Data::AssetId& assetId);

        protected:
            explicit ApplyShaperLookupTablePass(const RPI::PassDescriptor& descriptor);
            void Init() override;

            RHI::ShaderInputImageIndex m_shaderInputLutImageIndex;

            RHI::ShaderInputConstantIndex m_shaderShaperTypeIndex;
            RHI::ShaderInputConstantIndex m_shaderShaperBiasIndex;
            RHI::ShaderInputConstantIndex m_shaderShaperScaleIndex;

        private:

            // Scope producer functions...
            void SetupFrameGraphDependencies(RHI::FrameGraphInterface frameGraph, const RPI::PassScopeProducer& producer) override;
            void CompileResources(const RHI::FrameGraphCompileContext& context, const RPI::PassScopeProducer& producer) override;

            void UpdateShaperSrg();
            void AcquireLutImage();
            void ReleaseLutImage();

            DisplayMapperAssetLut       m_lutResource;
            AZ::Data::AssetId           m_lutAssetId;

            ShaperParams m_shaperParams;

            bool m_needToReloadLut = true;
        };
    }   // namespace Render
}   // namespace AZ
