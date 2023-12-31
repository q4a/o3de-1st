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

#include <CoreLights/DepthExponentiationPass.h>

#include <Atom/RHI/PipelineStateDescriptor.h>
#include <Atom/RPI.Reflect/Shader/ShaderOptionGroup.h>

namespace AZ
{
    namespace Render
    {
        DepthExponentiationPass::DepthExponentiationPass(const RPI::PassDescriptor& descriptor)
            : Base(descriptor)
            , m_optionName("o_shadowmapLightType")
            , m_optionValues
            {
                Name("ShadowmapLightType::Directional"),
                Name("ShadowmapLightType::Spot")
            }
        {
        }

        RPI::Ptr<DepthExponentiationPass> DepthExponentiationPass::Create(const RPI::PassDescriptor& descriptor)
        {
            return aznew DepthExponentiationPass(descriptor);
        }

        void DepthExponentiationPass::SetShadowmapType(Shadow::ShadowmapType type)
        {
            if (!m_shaderOptionInitialized)
            {
                InitializeShaderOption();
                m_shaderOptionInitialized = true;
            }

            m_shadowmapType = type;
            const uint32_t typeIndex = aznumeric_cast<uint32_t>(type);
            RPI::ShaderOptionGroup shaderOption = m_shader->CreateShaderOptionGroup();
            shaderOption.SetValue(m_optionName, m_optionValues[typeIndex]);

            if (m_shaderResourceGroup)
            {
                m_shaderResourceGroup->SetShaderVariantKeyFallbackValue(shaderOption.GetShaderVariantKeyFallbackValue());
            }
        }

        Shadow::ShadowmapType DepthExponentiationPass::GetShadowmapType() const
        {
            return m_shadowmapType;
        }

        void DepthExponentiationPass::FrameBeginInternal(FramePrepareParams params)
        {
            if (!m_shaderOptionInitialized)
            {
                InitializeShaderOption();
                m_shaderOptionInitialized = true;
            }

            Base::FrameBeginInternal(params);
        }

        void DepthExponentiationPass::CompileResources(const RHI::FrameGraphCompileContext& context, const RPI::PassScopeProducer& producer)
        {
            if (m_shaderVariantNeedsUpdate)
            {
                SetShadowmapType(m_shadowmapType);
                m_shaderVariantNeedsUpdate = false;
            }

            Base::CompileResources(context, producer);
        }

        void DepthExponentiationPass::BuildCommandList(const RHI::FrameGraphExecuteContext& context, const RPI::PassScopeProducer& producer)
        {
            const uint32_t typeIndex = aznumeric_cast<uint32_t>(m_shadowmapType);
            m_dispatchItem.m_pipelineState = m_shaderVariant[typeIndex].m_pipelineState;

            Base::BuildCommandList(context, producer);
        }

        void DepthExponentiationPass::InitializeShaderOption()
        {
            AZ_Assert(m_shader != nullptr, "DepthExponentiationPass %s has a null shader when calling FrameBeginInternal.", GetPathName().GetCStr());

            // Caching all pipeline state for each shader variation for performance reason.
            m_shaderVariant.clear();
            for (const Name& valueName : m_optionValues)
            {
                RPI::ShaderOptionGroup shaderOption = m_shader->CreateShaderOptionGroup();
                shaderOption.SetValue(m_optionName, valueName);

                RPI::ShaderVariantSearchResult searchResult = m_shader->FindVariantStableId(shaderOption.GetShaderVariantId());
                RPI::ShaderVariant shaderVariant = m_shader->GetVariant(searchResult.GetStableId());

                RHI::PipelineStateDescriptorForDispatch pipelineStateDescriptor;
                shaderVariant.ConfigurePipelineState(pipelineStateDescriptor);

                ShaderVariantInfo variationInfo{
                    searchResult.IsFullyBaked(),
                    m_shader->AcquirePipelineState(pipelineStateDescriptor)
                };
                m_shaderVariant.push_back(AZStd::move(variationInfo));
            }
            m_shaderVariantNeedsUpdate = true;
        }

    } // namespace Rendere
} // namespace AZ
