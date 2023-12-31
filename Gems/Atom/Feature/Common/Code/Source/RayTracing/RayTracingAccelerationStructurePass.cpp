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

#include <Atom/RHI/FrameScheduler.h>
#include <Atom/RHI/CommandList.h>
#include <Atom/RHI/RHISystemInterface.h>
#include <Atom/RPI.Public/Buffer/BufferSystemInterface.h>
#include <Atom/RPI.Public/Buffer/Buffer.h>
#include <Atom/RPI.Public/RenderPipeline.h>
#include <Atom/RPI.Public/Scene.h>
#include <Atom/Feature/Mesh/MeshFeatureProcessor.h>
#include <RayTracing/RayTracingFeatureProcessor.h>
#include <RayTracing/RayTracingAccelerationStructurePass.h>

namespace AZ
{
    namespace Render
    {
        RPI::Ptr<RayTracingAccelerationStructurePass> RayTracingAccelerationStructurePass::Create(const RPI::PassDescriptor& descriptor)
        {
            RPI::Ptr<RayTracingAccelerationStructurePass> rayTracingAccelerationStructurePass = aznew RayTracingAccelerationStructurePass(descriptor);
            return AZStd::move(rayTracingAccelerationStructurePass);
        }

        RayTracingAccelerationStructurePass::RayTracingAccelerationStructurePass(const RPI::PassDescriptor& descriptor)
            : Pass(descriptor)
        {
            // disable this pass if we're on a platform that doesn't support raytracing
            RHI::Ptr<RHI::Device> device = RHI::RHISystemInterface::Get()->GetDevice();
            if (device->GetFeatures().m_rayTracing == false)
            {
                SetEnabled(false);
            }
        }

        void RayTracingAccelerationStructurePass::BuildAttachmentsInternal()
        {
            SetScopeId(RHI::ScopeId(GetPathName()));
        }

        void RayTracingAccelerationStructurePass::FrameBeginInternal(FramePrepareParams params)
        {
            // check raytracing data revision
            RPI::Scene* scene = m_pipeline->GetScene();
            RayTracingFeatureProcessor* rayTracingFeatureProcessor = scene->GetFeatureProcessor<RayTracingFeatureProcessor>();
            if (rayTracingFeatureProcessor && rayTracingFeatureProcessor->GetRevision() != m_rayTracingRevision)
            {
                // ray tracing data has changed, add the scope in order to update the TLAS
                params.m_frameGraphBuilder->ImportScopeProducer(*this);
            }
        }

        void RayTracingAccelerationStructurePass::SetupFrameGraphDependencies(RHI::FrameGraphInterface frameGraph)
        {
            RHI::Ptr<RHI::Device> device = RHI::RHISystemInterface::Get()->GetDevice();

            RPI::Scene* scene = m_pipeline->GetScene();
            TransformServiceFeatureProcessor* transformFeatureProcessor = scene->GetFeatureProcessor<TransformServiceFeatureProcessor>();
            RayTracingFeatureProcessor* rayTracingFeatureProcessor = scene->GetFeatureProcessor<RayTracingFeatureProcessor>();
            RHI::RayTracingBufferPools& rayTracingBufferPools = rayTracingFeatureProcessor->GetBufferPools();
            RayTracingFeatureProcessor::MeshMap& rayTracingMeshes = rayTracingFeatureProcessor->GetMeshes();
            uint32_t rayTracingSubMeshCount = rayTracingFeatureProcessor->GetSubMeshCount();

            // create the TLAS descriptor
            RHI::RayTracingTlasDescriptor tlasDescriptor;
            RHI::RayTracingTlasDescriptor* tlasDescriptorBuild = tlasDescriptor.Build();

            uint32_t blasIndex = 0;
            for (auto& rayTracingMesh : rayTracingMeshes)
            {
                for (auto& rayTracingSubMesh : rayTracingMesh.second.m_subMeshes)
                {
                    tlasDescriptorBuild->Instance()
                        ->InstanceID(blasIndex)
                        ->HitGroupIndex(blasIndex)
                        ->Blas(rayTracingSubMesh.m_blas)
                        ->Transform(rayTracingMesh.second.m_transform)
                    ;
                }

                blasIndex++;
            }

            // create the TLAS buffers based on the descriptor
            RHI::Ptr<RHI::RayTracingTlas>& rayTracingTlas = rayTracingFeatureProcessor->GetTlas();
            rayTracingTlas->CreateBuffers(*device, &tlasDescriptor, rayTracingBufferPools);

            // import and attach the TLAS buffer
            const RHI::Ptr<RHI::Buffer>& rayTracingTlasBuffer = rayTracingTlas->GetTlasBuffer();
            if (rayTracingTlasBuffer && rayTracingSubMeshCount)
            {
                AZ::RHI::AttachmentId tlasAttachmentId = rayTracingFeatureProcessor->GetTlasAttachmentId();
                if (frameGraph.GetAttachmentDatabase().IsAttachmentValid(tlasAttachmentId) == false)
                {
                    RHI::ResultCode result = frameGraph.GetAttachmentDatabase().ImportBuffer(tlasAttachmentId, rayTracingTlasBuffer);
                    AZ_Assert(result == RHI::ResultCode::Success, "Failed to import ray tracing TLAS buffer with error %d", result);
                }

                uint32_t tlasBufferByteCount = aznumeric_cast<uint32_t>(rayTracingTlasBuffer->GetDescriptor().m_byteCount);
                RHI::BufferViewDescriptor tlasBufferViewDescriptor = RHI::BufferViewDescriptor::CreateRayTracingTLAS(tlasBufferByteCount);

                RHI::BufferScopeAttachmentDescriptor desc;
                desc.m_attachmentId = tlasAttachmentId;
                desc.m_bufferViewDescriptor = tlasBufferViewDescriptor;
                desc.m_loadStoreAction.m_loadAction = AZ::RHI::AttachmentLoadAction::DontCare;

                frameGraph.UseShaderAttachment(desc, RHI::ScopeAttachmentAccess::Write);
            }

            m_rayTracingRevision = rayTracingFeatureProcessor->GetRevision();
        }

        void RayTracingAccelerationStructurePass::BuildCommandList(const RHI::FrameGraphExecuteContext& context)
        {
            RPI::Scene* scene = m_pipeline->GetScene();
            RayTracingFeatureProcessor* rayTracingFeatureProcessor = scene->GetFeatureProcessor<RayTracingFeatureProcessor>();
            AZ_Assert(rayTracingFeatureProcessor, "RayTracingAccelerationStructurePass requires the RayTracingFeatureProcessor");

            if (!rayTracingFeatureProcessor->GetSubMeshCount())
            {
                // no ray tracing meshes in the scene
                return;
            }

            // build newly added BLAS objects
            // [GFX TODO][ATOM-14159] Add changelist for meshes in the RayTracingFeatureProcessor
            RayTracingFeatureProcessor::MeshMap& rayTracingMeshes = rayTracingFeatureProcessor->GetMeshes();
            for (auto& rayTracingMesh : rayTracingMeshes)
            {
                if (rayTracingMesh.second.m_blasBuilt == false)
                {
                    for (auto& rayTracingSubMesh : rayTracingMesh.second.m_subMeshes)
                    {
                        context.GetCommandList()->BuildBottomLevelAccelerationStructure(*rayTracingSubMesh.m_blas);
                    }

                    rayTracingMesh.second.m_blasBuilt = true;
                }
            }

            // build the TLAS object
            context.GetCommandList()->BuildTopLevelAccelerationStructure(*rayTracingFeatureProcessor->GetTlas());
        }
    }   // namespace RPI
}   // namespace AZ
