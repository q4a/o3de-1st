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

#include <AzCore/Serialization/SerializeContext.h>
#include <Atom/RPI.Public/RPIUtils.h>
#include <Atom/RPI.Public/Scene.h>
#include <Atom/RPI.Public/Shader/Shader.h>
#include <Atom/RPI.Public/View.h>
#include <Atom/RPI.Public/Pass/PassFilter.h>
#include <DiffuseProbeGrid/DiffuseProbeGridFeatureProcessor.h>
#include <Atom/Feature/TransformService/TransformServiceFeatureProcessor.h>
#include <Atom/RHI/Factory.h>
#include <Atom/RHI/RHISystemInterface.h>
#include <Atom/RHI/PipelineState.h>
#include <Atom/RHI.Reflect/InputStreamLayoutBuilder.h>

// This component invokes shaders based on Nvidia's RTX-GI SDK.
// Please refer to "Shaders/DiffuseGlobalIllumination/Nvidia RTX-GI License.txt" for license information.

namespace AZ
{
    namespace Render
    {
        void DiffuseProbeGridFeatureProcessor::Reflect(ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext
                    ->Class<DiffuseProbeGridFeatureProcessor, FeatureProcessor>()
                    ->Version(0);
            }
        }

        void DiffuseProbeGridFeatureProcessor::Activate()
        {
            AZ::RPI::Scene* scene = GetParentScene();
            RHI::RHISystemInterface* rhiSystem = RHI::RHISystemInterface::Get();

            m_diffuseProbeGrids.reserve(InitialProbeGridAllocationSize);

            RHI::BufferPoolDescriptor desc;
            desc.m_heapMemoryLevel = RHI::HeapMemoryLevel::Device;
            desc.m_bindFlags = RHI::BufferBindFlags::InputAssembly;

            m_bufferPool = RHI::Factory::Get().CreateBufferPool();
            m_bufferPool->SetName(Name("DiffuseProbeGridBoxBufferPool"));
            RHI::ResultCode resultCode = m_bufferPool->Init(*rhiSystem->GetDevice(), desc);
            AZ_Error("DiffuseProbeGridFeatureProcessor", resultCode == RHI::ResultCode::Success, "Failed to initialize buffer pool");

            // create box mesh vertices and indices
            CreateBoxMesh();

            // image pool
            {
                RHI::ImagePoolDescriptor imagePoolDesc;
                imagePoolDesc.m_bindFlags = RHI::ImageBindFlags::ShaderReadWrite;

                m_probeGridRenderData.m_imagePool = RHI::Factory::Get().CreateImagePool();
                RHI::ResultCode result = m_probeGridRenderData.m_imagePool->Init(*rhiSystem->GetDevice(), imagePoolDesc);
                AZ_Assert(result == RHI::ResultCode::Success, "Failed to initialize output image pool");
            }

            // create image view descriptors
            m_probeGridRenderData.m_probeRayTraceImageViewDescriptor = RHI::ImageViewDescriptor::Create(RHI::Format::R32G32B32A32_FLOAT, 0, 0);
            m_probeGridRenderData.m_probeIrradianceImageViewDescriptor = RHI::ImageViewDescriptor::Create(RHI::Format::R10G10B10A2_UNORM, 0, 0);
            m_probeGridRenderData.m_probeDistanceImageViewDescriptor = RHI::ImageViewDescriptor::Create(RHI::Format::R32G32_FLOAT, 0, 0);
            m_probeGridRenderData.m_probeRelocationImageViewDescriptor = RHI::ImageViewDescriptor::Create(RHI::Format::R16G16B16A16_FLOAT, 0, 0);

            // load shader
            // Note: the shader may not be available on all platforms
            Data::Instance<RPI::Shader> shader = RPI::LoadShader("Shaders/DiffuseGlobalIllumination/DiffuseProbeGridRender.azshader");
            if (shader)
            {
                m_probeGridRenderData.m_drawListTag = shader->GetDrawListTag();

                m_probeGridRenderData.m_pipelineState = aznew RPI::PipelineStateForDraw;
                m_probeGridRenderData.m_pipelineState->Init(shader); // uses default shader variant
                m_probeGridRenderData.m_pipelineState->SetInputStreamLayout(m_boxStreamLayout);
                m_probeGridRenderData.m_pipelineState->SetOutputFromScene(GetParentScene());
                m_probeGridRenderData.m_pipelineState->Finalize();

                // load object shader resource group
                m_probeGridRenderData.m_srgAsset = shader->FindShaderResourceGroupAsset(Name{ "ObjectSrg" });
                AZ_Error("DiffuseProbeGridFeatureProcessor", m_probeGridRenderData.m_srgAsset.GetId().IsValid(), "Failed to find ObjectSrg asset");
                AZ_Error("DiffuseProbeGridFeatureProcessor", m_probeGridRenderData.m_srgAsset.IsReady(), "ObjectSrg asset is not loaded");
            }

            EnableSceneNotification();
        }

