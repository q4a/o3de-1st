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

#include <Atom/RHI/PipelineState.h>
#include <Atom/RPI.Public/Pass/ComputePass.h>
#include <CoreLights/Shadow.h>

namespace AZ
{
    namespace Render
    {
        //! DepthExponentiationPass output exponential of depth for ESM filering.
        class DepthExponentiationPass final
            : public RPI::ComputePass
        {
            using Base = RPI::ComputePass;
            AZ_RPI_PASS(DepthExponentiationPass);

        public:
            AZ_RTTI(DepthExponentiationPass, "9B91DE5C-0842-4CF8-AA30-B024277E0FAB", Base);
            AZ_CLASS_ALLOCATOR(DepthExponentiationPass, SystemAllocator, 0);

            ~DepthExponentiationPass() = default;
            static RPI::Ptr<DepthExponentiationPass> Create(const RPI::PassDescriptor& descriptor);

            //! This sets the kind of shadowmaps.
            void SetShadowmapType(Shadow::ShadowmapType type);

            //! This returns the shadowmap type of this pass.
            Shadow::ShadowmapType GetShadowmapType() const;

        private:
            struct ShaderVariantInfo
            {
                const bool m_isFullyBaked = false;
                const RHI::PipelineState* m_pipelineState = nullptr;
            };

            DepthExponentiationPass() = delete;
            explicit DepthExponentiationPass(const RPI::PassDescriptor& descriptor);

            // RPI::Pass overrides...
            void FrameBeginInternal(FramePrepareParams params) override;
            void CompileResources(const RHI::FrameGraphCompileContext& context, const RPI::PassScopeProducer& producer) override;
            void BuildCommandList(const RHI::FrameGraphExecuteContext& context, const RPI::PassScopeProducer& producer) override;

            void InitializeShaderOption();

            const Name m_optionName;
            const AZStd::vector<Name> m_optionValues;

            Shadow::ShadowmapType m_shadowmapType = Shadow::ShadowmapType::Directional;
            AZStd::vector<ShaderVariantInfo> m_shaderVariant;
            RPI::ShaderVariantKey m_currentShaderVarationKeyFallbackValue;

            bool m_shaderOptionInitialized = false;
            bool m_shaderVariantNeedsUpdate = false;
        };
    } // namespace Render
} // namespace AZ
