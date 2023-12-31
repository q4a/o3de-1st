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

#include <Atom/RPI.Public/Pass/Pass.h>
#include <Atom/RPI.Public/Pass/ParentPass.h>
#include <Atom/RPI.Public/Shader/ShaderResourceGroup.h>
#include <Atom/RPI.Public/Shader/Shader.h>

#include <Atom/RPI.Reflect/Pass/DownsampleMipChainPassData.h>
#include <Atom/Feature/PostProcess/Bloom/BloomConstants.h>

namespace AZ
{
    namespace Render
    {
        class BloomCompositeChildPass;

        // Pass for combining contributions of all downsampled images to the render target
        class BloomCompositePass
            : public RPI::ParentPass
        {
            AZ_RPI_PASS(BloomCompositePass);

        public:
            AZ_RTTI(BloomCompositePass, "{02E41C48-5CC7-4277-B66E-009E4D6D32BA}", ParentPass);
            AZ_CLASS_ALLOCATOR(BloomCompositePass, SystemAllocator, 0);
            virtual ~BloomCompositePass() = default;

            //! Creates a BloomCompositePass
            static RPI::Ptr<BloomCompositePass> Create(const RPI::PassDescriptor& descriptor);
        protected:
            BloomCompositePass(const RPI::PassDescriptor& descriptor);

            // Pass behaviour overrides...
            void BuildAttachmentsInternal() override;
            void FrameBeginInternal(FramePrepareParams params) override;

            void GetAttachmentInfo();
            void UpdateParameters();
            void BuildChildPasses();
            void UpdateChildren();
            void CreateBinding(BloomCompositeChildPass* pass, uint32_t mipLevel);

            RPI::DownsampleMipChainPassData m_passData;

            uint32_t m_inputWidth = 0;
            uint32_t m_inputHeight = 0;

            uint32_t m_outputWidth = 0;
            uint32_t m_outputHeight = 0;

            AZStd::vector<Vector3> m_tintData = {
                AZ::Render::Bloom::DefaultTint,
                AZ::Render::Bloom::DefaultTint,
                AZ::Render::Bloom::DefaultTint,
                AZ::Render::Bloom::DefaultTint,
                AZ::Render::Bloom::DefaultTint
            };

            float m_intensity = AZ::Render::Bloom::DefaultIntensity;

            bool m_enableBicubic = AZ::Render::Bloom::DefaultEnableBicubicFilter;
        };

        // Pass spawned by parent composite pass for additive upsampling
        class BloomCompositeChildPass
            : public RPI::ComputePass
        {
            AZ_RPI_PASS(BloomCompositeChildPass);

            friend class BloomBlurPass;

        public:
            AZ_RTTI(BloomCompositeChildPass, "{85D3FE9B-D347-40D6-B666-B4DF855F5B80}", RenderPass);
            AZ_CLASS_ALLOCATOR(BloomCompositeChildPass, SystemAllocator, 0);
            virtual ~BloomCompositeChildPass() = default;

            //! Creates a BloomCompositeChildPass
            static RPI::Ptr<BloomCompositeChildPass> Create(const RPI::PassDescriptor& descriptor);

            void UpdateParameters(uint32_t sourceMip, uint32_t sourceImageWidth, uint32_t sourceImageHeight, uint32_t targetImageWidth, uint32_t targetImageHeight, bool enableBicubic, Vector3 tint, float intensity);

        protected:
            BloomCompositeChildPass(const RPI::PassDescriptor& descriptor);

            // Pass Behaviour Overrides...
            void FrameBeginInternal(FramePrepareParams params) override;

            void FindShaderConstantInputIndex(AZ::RHI::ShaderInputConstantIndex& index, const char* name);

            AZ::RHI::ShaderInputConstantIndex m_intensityInputIndex;
            AZ::RHI::ShaderInputConstantIndex m_sourceImageSizeInputIndex;
            AZ::RHI::ShaderInputConstantIndex m_sourceImageTexelSizeInputIndex;
            AZ::RHI::ShaderInputConstantIndex m_targetImageSizeInputIndex;
            AZ::RHI::ShaderInputConstantIndex m_sourceMipLevelInputIndex;
            AZ::RHI::ShaderInputConstantIndex m_enableBicubicInputIndex;
            AZ::RHI::ShaderInputConstantIndex m_tintInputIndex;

            uint32_t m_targetImageWidth;
            uint32_t m_targetImageHeight;
        };
    }   // namespace RPI
}   // namespace AZ