        void DiffuseProbeGridFeatureProcessor::Deactivate()
        {
            AZ_Warning("DiffuseProbeGridFeatureProcessor", m_diffuseProbeGrids.size() == 0, 
                "Deactivating the DiffuseProbeGridFeatureProcessor, but there are still outstanding probe grids probes. Components\n"
                "using DiffuseProbeGridHandles should free them before the DiffuseProbeGridFeatureProcessor is deactivated.\n"
            );

            DisableSceneNotification();

            if (m_bufferPool)
            {
                m_bufferPool.reset();
            }
        }

        void DiffuseProbeGridFeatureProcessor::Simulate([[maybe_unused]] const FeatureProcessor::SimulatePacket& packet)
        {
            AZ_PROFILE_FUNCTION(Debug::ProfileCategory::AzRender);

            // update pipeline states
            if (m_needUpdatePipelineStates)
            {
                UpdatePipelineStates();
                m_needUpdatePipelineStates = false;
            }

            // if the volumes changed we need to re-sort the probe list
            if (m_probeGridSortRequired)
            {
                AZ_PROFILE_SCOPE(Debug::ProfileCategory::AzRender, "Sort diffuse probe grids");

                // sort the probes by descending inner volume size, so the smallest volumes are rendered last
                auto sortFn = [](AZStd::shared_ptr<DiffuseProbeGrid> const& probe1, AZStd::shared_ptr<DiffuseProbeGrid> const& probe2) -> bool
                {
                    const Aabb& aabb1 = probe1->GetAabbWs();
                    const Aabb& aabb2 = probe2->GetAabbWs();
                    float size1 = aabb1.GetXExtent() * aabb1.GetZExtent() * aabb1.GetYExtent();
                    float size2 = aabb2.GetXExtent() * aabb2.GetZExtent() * aabb2.GetYExtent();
                    return (size1 > size2);
                };

                AZStd::sort(m_diffuseProbeGrids.begin(), m_diffuseProbeGrids.end(), sortFn);
                m_probeGridSortRequired = false;
            }

            // call Simulate on all diffuse probe grids
            for (uint32_t probeGridIndex = 0; probeGridIndex < m_diffuseProbeGrids.size(); ++probeGridIndex)
            {
                AZStd::shared_ptr<DiffuseProbeGrid>& diffuseProbeGrid = m_diffuseProbeGrids[probeGridIndex];
                diffuseProbeGrid->Simulate(GetParentScene(), probeGridIndex);
            }
        }

        void DiffuseProbeGridFeatureProcessor::Render(const FeatureProcessor::RenderPacket& packet)
        {
            AZ_PROFILE_FUNCTION(Debug::ProfileCategory::AzRender);

            for (auto& diffuseProbeGrid : m_diffuseProbeGrids)
            {
                for (auto& view : packet.m_views)
                {
                    AZ_Assert(diffuseProbeGrid.use_count() > 1, "DiffuseProbeGrid found with no corresponding owner, ensure that RemoveProbe() is called before releasing probe handles");

                    diffuseProbeGrid->Render(view);
                }
            }
        }

