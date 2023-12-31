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

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Asset/AssetManagerBus.h>

#include <Atom/RHI/CommandList.h>
#include <Atom/RHI/Factory.h>
#include <Atom/RHI/FrameScheduler.h>
#include <Atom/RHI/PipelineState.h>

#include <Atom/RPI.Reflect/Pass/ComputePassData.h>
#include <Atom/RPI.Reflect/Pass/PassTemplate.h>
#include <Atom/RPI.Reflect/Shader/ShaderAsset.h>

#include <Atom/RPI.Public/Pass/ComputePass.h>
#include <Atom/RPI.Public/Pass/PassUtils.h>
#include <Atom/RPI.Public/RPIUtils.h>

#include <Atom/RPI.Public/Shader/Shader.h>
#include <Atom/RPI.Public/Shader/ShaderResourceGroup.h>

namespace AZ
{
    namespace RPI
    {
        ComputePass::~ComputePass()
        {
            ShaderReloadNotificationBus::Handler::BusDisconnect();
        }

        Ptr<ComputePass> ComputePass::Create(const PassDescriptor& descriptor)
        {
            Ptr<ComputePass> pass = aznew ComputePass(descriptor);
            return pass;
        }

        ComputePass::ComputePass(const PassDescriptor& descriptor)
            : RenderPass(descriptor)
            , m_passDescriptor(descriptor)
        {
            LoadShader();
        }

        void ComputePass::LoadShader()
        {
            // Load ComputePassData...
            const ComputePassData* passData = PassUtils::GetPassData<ComputePassData>(m_passDescriptor);
            if (passData == nullptr)
            {
                AZ_Error("PassSystem", false, "[ComputePass '%s']: Trying to construct without valid ComputePassData!",
                    GetPathName().GetCStr());
                return;
            }

            // Load Shader
            Data::Asset<ShaderAsset> shaderAsset;
            if (passData->m_shaderReference.m_assetId.IsValid())
            {
                shaderAsset = RPI::FindShaderAsset(passData->m_shaderReference.m_assetId, passData->m_shaderReference.m_filePath);
            }

            if (!shaderAsset.GetId().IsValid())
            {
                AZ_Error("PassSystem", false, "[ComputePass '%s']: Failed to load shader '%s'!",
                    GetPathName().GetCStr(),
                    passData->m_shaderReference.m_filePath.data());
                return;
            }

            m_shader = Shader::FindOrCreate(shaderAsset);
            if (m_shader == nullptr)
            {
                AZ_Error("PassSystem", false, "[ComputePass '%s']: Failed to load shader '%s'!",
                    GetPathName().GetCStr(),
                    passData->m_shaderReference.m_filePath.data());
                return;
            }

            // Load Pass SRG...
            const Data::Asset<ShaderResourceGroupAsset>& passSrgAsset = m_shader->FindShaderResourceGroupAsset(SrgBindingSlot::Pass);
            if (passSrgAsset)
            {
                m_shaderResourceGroup = ShaderResourceGroup::Create(passSrgAsset);

                AZ_Assert(m_shaderResourceGroup, "[ComputePass '%s']: Failed to create SRG from shader asset '%s'",
                    GetPathName().GetCStr(),
                    passData->m_shaderReference.m_filePath.data());

                PassUtils::BindDataMappingsToSrg(m_passDescriptor, m_shaderResourceGroup.get());
            }

            // Load Draw SRG...
            const Data::Asset<ShaderResourceGroupAsset>& drawSrgAsset = m_shader->FindShaderResourceGroupAsset(SrgBindingSlot::Draw);
            if (drawSrgAsset)
            {
                m_drawSrg = ShaderResourceGroup::Create(drawSrgAsset);
            }

            RHI::DispatchDirect dispatchArgs;
            dispatchArgs.m_totalNumberOfThreadsX = passData->m_totalNumberOfThreadsX;
            dispatchArgs.m_totalNumberOfThreadsY = passData->m_totalNumberOfThreadsY;
            dispatchArgs.m_totalNumberOfThreadsZ = passData->m_totalNumberOfThreadsZ;

            const auto numThreads = m_shader->GetAsset()->GetAttribute(RHI::ShaderStage::Compute, Name{ "numthreads" });
            if (numThreads)
            {
                const RHI::ShaderStageAttributeArguments& args = *numThreads;
                bool validArgs = args.size() == 3;
                if (validArgs)
                {
                    validArgs &= args[0].type() == azrtti_typeid<int>();
                    validArgs &= args[1].type() == azrtti_typeid<int>();
                    validArgs &= args[2].type() == azrtti_typeid<int>();
                }

                if (!validArgs)
                {
                    AZ_Error("PassSystem", false, "[ComputePass '%s']: Shader '%s' contains invalid numthreads arguments.",
                        GetPathName().GetCStr(),
                        passData->m_shaderReference.m_filePath.data());
                    return;
                }

                dispatchArgs.m_threadsPerGroupX = aznumeric_cast<uint16_t>(AZStd::any_cast<int>(args[0]));
                dispatchArgs.m_threadsPerGroupY = aznumeric_cast<uint16_t>(AZStd::any_cast<int>(args[1]));
                dispatchArgs.m_threadsPerGroupZ = aznumeric_cast<uint16_t>(AZStd::any_cast<int>(args[2]));
            }
            m_dispatchItem.m_arguments = dispatchArgs;

            m_isFullscreenPass = passData->m_makeFullscreenPass;

            // Setup pipeline state...
            RHI::PipelineStateDescriptorForDispatch pipelineStateDescriptor;
            const auto& shaderVariant = m_shader->GetVariant(RPI::ShaderAsset::RootShaderVariantStableId);
            shaderVariant.ConfigurePipelineState(pipelineStateDescriptor);

            m_dispatchItem.m_pipelineState = m_shader->AcquirePipelineState(pipelineStateDescriptor);

            ShaderReloadNotificationBus::Handler::BusDisconnect();
            ShaderReloadNotificationBus::Handler::BusConnect(passData->m_shaderReference.m_assetId);
        }

