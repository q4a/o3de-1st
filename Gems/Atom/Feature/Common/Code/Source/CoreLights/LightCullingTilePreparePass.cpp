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

#include <CoreLights/LightCullingTilePreparePass.h>

#include <Atom/RHI/Factory.h>
#include <Atom/RHI/PipelineState.h>
#include <Atom/RPI.Public/Pass/PassUtils.h>
#include <Atom/RPI.Public/RenderPipeline.h>
#include <Atom/RPI.Public/RPIUtils.h>
#include <Atom/RHI/RHISystemInterface.h>
#include <Atom/RPI.Reflect/Pass/PassTemplate.h>
#include <Atom/RPI.Public/View.h>

namespace AZ
{
    namespace Render
    {
        RPI::Ptr<LightCullingTilePreparePass> LightCullingTilePreparePass::Create(const RPI::PassDescriptor& descriptor)
        {
            RPI::Ptr<LightCullingTilePreparePass> pass = aznew LightCullingTilePreparePass(descriptor);
            return pass;
        }

        LightCullingTilePreparePass::LightCullingTilePreparePass(const RPI::PassDescriptor& descriptor)
            : RPI::ComputePass(descriptor)
            , m_msaaNoneName("MsaaMode::None")
            , m_msaaMode2xName("MsaaMode::Msaa2x")
            , m_msaaMode4xName("MsaaMode::Msaa4x")
            , m_msaaMode8xName("MsaaMode::Msaa8x")
            , m_msaaOptionName("o_msaaMode")
            , m_constantDataName("m_constantData")
        {
        }

        void LightCullingTilePreparePass::CompileResources(const RHI::FrameGraphCompileContext& context, const RPI::PassScopeProducer& producer)
        {
            SetConstantData();
            ComputePass::CompileResources(context, producer);
        }

        void LightCullingTilePreparePass::BuildCommandList(const RHI::FrameGraphExecuteContext& context, [[maybe_unused]] const RPI::PassScopeProducer& producer)
        {
            // Dispatch one compute shader thread per depth buffer pixel. These threads are divided into thread-groups that analyze one tile. (Typically 16x16 pixel tiles)
            RHI::CommandList* commandList = context.GetCommandList();
            SetSrgsForDispatch(commandList);

            RHI::Size resolution = GetDepthBufferDimensions();

            m_dispatchItem.m_arguments.m_direct.m_totalNumberOfThreadsX = resolution.m_width;
            m_dispatchItem.m_arguments.m_direct.m_totalNumberOfThreadsY = resolution.m_height;
            m_dispatchItem.m_arguments.m_direct.m_totalNumberOfThreadsZ = 1;
            m_dispatchItem.m_pipelineState = m_msaaPipelineState.get();
            commandList->Submit(m_dispatchItem);
        }

        AZ::RHI::Size LightCullingTilePreparePass::GetDepthBufferDimensions()
        {
            AZ_Assert(GetInputBinding(0).m_name == AZ::Name("Depth"), "LightCullingTilePrepare: Expecting slot 0 to be the depth buffer");

            const RPI::PassAttachment* attachment = GetInputBinding(0).m_attachment.get();
            return attachment->m_descriptor.m_image.m_size;
        }

        AZStd::array<float, 2> LightCullingTilePreparePass::ComputeUnprojectConstants() const
        {
            AZStd::array<float, 2> unprojectConstants;
            const auto& view = m_pipeline->GetDefaultView();

            // Our view to clip matrix is right-hand and column major
            // i.e. something like this:
            // [- -  - -][x]
            // [- -  - -][y]
            // [0 0  A B][z]
            // [0 0 -1 0][1]
            // To unproject from depth buffer to Z, we want to pack the A and B variables into a constant buffer:
            unprojectConstants[0] = (-view->GetViewToClipMatrix().GetRow(2).GetElement(3));
            unprojectConstants[1] = (view->GetViewToClipMatrix().GetRow(2).GetElement(2));
            return unprojectConstants;
        }