        DiffuseProbeGridHandle DiffuseProbeGridFeatureProcessor::AddProbeGrid(const AZ::Transform& transform, const AZ::Vector3& extents, const AZ::Vector3& probeSpacing)
        {
            AZStd::shared_ptr<DiffuseProbeGrid> diffuseProbeGrid = AZStd::make_shared<DiffuseProbeGrid>();
            diffuseProbeGrid->Init(GetParentScene(), &m_probeGridRenderData);
            diffuseProbeGrid->SetTransform(transform);
            diffuseProbeGrid->SetExtents(extents);
            diffuseProbeGrid->SetProbeSpacing(probeSpacing);
            m_diffuseProbeGrids.push_back(diffuseProbeGrid);
            m_probeGridSortRequired = true;

            return diffuseProbeGrid;
        }

        void DiffuseProbeGridFeatureProcessor::RemoveProbeGrid(DiffuseProbeGridHandle& probeGrid)
        {
            AZ_Assert(probeGrid.get(), "RemoveProbeGrid called with an invalid handle");

            auto itEntry = AZStd::find_if(m_diffuseProbeGrids.begin(), m_diffuseProbeGrids.end(), [&](AZStd::shared_ptr<DiffuseProbeGrid> const& entry)
            {
                return (entry == probeGrid);
            });

            AZ_Assert(itEntry != m_diffuseProbeGrids.end(), "RemoveProbeGrid called with a probe grid that is not in the probe list");
            m_diffuseProbeGrids.erase(itEntry);
            probeGrid = nullptr;
        }

        bool DiffuseProbeGridFeatureProcessor::ValidateExtents(const DiffuseProbeGridHandle& probeGrid, const AZ::Vector3& newExtents)
        {
            AZ_Assert(probeGrid.get(), "SetTransform called with an invalid handle");
            return probeGrid->ValidateExtents(newExtents);
        }

        void DiffuseProbeGridFeatureProcessor::SetExtents(const DiffuseProbeGridHandle& probeGrid, const AZ::Vector3& extents)
        {
            AZ_Assert(probeGrid.get(), "SetExtents called with an invalid handle");
            probeGrid->SetExtents(extents);
            m_probeGridSortRequired = true;
        }

        void DiffuseProbeGridFeatureProcessor::SetTransform(const DiffuseProbeGridHandle& probeGrid, const AZ::Transform& transform)
        {
            AZ_Assert(probeGrid.get(), "SetTransform called with an invalid handle");
            probeGrid->SetTransform(transform);
            m_probeGridSortRequired = true;
        }

        bool DiffuseProbeGridFeatureProcessor::ValidateProbeSpacing(const DiffuseProbeGridHandle& probeGrid, const AZ::Vector3& newSpacing)
        {
            AZ_Assert(probeGrid.get(), "SetTransform called with an invalid handle");
            return probeGrid->ValidateProbeSpacing(newSpacing);
        }

        void DiffuseProbeGridFeatureProcessor::SetProbeSpacing(const DiffuseProbeGridHandle& probeGrid, const AZ::Vector3& probeSpacing)
        {
            AZ_Assert(probeGrid.get(), "SetProbeSpacing called with an invalid handle");
            probeGrid->SetProbeSpacing(probeSpacing);
        }

        void DiffuseProbeGridFeatureProcessor::SetViewBias(const DiffuseProbeGridHandle& probeGrid, float viewBias)
        {
            AZ_Assert(probeGrid.get(), "SetViewBias called with an invalid handle");
            probeGrid->SetViewBias(viewBias);
        }

        void DiffuseProbeGridFeatureProcessor::SetNormalBias(const DiffuseProbeGridHandle& probeGrid, float normalBias)
        {
            AZ_Assert(probeGrid.get(), "SetNormalBias called with an invalid handle");
            probeGrid->SetNormalBias(normalBias);
        }