        // Scope producer functions

        void ComputePass::SetupFrameGraphDependencies(RHI::FrameGraphInterface frameGraph, const PassScopeProducer& producer)
        {
            RenderPass::SetupFrameGraphDependencies(frameGraph, producer);
            frameGraph.SetEstimatedItemCount(1);
        }

        void ComputePass::CompileResources(const RHI::FrameGraphCompileContext& context, [[maybe_unused]] const PassScopeProducer& producer)
        {
            if (m_shaderResourceGroup != nullptr)
            {
                BindPassSrg(context, m_shaderResourceGroup);
                m_shaderResourceGroup->Compile();
            }
            if (m_drawSrg != nullptr)
            {
                BindSrg(m_drawSrg->GetRHIShaderResourceGroup());
                m_drawSrg->Compile();
            }
        }

        void ComputePass::BuildCommandList(const RHI::FrameGraphExecuteContext& context, [[maybe_unused]] const PassScopeProducer& producer)
        {
            RHI::CommandList* commandList = context.GetCommandList();

            SetSrgsForDispatch(commandList);

            commandList->Submit(m_dispatchItem);
        }

        void ComputePass::MatchDimensionsToOutput()
        {
            PassAttachment* outputAttachment = nullptr;
            
            if (GetOutputCount() > 0)
            {
                outputAttachment = GetOutputBinding(0).m_attachment.get();
            }
            else if (GetInputOutputCount() > 0)
            {
                outputAttachment = GetInputOutputBinding(0).m_attachment.get();
            }

            AZ_Assert(outputAttachment != nullptr, "[ComputePass '%s']: A fullscreen compute pass must have a valid output or input/output.",
                GetPathName().GetCStr());

            AZ_Assert(outputAttachment->GetAttachmentType() == RHI::AttachmentType::Image,
                "[ComputePass '%s']: The output of a fullscreen compute pass must be an image.",
                GetPathName().GetCStr());

            RHI::Size targetImageSize = outputAttachment->m_descriptor.m_image.m_size;

            SetTargetThreadCounts(targetImageSize.m_width, targetImageSize.m_height, targetImageSize.m_depth);
        }

        void ComputePass::SetTargetThreadCounts(uint32_t targetThreadCountX, uint32_t targetThreadCountY, uint32_t targetThreadCountZ)
        {
            auto& arguments = m_dispatchItem.m_arguments.m_direct;
            arguments.m_totalNumberOfThreadsX = targetThreadCountX;
            arguments.m_totalNumberOfThreadsY = targetThreadCountY;
            arguments.m_totalNumberOfThreadsZ = targetThreadCountZ;
        }

        Data::Instance<ShaderResourceGroup> ComputePass::GetShaderResourceGroup()
        {
            return m_shaderResourceGroup;
        }

        void ComputePass::FrameBeginInternal(FramePrepareParams params)
        {
            if (m_isFullscreenPass)
            {
                MatchDimensionsToOutput();
            }

            RenderPass::FrameBeginInternal(params);
        }

        void ComputePass::OnShaderReinitialized(const Shader& shader)
        {
            AZ_UNUSED(shader);
            LoadShader();
        }

        void ComputePass::OnShaderAssetReinitialized(const Data::Asset<ShaderAsset>& shaderAsset)
        {
            AZ_UNUSED(shaderAsset);
            LoadShader();
        }

        void ComputePass::OnShaderVariantReinitialized(const Shader& shader, const ShaderVariantId& shaderVariantId, ShaderVariantStableId shaderVariantStableId)
        {
            AZ_UNUSED(shader); AZ_UNUSED(shaderVariantId); AZ_UNUSED(shaderVariantStableId);
            LoadShader();
        }

    }   // namespace RPI
}   // namespace AZ