        void LightCullingTilePreparePass::ChooseShaderVariant()
        {
            const AZ::RPI::ShaderVariant& shaderVariant = CreateShaderVariant();
            CreatePipelineStateFromShaderVariant(shaderVariant);
        }

        AZ::Name LightCullingTilePreparePass::GetMultiSampleName()
        {
            const RHI::MultisampleState msaa = GetMultiSampleState();
            switch (msaa.m_samples)
            {
            case 1:
                return m_msaaNoneName;
            case 2:
                return m_msaaMode2xName;
            case 4:
                return m_msaaMode4xName;
            case 8:
                return m_msaaMode8xName;
            default:
                AZ_Error("LightCullingTilePreparePass", false, "Unhandled number of Msaa samples: %u", msaa.m_samples);
                return m_msaaNoneName;
            }
        }

        AZ::RHI::MultisampleState LightCullingTilePreparePass::GetMultiSampleState()
        {
            AZ_Assert(GetInputBinding(0).m_name == AZ::Name("Depth"), "LightCullingTilePrepare: Expecting slot 0 to be the depth buffer");

            const RPI::PassAttachment* attachment = GetInputBinding(0).m_attachment.get();
            return attachment->m_descriptor.m_image.m_multisampleState;
        }

        AZ::RPI::ShaderOptionGroup LightCullingTilePreparePass::CreateShaderOptionGroup()
        {
            RPI::ShaderOptionGroup shaderOptionGroup = m_shader->CreateShaderOptionGroup();
            shaderOptionGroup.SetUnspecifiedToDefaultValues();
            shaderOptionGroup.SetValue(m_msaaOptionName, GetMultiSampleName());
            return shaderOptionGroup;
        }

        void LightCullingTilePreparePass::CreatePipelineStateFromShaderVariant(const RPI::ShaderVariant& shaderVariant)
        {
            AZ::RHI::PipelineStateDescriptorForDispatch pipelineStateDescriptor;
            shaderVariant.ConfigurePipelineState(pipelineStateDescriptor);
            m_msaaPipelineState = m_shader->AcquirePipelineState(pipelineStateDescriptor);
        }

        const AZ::RPI::ShaderVariant& LightCullingTilePreparePass::CreateShaderVariant()
        {
            RPI::ShaderOptionGroup shaderOptionGroup = CreateShaderOptionGroup();
            RPI::ShaderVariantSearchResult findVariantResult = m_shader->FindVariantStableId(shaderOptionGroup.GetShaderVariantId());
            const RPI::ShaderVariant& shaderVariant = m_shader->GetVariant(findVariantResult.GetStableId());

            //Set the fallbackkey
            if (m_drawSrg)
            {
                m_drawSrg->SetShaderVariantKeyFallbackValue(shaderOptionGroup.GetShaderVariantKeyFallbackValue());
            }
            return shaderVariant;
        }

        void LightCullingTilePreparePass::SetConstantData()
        {
            struct ConstantData
            {
                AZStd::array<float, 2> m_unprojectZ;
                uint32_t depthBufferWidth;
                uint32_t depthBufferHeight;
            } constantData;

            const RHI::Size resolution = GetDepthBufferDimensions();
            constantData.m_unprojectZ = ComputeUnprojectConstants();
            constantData.depthBufferWidth = resolution.m_width;
            constantData.depthBufferHeight = resolution.m_height;

            bool setOk = m_shaderResourceGroup->SetConstant(m_constantDataIndex, constantData);
            AZ_Assert(setOk, "LightCullingTilePreparePass::SetConstantData() - could not set constant data");
        }

        void LightCullingTilePreparePass::BuildAttachmentsInternal() 
        {
            ChooseShaderVariant();
            m_constantDataIndex = m_shaderResourceGroup->FindShaderInputConstantIndex(m_constantDataName);
            AZ_Assert(m_constantDataIndex.IsValid(), "m_constantData not found in shader");
        }

    }   // namespace Render
}   // namespace AZ