        void DiffuseProbeGridFeatureProcessor::SetAmbientMultiplier(const DiffuseProbeGridHandle& probeGrid, float ambientMultiplier)
        {
            AZ_Assert(probeGrid.get(), "SetAmbientMultiplier called with an invalid handle");
            probeGrid->SetAmbientMultiplier(ambientMultiplier);
        }

        void DiffuseProbeGridFeatureProcessor::Enable(const DiffuseProbeGridHandle& probeGrid, bool enable)
        {
            AZ_Assert(probeGrid.get(), "Enable called with an invalid handle");
            probeGrid->Enable(enable);
        }

        void DiffuseProbeGridFeatureProcessor::SetGIShadows(const DiffuseProbeGridHandle& probeGrid, bool giShadows)
        {
            AZ_Assert(probeGrid.get(), "Enable called with an invalid handle");
            probeGrid->SetGIShadows(giShadows);
        }

        void DiffuseProbeGridFeatureProcessor::CreateBoxMesh()
        {
            // vertex positions
            static const Position positions[] =
            {
                // front
                { -0.5f, -0.5f,  0.5f },
                {  0.5f, -0.5f,  0.5f },
                {  0.5f,  0.5f,  0.5f },
                { -0.5f,  0.5f,  0.5f },

                // back
                { -0.5f, -0.5f, -0.5f },
                {  0.5f, -0.5f, -0.5f },
                {  0.5f,  0.5f, -0.5f },
                { -0.5f,  0.5f, -0.5f },

                // left
                { -0.5f, -0.5f,  0.5f },
                { -0.5f,  0.5f,  0.5f },
                { -0.5f,  0.5f, -0.5f },
                { -0.5f, -0.5f, -0.5f },

                // right
                {  0.5f, -0.5f,  0.5f },
                {  0.5f,  0.5f,  0.5f },
                {  0.5f,  0.5f, -0.5f },
                {  0.5f, -0.5f, -0.5f },

                // bottom
                { -0.5f, -0.5f,  0.5f },
                {  0.5f, -0.5f,  0.5f },
                {  0.5f, -0.5f, -0.5f },
                { -0.5f, -0.5f, -0.5f },

                // top
                { -0.5f,  0.5f,  0.5f },
                {  0.5f,  0.5f,  0.5f },
                {  0.5f,  0.5f, -0.5f },
                { -0.5f,  0.5f, -0.5f },
            };
            static const u32 numPositions = sizeof(positions) / sizeof(positions[0]);

            for (u32 i = 0; i < numPositions; ++i)
            {
                m_boxPositions.push_back(positions[i]);
            }

            // indices
            static const uint16_t indices[] =
            {
                // front
                0, 1, 2, 2, 3, 0,

                // back
                5, 4, 7, 7, 6, 5,

                // left
                8, 9, 10, 10, 11, 8,

                // right
                14, 13, 12, 12, 15, 14,

                // bottom
                18, 17, 16, 16, 19, 18,

                // top
                23, 20, 21, 21, 22, 23
            };
            static const u32 numIndices = sizeof(indices) / sizeof(indices[0]);

            for (u32 i = 0; i < numIndices; ++i)
            {
                m_boxIndices.push_back(indices[i]);
            }

            // create stream layout
            RHI::InputStreamLayoutBuilder layoutBuilder;
            layoutBuilder.AddBuffer()->Channel("POSITION", RHI::Format::R32G32B32_FLOAT);
            layoutBuilder.SetTopology(RHI::PrimitiveTopology::TriangleList);
            m_boxStreamLayout = layoutBuilder.End();

            // create index buffer
            AZ::RHI::BufferInitRequest request;
            m_boxIndexBuffer = AZ::RHI::Factory::Get().CreateBuffer();
            request.m_buffer = m_boxIndexBuffer.get();
            request.m_descriptor = AZ::RHI::BufferDescriptor{ AZ::RHI::BufferBindFlags::InputAssembly, m_boxIndices.size() * sizeof(uint16_t) };
            request.m_initialData = m_boxIndices.data();
            AZ::RHI::ResultCode result = m_bufferPool->InitBuffer(request);
            AZ_Error("DiffuseProbeGridFeatureProcessor", result == RHI::ResultCode::Success, "Failed to initialize box index buffer - error [%d]", result);

            // create index buffer view
            AZ::RHI::IndexBufferView indexBufferView =
            {
                *m_boxIndexBuffer,
                0,
                sizeof(indices),
                AZ::RHI::IndexFormat::Uint16,
            };
            m_probeGridRenderData.m_boxIndexBufferView = indexBufferView;
            m_probeGridRenderData.m_boxIndexCount = numIndices;

            // create position buffer
            m_boxPositionBuffer = AZ::RHI::Factory::Get().CreateBuffer();
            request.m_buffer = m_boxPositionBuffer.get();
            request.m_descriptor = AZ::RHI::BufferDescriptor{ AZ::RHI::BufferBindFlags::InputAssembly, m_boxPositions.size() * sizeof(Position) };
            request.m_initialData = m_boxPositions.data();
            result = m_bufferPool->InitBuffer(request);
            AZ_Error("DiffuseProbeGridFeatureProcessor", result == RHI::ResultCode::Success, "Failed to initialize box index buffer - error [%d]", result);

            // create position buffer view
            RHI::StreamBufferView positionBufferView =
            {
                *m_boxPositionBuffer,
                0,
                (uint32_t)(m_boxPositions.size() * sizeof(Position)),
                sizeof(Position),
            };
            m_probeGridRenderData.m_boxPositionBufferView = { { positionBufferView } };

            AZ::RHI::ValidateStreamBufferViews(m_boxStreamLayout, m_probeGridRenderData.m_boxPositionBufferView);
        }

