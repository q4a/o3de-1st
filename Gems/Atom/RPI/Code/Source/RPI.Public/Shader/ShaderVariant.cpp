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
#include <Atom/RPI.Public/Shader/ShaderVariant.h>

#include <Atom/RHI/DrawListTagRegistry.h>
#include <Atom/RHI/RHISystemInterface.h>

#include <Atom/RHI.Reflect/ShaderStageFunction.h>

namespace AZ
{
    namespace RPI
    {
        bool ShaderVariant::Init(
            const ShaderAsset& shaderAsset,
            Data::Asset<ShaderVariantAsset> shaderVariantAsset)
        {            
            m_pipelineStateType = shaderAsset.GetPipelineStateType();
            m_pipelineLayoutDescriptor = shaderAsset.GetPipelineLayoutDescriptor();
            m_shaderVariantAsset = shaderVariantAsset;
            return true;
        }

        void ShaderVariant::ConfigurePipelineState(RHI::PipelineStateDescriptor& descriptor) const
        {
            descriptor.m_pipelineLayoutDescriptor = m_pipelineLayoutDescriptor;

            switch (descriptor.GetType())
            {
            case RHI::PipelineStateType::Draw:
            {
                AZ_Assert(m_pipelineStateType == RHI::PipelineStateType::Draw, "ShaderVariant is not intended for the raster pipeline.");
                RHI::PipelineStateDescriptorForDraw& descriptorForDraw = static_cast<RHI::PipelineStateDescriptorForDraw&>(descriptor);
                descriptorForDraw.m_vertexFunction = m_shaderVariantAsset->GetShaderStageFunction(RHI::ShaderStage::Vertex);
                descriptorForDraw.m_tessellationFunction = m_shaderVariantAsset->GetShaderStageFunction(RHI::ShaderStage::Tessellation);
                descriptorForDraw.m_fragmentFunction = m_shaderVariantAsset->GetShaderStageFunction(RHI::ShaderStage::Fragment);
                descriptorForDraw.m_renderStates = m_shaderVariantAsset->GetRenderStates();
                break;
            }

            case RHI::PipelineStateType::Dispatch:
            {
                AZ_Assert(m_pipelineStateType == RHI::PipelineStateType::Dispatch, "ShaderVariant is not intended for the compute pipeline.");
                RHI::PipelineStateDescriptorForDispatch& descriptorForDispatch = static_cast<RHI::PipelineStateDescriptorForDispatch&>(descriptor);
                descriptorForDispatch.m_computeFunction = m_shaderVariantAsset->GetShaderStageFunction(RHI::ShaderStage::Compute);
                break;
            }

            case RHI::PipelineStateType::RayTracing:
            {
                AZ_Assert(m_pipelineStateType == RHI::PipelineStateType::RayTracing, "ShaderVariant is not intended for the ray tracing pipeline.");
                RHI::PipelineStateDescriptorForRayTracing& descriptorForRayTracing = static_cast<RHI::PipelineStateDescriptorForRayTracing&>(descriptor);
                descriptorForRayTracing.m_rayTracingFunction = m_shaderVariantAsset->GetShaderStageFunction(RHI::ShaderStage::RayTracing);
                break;
            }

            default:
                AZ_Assert(false, "Unexpected PipelineStateType");
                break;
            }
        }

        const ShaderInputContract& ShaderVariant::GetInputContract() const
        {
            return m_shaderVariantAsset->GetInputContract();
        }

        const ShaderOutputContract& ShaderVariant::GetOutputContract() const
        {
            return m_shaderVariantAsset->GetOutputContract();
        }
    } // namespace RPI
} // namespace AZ
