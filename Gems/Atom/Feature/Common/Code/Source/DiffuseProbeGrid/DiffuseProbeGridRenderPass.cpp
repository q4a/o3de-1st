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

#include <DiffuseProbeGrid/DiffuseProbeGridRenderPass.h>
#include <DiffuseProbeGrid/DiffuseProbeGridFeatureProcessor.h>
#include <Atom/RPI.Public/Image/AttachmentImagePool.h>
#include <Atom/RPI.Public/Image/ImageSystemInterface.h>
#include <Atom/RPI.Public/RenderPipeline.h>
#include <Atom/RPI.Public/RPIUtils.h>
#include <RayTracing/RayTracingFeatureProcessor.h>

namespace AZ
{
    namespace Render
    {
        RPI::Ptr<Render::DiffuseProbeGridRenderPass> DiffuseProbeGridRenderPass::Create(const RPI::PassDescriptor& descriptor)
        {
            return aznew DiffuseProbeGridRenderPass(descriptor);
        }

        DiffuseProbeGridRenderPass::DiffuseProbeGridRenderPass(const RPI::PassDescriptor& descriptor)
            : Base(descriptor)
        {
            // create the shader resource group
            // Note: the shader may not be available on all platforms
            AZStd::string shaderFilePath = "Shaders/DiffuseGlobalIllumination/DiffuseProbeGridRender.azshader";
            m_shader = RPI::LoadShader(shaderFilePath);
            if (m_shader == nullptr)
            {
                return;
            }

            m_srgAsset = m_shader->FindShaderResourceGroupAsset(RPI::SrgBindingSlot::Pass);
            AZ_Assert(m_srgAsset->IsReady(), "[DiffuseProbeGridRenderPass '%s']: Failed to find SRG asset", GetPathName().GetCStr());

            m_shaderResourceGroup = RPI::ShaderResourceGroup::Create(m_srgAsset);
            AZ_Assert(m_shaderResourceGroup, "[DiffuseProbeGridRenderPass '%s']: Failed to create SRG", GetPathName().GetCStr());
        }

        void DiffuseProbeGridRenderPass::FrameBeginInternal(FramePrepareParams params)
        {
            RPI::Scene* scene = m_pipeline->GetScene();
            DiffuseProbeGridFeatureProcessor* diffuseProbeGridFeatureProcessor = scene->GetFeatureProcessor<DiffuseProbeGridFeatureProcessor>();

            if (!diffuseProbeGridFeatureProcessor || diffuseProbeGridFeatureProcessor->GetProbeGrids().empty())
            {
                // no diffuse probe grids
                return;
            }

            RayTracingFeatureProcessor* rayTracingFeatureProcessor = scene->GetFeatureProcessor<RayTracingFeatureProcessor>();
            AZ_Assert(rayTracingFeatureProcessor, "DiffuseProbeGridRenderPass requires the RayTracingFeatureProcessor");

            if (!rayTracingFeatureProcessor->GetSubMeshCount())
            {
                // empty scene
                return;
            }

            // get output attachment size
            AZ_Assert(GetInputOutputCount() > 0, "DiffuseProbeGridRenderPass: Could not find output bindings");
            RPI::PassAttachment* m_outputAttachment = GetInputOutputBinding(0).m_attachment.get();
            AZ_Assert(m_outputAttachment, "DiffuseProbeGridRenderPass: Output binding has no attachment!");
            
            RHI::Size size = m_outputAttachment->m_descriptor.m_image.m_size;
            RHI::Viewport viewport(0.f, aznumeric_cast<float>(size.m_width), 0.f, aznumeric_cast<float>(size.m_height));
            RHI::Scissor scissor(0, 0, size.m_width, size.m_height);
            
            params.m_viewportState = viewport;
            params.m_scissorState = scissor;

            Base::FrameBeginInternal(params);
        }

        void DiffuseProbeGridRenderPass::SetupFrameGraphDependencies(RHI::FrameGraphInterface frameGraph, const RPI::PassScopeProducer& producer)
        {
            RPI::Scene* scene = m_pipeline->GetScene();
            DiffuseProbeGridFeatureProcessor* diffuseProbeGridFeatureProcessor = scene->GetFeatureProcessor<DiffuseProbeGridFeatureProcessor>();

            for (auto& diffuseProbeGrid : diffuseProbeGridFeatureProcessor->GetProbeGrids())
            {
                // probe irradiance image
                {
                    RHI::ImageScopeAttachmentDescriptor desc;
                    desc.m_attachmentId = diffuseProbeGrid->GetIrradianceImageAttachmentId();
                    desc.m_imageViewDescriptor = diffuseProbeGrid->GetRenderData()->m_probeIrradianceImageViewDescriptor;
                    desc.m_loadStoreAction.m_loadAction = AZ::RHI::AttachmentLoadAction::Load;

                    frameGraph.UseShaderAttachment(desc, RHI::ScopeAttachmentAccess::Read);
                }

                // probe distance image
                {
                    RHI::ImageScopeAttachmentDescriptor desc;
                    desc.m_attachmentId = diffuseProbeGrid->GetDistanceImageAttachmentId();
                    desc.m_imageViewDescriptor = diffuseProbeGrid->GetRenderData()->m_probeDistanceImageViewDescriptor;
                    desc.m_loadStoreAction.m_loadAction = AZ::RHI::AttachmentLoadAction::Load;

                    frameGraph.UseShaderAttachment(desc, RHI::ScopeAttachmentAccess::Read);
                }

                // probe relocation image
                {
                    RHI::ImageScopeAttachmentDescriptor desc;
                    desc.m_attachmentId = diffuseProbeGrid->GetRelocationImageAttachmentId();
                    desc.m_imageViewDescriptor = diffuseProbeGrid->GetRenderData()->m_probeRelocationImageViewDescriptor;
                    desc.m_loadStoreAction.m_loadAction = AZ::RHI::AttachmentLoadAction::Load;

                    frameGraph.UseShaderAttachment(desc, RHI::ScopeAttachmentAccess::ReadWrite);
                }
            }

            Base::SetupFrameGraphDependencies(frameGraph, producer);
        }

        void DiffuseProbeGridRenderPass::CompileResources(const RHI::FrameGraphCompileContext& context, const RPI::PassScopeProducer& producer)
        {
            RPI::Scene* scene = m_pipeline->GetScene();
            DiffuseProbeGridFeatureProcessor* diffuseProbeGridFeatureProcessor = scene->GetFeatureProcessor<DiffuseProbeGridFeatureProcessor>();

            for (auto& diffuseProbeGrid : diffuseProbeGridFeatureProcessor->GetProbeGrids())
            {
                // the diffuse probe grid Srg must be updated in the Compile phase in order to successfully bind the ReadWrite shader inputs
                // (see line 93 of ShaderResourceGroupData.cpp)
                diffuseProbeGrid->UpdateRenderObjectSrg();
            }

            Base::CompileResources(context, producer);
        }
    } // namespace Render
} // namespace AZ