        void DiffuseProbeGridFeatureProcessor::OnRenderPipelinePassesChanged([[maybe_unused]] RPI::RenderPipeline* renderPipeline)
        {
            UpdatePasses();
            m_needUpdatePipelineStates = true;
        }

        void DiffuseProbeGridFeatureProcessor::OnRenderPipelineAdded([[maybe_unused]] RPI::RenderPipelinePtr pipeline)
        {
            UpdatePasses();
            m_needUpdatePipelineStates = true;
        }

        void DiffuseProbeGridFeatureProcessor::OnRenderPipelineRemoved([[maybe_unused]] RPI::RenderPipeline* pipeline)
        {
            m_needUpdatePipelineStates = true;
        }

        void DiffuseProbeGridFeatureProcessor::UpdatePipelineStates()
        {
            if(m_probeGridRenderData.m_pipelineState)
            {
                m_probeGridRenderData.m_pipelineState->SetOutputFromScene(GetParentScene());
                m_probeGridRenderData.m_pipelineState->Finalize();
            }
        }

        void DiffuseProbeGridFeatureProcessor::UpdatePasses()
        {
            // disable the DiffuseProbeGridUpdatePass if the platform does not support raytracing
            RHI::Ptr<RHI::Device> device = RHI::RHISystemInterface::Get()->GetDevice();
            if (device->GetFeatures().m_rayTracing == false)
            {
                RPI::PassHierarchyFilter updatePassFilter(AZ::Name("DiffuseProbeGridUpdatePass"));
                const AZStd::vector<RPI::Pass*>& updatePasses = RPI::PassSystemInterface::Get()->FindPasses(updatePassFilter);
                for (RPI::Pass* pass : updatePasses)
                {
                    pass->SetEnabled(false);
                }

                RPI::PassHierarchyFilter renderPassFilter(AZ::Name("DiffuseProbeGridRenderPass"));
                const AZStd::vector<RPI::Pass*>& renderPasses = RPI::PassSystemInterface::Get()->FindPasses(renderPassFilter);
                for (RPI::Pass* pass : renderPasses)
                {
                    pass->SetEnabled(false);
                }
            }
        }

    } // namespace Render
} // namespace AZ
