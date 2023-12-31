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

#include <Atom/RPI.Public/Pass/ComputePass.h>
#include <Atom/Feature/PostProcess/Bloom/BloomConstants.h>

namespace AZ
{
    namespace Render
    {
        class ShaderResourceGroup;

        class BloomDownsamplePass
            : public RPI::ComputePass
        {
            AZ_RPI_PASS(BloomDownsamplePass);

        public:
            AZ_RTTI(BloomDownsamplePass, "{D1CA5F45-70DB-4130-B5FA-147EFB010B1F}", RenderPass);
            AZ_CLASS_ALLOCATOR(BloomDownsamplePass, SystemAllocator, 0);
            virtual ~BloomDownsamplePass() = default;

            //! Creates a BloomDownsamplePass
            static RPI::Ptr<BloomDownsamplePass> Create(const RPI::PassDescriptor& descriptor);

        protected:
            BloomDownsamplePass(const RPI::PassDescriptor& descriptor);

            // Pass Behaviour Overrides...
            void BuildAttachmentsInternal() override;
            void FrameBeginInternal(FramePrepareParams params) override;

            void BuildOutAttachmentBinding();
            AZ::Vector4 CalThresholdConstants();
            void FindShaderConstantInputIndex(AZ::RHI::ShaderInputConstantIndex& index, const char* nameStr);

            // output texture vertical dimension required by compute shader
            AZ::RHI::ShaderInputConstantIndex m_sourceImageTexelSizeInputIndex;
            AZ::RHI::ShaderInputConstantIndex m_thresholdConstantsInputIndex;

            float m_threshold = AZ::Render::Bloom::DefaultThreshold;
            float m_knee = AZ::Render::Bloom::DefaultKnee;

            bool m_srgNeedsUpdate = true;
        };
    }   // namespace RPI
}   // namespace AZ
