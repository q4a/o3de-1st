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

#include <Atom/RHI.Reflect/ShaderResourceGroupLayoutDescriptor.h>
#include <Atom/RPI.Public/Shader/ShaderResourceGroup.h>
#include <DiffuseProbeGrid/DiffuseProbeGrid.h>
#include <Atom/RPI.Public/View.h>
#include <Atom/RHI/RHISystemInterface.h>
#include <Atom/RHI/Factory.h>
#include <CoreLights/DirectionalLightFeatureProcessor.h>
#include <CoreLights/SpotLightFeatureProcessor.h>
#include <CoreLights/PointLightFeatureProcessor.h>
#include <RayTracing/RayTracingFeatureProcessor.h>
#include <AzCore/Math/MathUtils.h>

namespace AZ
{
    namespace Render
    {
        DiffuseProbeGrid::~DiffuseProbeGrid()
        {
            if (m_transformService && m_objectId.IsValid())
            {
                m_transformService->ReleaseObjectId(m_objectId);
            }

            m_transformService = nullptr;
        }

        void DiffuseProbeGrid::Init(RPI::Scene* scene, DiffuseProbeGridRenderData* renderData)
        {
            AZ_Assert(scene, "DiffuseProbeGrid::Init called with a null Scene pointer");

            m_renderData = renderData;

            // reserve transform objectId
            m_transformService = scene->GetFeatureProcessor<TransformServiceFeatureProcessor>();
            AZ_Assert(m_transformService, "ReflectionProbe requires a TransformServiceFeatureProcessor on its parent scene.");
            m_objectId = m_transformService->ReserveObjectId();

            // create attachment Ids
            AZStd::string uuidString = AZ::Uuid::CreateRandom().ToString<AZStd::string>();
            m_rayTraceImageAttachmentId = AZStd::string::format("ProbeRayTraceImageAttachmentId_%s", uuidString.c_str());
            m_irradianceImageAttachmentId = AZStd::string::format("ProbeIrradianceImageAttachmentId_%s", uuidString.c_str());
            m_distanceImageAttachmentId = AZStd::string::format("ProbeDistanceImageAttachmentId_%s", uuidString.c_str());
            m_relocationImageAttachmentId = AZStd::string::format("ProbeRelocationImageAttachmentId_%s", uuidString.c_str());
        }

        void DiffuseProbeGrid::Simulate([[maybe_unused]] RPI::Scene* scene, uint32_t probeIndex)
        {
            AZ_Assert(scene, "DiffuseProbeGrid::Simulate called with a null Scene pointer");

            UpdateTextures();

            if (m_renderObjectSrg)
            {
                // the list index passed in from the feature processor is the index of this probe in the sorted probe list.
                // this is needed to render the probe volumes in order from largest to smallest
                RHI::DrawItemSortKey sortKey = static_cast<RHI::DrawItemSortKey>(probeIndex);
                if (sortKey != m_sortKey)
                {
                    if (m_renderData->m_pipelineState->GetRHIPipelineState())
                    {
                        // the sort key changed, rebuild draw packets
                        m_sortKey = sortKey;

                        RHI::DrawPacketBuilder drawPacketBuilder;

                        RHI::DrawIndexed drawIndexed;
                        drawIndexed.m_indexCount = aznumeric_cast<uint32_t>(m_renderData->m_boxIndexCount);
                        drawIndexed.m_indexOffset = 0;
                        drawIndexed.m_vertexOffset = 0;

                        drawPacketBuilder.Begin(nullptr);
                        drawPacketBuilder.SetDrawArguments(drawIndexed);
                        drawPacketBuilder.SetIndexBufferView(m_renderData->m_boxIndexBufferView);
                        drawPacketBuilder.AddShaderResourceGroup(m_renderObjectSrg->GetRHIShaderResourceGroup());

                        RHI::DrawPacketBuilder::DrawRequest drawRequest;
                        drawRequest.m_listTag = m_renderData->m_drawListTag;
                        drawRequest.m_pipelineState = m_renderData->m_pipelineState->GetRHIPipelineState();
                        drawRequest.m_streamBufferViews = m_renderData->m_boxPositionBufferView;
                        drawRequest.m_sortKey = m_sortKey;
                        drawPacketBuilder.AddDrawItem(drawRequest);

                        m_drawPacket = drawPacketBuilder.End();
                    }
                }
            }

            m_probeRayRotationTransform = ComputeRandomRotation();
        }

        void DiffuseProbeGrid::Render(RPI::ViewPtr view)
        {
            // [GFX TODO][ATOM-4364] Add culling for probe grids
            if (view->HasDrawListTag(m_renderData->m_drawListTag))
            {
                if (m_drawPacket)
                {
                    view->AddDrawPacket(m_drawPacket.get());
                }
            }
        }

        bool DiffuseProbeGrid::ValidateProbeSpacing(const AZ::Vector3& newSpacing)
        {
            return ValidateProbeCount(m_extents, newSpacing);
        }

        void DiffuseProbeGrid::SetProbeSpacing(const AZ::Vector3& probeSpacing)
        {
            m_probeSpacing = probeSpacing;

            // recompute the number of probes since the spacing changed
            UpdateProbeCount();

            // probes need to be relocated since the grid density changed
            m_remainingRelocationIterations = DefaultNumRelocationIterations;

            m_updateTextures = true;
        }

        void DiffuseProbeGrid::SetViewBias(float viewBias)
        {
            m_viewBias = viewBias;
            m_updateRenderObjectSrg = true;
        }

        void DiffuseProbeGrid::SetNormalBias(float normalBias)
        {
            m_normalBias = normalBias;
            m_updateRenderObjectSrg = true;
        }

        void DiffuseProbeGrid::SetTransform(const AZ::Transform& transform)
        {
            m_position = transform.GetTranslation();
            m_transformService->SetTransformForId(m_objectId, transform);
            m_aabbWs = Aabb::CreateCenterHalfExtents(m_position, m_extents / 2.0f);

            // probes need to be relocated since the grid position changed
            m_remainingRelocationIterations = DefaultNumRelocationIterations;

            m_updateRenderObjectSrg = true;
        }

        bool DiffuseProbeGrid::ValidateExtents(const AZ::Vector3& newExtents)
        {
            return ValidateProbeCount(newExtents, m_probeSpacing);
        }

        void DiffuseProbeGrid::SetExtents(const AZ::Vector3& extents)
        {
            m_extents = extents;
            m_aabbWs = Aabb::CreateCenterHalfExtents(m_position, m_extents / 2.0f);

            // recompute the number of probes since the extents changed
            UpdateProbeCount();

            // probes need to be relocated since the grid extents changed
            m_remainingRelocationIterations = DefaultNumRelocationIterations;

            m_updateTextures = true;
        }

        void DiffuseProbeGrid::SetAmbientMultiplier(float ambientMultiplier)
        {
            m_ambientMultiplier = ambientMultiplier;
            m_updateRenderObjectSrg = true;
        }

        void DiffuseProbeGrid::Enable(bool enabled)
        {
            m_enabled = enabled;
            m_updateRenderObjectSrg = true;
        }

        uint32_t DiffuseProbeGrid::GetTotalProbeCount() const
        {
            return m_probeCountX * m_probeCountY * m_probeCountZ;
        }

        // compute probe counts for a 2D texture layout
        void DiffuseProbeGrid::GetTexture2DProbeCount(uint32_t& probeCountX, uint32_t& probeCountY) const
        {
            // z-up left-handed
            probeCountX = m_probeCountY * m_probeCountZ;
            probeCountY = m_probeCountX;
        }

        void DiffuseProbeGrid::UpdateTextures()
        {
            if (!m_updateTextures)
            {
                return;
            }

            RHI::Ptr<RHI::Device> device = RHI::RHISystemInterface::Get()->GetDevice();

            // advance to the next image in the frame image array
            m_currentImageIndex = (m_currentImageIndex + 1) % ImageFrameCount;

            // probe raytrace
            {
                uint32_t width = m_numRaysPerProbe;
                uint32_t height = GetTotalProbeCount();

                m_rayTraceImage[m_currentImageIndex] = RHI::Factory::Get().CreateImage();
                uint32_t imageSize = width * height * RHI::GetFormatSize(RHI::Format::R32G32B32A32_FLOAT);

                RHI::ImageInitRequest request;
                request.m_image = m_rayTraceImage[m_currentImageIndex].get();
                request.m_descriptor = RHI::ImageDescriptor::Create2D(RHI::ImageBindFlags::ShaderReadWrite, width, height, RHI::Format::R32G32B32A32_FLOAT);
                RHI::ResultCode result = m_renderData->m_imagePool->InitImage(request);
                AZ_Assert(result == RHI::ResultCode::Success, "Failed to initialize m_probeRayTraceImage image");
            }

            uint32_t probeCountX;
            uint32_t probeCountY;
            GetTexture2DProbeCount(probeCountX, probeCountY);

            // probe irradiance
            {
                uint32_t width = probeCountX * (DefaultNumIrradianceTexels + 2);
                uint32_t height = probeCountY * (DefaultNumIrradianceTexels + 2);

                m_irradianceImage[m_currentImageIndex] = RHI::Factory::Get().CreateImage();
                uint32_t imageSize = width * height * RHI::GetFormatSize(RHI::Format::R10G10B10A2_UNORM);

                RHI::ImageInitRequest request;
                request.m_image = m_irradianceImage[m_currentImageIndex].get();
                request.m_descriptor = RHI::ImageDescriptor::Create2D(RHI::ImageBindFlags::ShaderReadWrite, width, height, RHI::Format::R10G10B10A2_UNORM);
                RHI::ResultCode result = m_renderData->m_imagePool->InitImage(request);
                AZ_Assert(result == RHI::ResultCode::Success, "Failed to initialize m_probeIrradianceImage image");
            }

            // probe distance
            {
                uint32_t width = probeCountX * (DefaultNumDistanceTexels + 2);
                uint32_t height = probeCountY * (DefaultNumDistanceTexels + 2);

                m_distanceImage[m_currentImageIndex] = RHI::Factory::Get().CreateImage();
                uint32_t imageSize = width * height * RHI::GetFormatSize(RHI::Format::R32G32_FLOAT);

                RHI::ImageInitRequest request;
                request.m_image = m_distanceImage[m_currentImageIndex].get();
                request.m_descriptor = RHI::ImageDescriptor::Create2D(RHI::ImageBindFlags::ShaderReadWrite, width, height, RHI::Format::R32G32_FLOAT);
                RHI::ResultCode result = m_renderData->m_imagePool->InitImage(request);
                AZ_Assert(result == RHI::ResultCode::Success, "Failed to initialize m_probeDistanceImage image");
            }

            // probe relocation
            {
                uint32_t width = probeCountX;
                uint32_t height = probeCountY;

                m_relocationImage[m_currentImageIndex] = RHI::Factory::Get().CreateImage();
                uint32_t imageSize = width * height * RHI::GetFormatSize(RHI::Format::R16G16B16A16_FLOAT);

                RHI::ImageInitRequest request;
                request.m_image = m_relocationImage[m_currentImageIndex].get();
                request.m_descriptor = RHI::ImageDescriptor::Create2D(RHI::ImageBindFlags::ShaderReadWrite, width, height, RHI::Format::R16G16B16A16_FLOAT);
                RHI::ResultCode result = m_renderData->m_imagePool->InitImage(request);
                AZ_Assert(result == RHI::ResultCode::Success, "Failed to initialize m_probeRelocationImage image");
            }

            m_updateTextures = false;

            // textures have changed so we need to update the render Srg to bind the new ones
            m_updateRenderObjectSrg = true;
        }

        void DiffuseProbeGrid::ComputeProbeCount(const AZ::Vector3& extents, const AZ::Vector3& probeSpacing, uint32_t& probeCountX, uint32_t& probeCountY, uint32_t& probeCountZ)
        {
            probeCountX = aznumeric_cast<uint32_t>(AZStd::floorf(extents.GetX() / probeSpacing.GetX()));
            probeCountY = aznumeric_cast<uint32_t>(AZStd::floorf(extents.GetY() / probeSpacing.GetY()));
            probeCountZ = aznumeric_cast<uint32_t>(AZStd::floorf(extents.GetZ() / probeSpacing.GetZ()));
        }

        bool DiffuseProbeGrid::ValidateProbeCount(const AZ::Vector3& extents, const AZ::Vector3& probeSpacing)
        {
            uint32_t probeCountX = 0;
            uint32_t probeCountY = 0;
            uint32_t probeCountZ = 0;
            ComputeProbeCount(extents, probeSpacing, probeCountX, probeCountY, probeCountZ);
            uint32_t totalProbeCount = probeCountX * probeCountY * probeCountZ;

            if (totalProbeCount == 0)
            {
                return false;
            }

            // radiance texture height is equal to the probe count
            if (totalProbeCount > MaxTextureDimension)
            {
                return false;
            }

            // distance texture uses the largest number of texels per probe
            // z-up left-handed
            uint32_t width = probeCountY * probeCountZ * (DefaultNumDistanceTexels + 2);
            uint32_t height = probeCountX * (DefaultNumDistanceTexels + 2);

            if (width > MaxTextureDimension || height > MaxTextureDimension)
            {
                return false;
            }

            return true;
        }

        void DiffuseProbeGrid::UpdateProbeCount()
        {
            ComputeProbeCount(m_extents,
                              m_probeSpacing,
                              m_probeCountX,
                              m_probeCountY,
                              m_probeCountZ);
        }

        void DiffuseProbeGrid::SetGridConstants(Data::Instance<RPI::ShaderResourceGroup>& srg)
        {
            const RHI::ShaderResourceGroupLayout* srgLayout = srg->GetLayout();
            RHI::ShaderInputConstantIndex constantIndex;

            constantIndex = srgLayout->FindShaderInputConstantIndex(AZ::Name("m_probeGrid.origin"));
            srg->SetConstant(constantIndex, m_position);

            constantIndex = srgLayout->FindShaderInputConstantIndex(AZ::Name("m_probeGrid.numRaysPerProbe"));
            srg->SetConstant(constantIndex, m_numRaysPerProbe);

            constantIndex = srgLayout->FindShaderInputConstantIndex(AZ::Name("m_probeGrid.probeGridSpacing"));
            srg->SetConstant(constantIndex, m_probeSpacing);

            constantIndex = srgLayout->FindShaderInputConstantIndex(AZ::Name("m_probeGrid.probeMaxRayDistance"));
            srg->SetConstant(constantIndex, m_probeMaxRayDistance);

            constantIndex = srgLayout->FindShaderInputConstantIndex(AZ::Name("m_probeGrid.probeGridCounts"));
            uint32_t probeGridCounts[3];
            probeGridCounts[0] = m_probeCountX;
            probeGridCounts[1] = m_probeCountY;
            probeGridCounts[2] = m_probeCountZ;
            srg->SetConstantRaw(constantIndex, &probeGridCounts[0], sizeof(probeGridCounts));

            constantIndex = srgLayout->FindShaderInputConstantIndex(AZ::Name("m_probeGrid.probeDistanceExponent"));
            srg->SetConstant(constantIndex, m_probeDistanceExponent);

            constantIndex = srgLayout->FindShaderInputConstantIndex(AZ::Name("m_probeGrid.probeHysteresis"));
            srg->SetConstant(constantIndex, m_probeHysteresis);

            constantIndex = srgLayout->FindShaderInputConstantIndex(AZ::Name("m_probeGrid.probeChangeThreshold"));
            srg->SetConstant(constantIndex, m_probeChangeThreshold);

            constantIndex = srgLayout->FindShaderInputConstantIndex(AZ::Name("m_probeGrid.probeBrightnessThreshold"));
            srg->SetConstant(constantIndex, m_probeBrightnessThreshold);

            constantIndex = srgLayout->FindShaderInputConstantIndex(AZ::Name("m_probeGrid.probeIrradianceEncodingGamma"));
            srg->SetConstant(constantIndex, m_probeIrradianceEncodingGamma);

            constantIndex = srgLayout->FindShaderInputConstantIndex(AZ::Name("m_probeGrid.probeInverseIrradianceEncodingGamma"));
            srg->SetConstant(constantIndex, m_probeInverseIrradianceEncodingGamma);

            constantIndex = srgLayout->FindShaderInputConstantIndex(AZ::Name("m_probeGrid.probeNumIrradianceTexels"));
            srg->SetConstant(constantIndex, DefaultNumIrradianceTexels);

            constantIndex = srgLayout->FindShaderInputConstantIndex(AZ::Name("m_probeGrid.probeNumDistanceTexels"));
            srg->SetConstant(constantIndex, DefaultNumDistanceTexels);

            constantIndex = srgLayout->FindShaderInputConstantIndex(AZ::Name("m_probeGrid.normalBias"));
            srg->SetConstant(constantIndex, m_normalBias);

            constantIndex = srgLayout->FindShaderInputConstantIndex(AZ::Name("m_probeGrid.viewBias"));
            srg->SetConstant(constantIndex, m_viewBias);

            constantIndex = srgLayout->FindShaderInputConstantIndex(AZ::Name("m_probeGrid.probeMinFrontfaceDistance"));
            srg->SetConstant(constantIndex, m_probeMinFrontfaceDistance);

            constantIndex = srgLayout->FindShaderInputConstantIndex(AZ::Name("m_probeGrid.probeBackfaceThreshold"));
            srg->SetConstant(constantIndex, m_probeBackfaceThreshold);

            constantIndex = srgLayout->FindShaderInputConstantIndex(AZ::Name("m_probeGrid.probeRayRotationTransform"));
            srg->SetConstant(constantIndex, m_probeRayRotationTransform);
        }

        void DiffuseProbeGrid::UpdateRayTraceSrg(const Data::Asset<RPI::ShaderResourceGroupAsset>& srgAsset, const RPI::Scene* scene)
        {
            if (!m_rayTraceSrg)
            {
                m_rayTraceSrg = RPI::ShaderResourceGroup::Create(srgAsset);
                AZ_Error("DiffuseProbeGrid", m_rayTraceSrg.get(), "Failed to create RayTrace shader resource group");
            }

            const RHI::ShaderResourceGroupLayout* srgLayout = m_rayTraceSrg->GetLayout();
            RHI::ShaderInputImageIndex imageIndex;
            RHI::ShaderInputBufferIndex bufferIndex;
            RHI::ShaderInputConstantIndex constantIndex;

            // TLAS
            RayTracingFeatureProcessor* rayTracingFeatureProcessor = scene->GetFeatureProcessor<RayTracingFeatureProcessor>();
            uint32_t tlasBufferByteCount = aznumeric_cast<uint32_t>(rayTracingFeatureProcessor->GetTlas()->GetTlasBuffer()->GetDescriptor().m_byteCount);
            RHI::BufferViewDescriptor bufferViewDescriptor = RHI::BufferViewDescriptor::CreateRayTracingTLAS(tlasBufferByteCount);

            bufferIndex = srgLayout->FindShaderInputBufferIndex(AZ::Name("m_scene"));
            m_rayTraceSrg->SetBufferView(bufferIndex, rayTracingFeatureProcessor->GetTlas()->GetTlasBuffer()->GetBufferView(bufferViewDescriptor).get());

            // probe raytrace
            imageIndex = srgLayout->FindShaderInputImageIndex(AZ::Name("m_probeRayTrace"));
            m_rayTraceSrg->SetImageView(imageIndex, m_rayTraceImage[m_currentImageIndex]->GetImageView(m_renderData->m_probeRayTraceImageViewDescriptor).get());

            // probe irradiance
            imageIndex = srgLayout->FindShaderInputImageIndex(AZ::Name("m_probeIrradiance"));
            m_rayTraceSrg->SetImageView(imageIndex, m_irradianceImage[m_currentImageIndex]->GetImageView(m_renderData->m_probeIrradianceImageViewDescriptor).get());

            // probe distance
            imageIndex = srgLayout->FindShaderInputImageIndex(AZ::Name("m_probeDistance"));
            m_rayTraceSrg->SetImageView(imageIndex, m_distanceImage[m_currentImageIndex]->GetImageView(m_renderData->m_probeDistanceImageViewDescriptor).get());

            // probe relocation
            imageIndex = srgLayout->FindShaderInputImageIndex(AZ::Name("m_probeOffsets"));
            m_rayTraceSrg->SetImageView(imageIndex, m_relocationImage[m_currentImageIndex]->GetImageView(m_renderData->m_probeRelocationImageViewDescriptor).get());

            // directional lights
            const auto directionalLightFP = scene->GetFeatureProcessor<DirectionalLightFeatureProcessor>();
            bufferIndex = srgLayout->FindShaderInputBufferIndex(AZ::Name("m_directionalLights"));
            m_rayTraceSrg->SetBufferView(bufferIndex, directionalLightFP->GetLightBuffer()->GetBufferView());

            constantIndex = srgLayout->FindShaderInputConstantIndex(AZ::Name("m_directionalLightCount"));
            m_rayTraceSrg->SetConstant(constantIndex, directionalLightFP->GetLightCount());

            // spot lights
            const auto spotLightFP = scene->GetFeatureProcessor<SpotLightFeatureProcessor>();
            bufferIndex = srgLayout->FindShaderInputBufferIndex(AZ::Name("m_spotLights"));
            m_rayTraceSrg->SetBufferView(bufferIndex, spotLightFP->GetLightBuffer()->GetBufferView());

            constantIndex = srgLayout->FindShaderInputConstantIndex(AZ::Name("m_spotLightCount"));
            m_rayTraceSrg->SetConstant(constantIndex, spotLightFP->GetLightCount());

            // point lights
            const auto pointLightFP = scene->GetFeatureProcessor<PointLightFeatureProcessor>();
            bufferIndex = srgLayout->FindShaderInputBufferIndex(AZ::Name("m_pointLights"));
            m_rayTraceSrg->SetBufferView(bufferIndex, pointLightFP->GetLightBuffer()->GetBufferView());

            constantIndex = srgLayout->FindShaderInputConstantIndex(AZ::Name("m_pointLightCount"));
            m_rayTraceSrg->SetConstant(constantIndex, pointLightFP->GetLightCount());

            // grid settings
            constantIndex = srgLayout->FindShaderInputConstantIndex(Name("m_ambientMultiplier"));
            m_rayTraceSrg->SetConstant(constantIndex, m_ambientMultiplier);

            constantIndex = srgLayout->FindShaderInputConstantIndex(Name("m_giShadows"));
            m_rayTraceSrg->SetConstant(constantIndex, m_giShadows);

            SetGridConstants(m_rayTraceSrg);
            m_rayTraceSrg->Compile();
        }

        void DiffuseProbeGrid::UpdateBlendIrradianceSrg(const Data::Asset<RPI::ShaderResourceGroupAsset>& srgAsset)
        {
            if (!m_blendIrradianceSrg)
            {
                m_blendIrradianceSrg = RPI::ShaderResourceGroup::Create(srgAsset);
                AZ_Error("DiffuseProbeGrid", m_blendIrradianceSrg.get(), "Failed to create BlendIrradiance shader resource group");
            }

            const RHI::ShaderResourceGroupLayout* srgLayout = m_blendIrradianceSrg->GetLayout();
            RHI::ShaderInputImageIndex imageIndex;
        
            imageIndex = srgLayout->FindShaderInputImageIndex(AZ::Name("m_probeRayTrace"));
            m_blendIrradianceSrg->SetImageView(imageIndex, m_rayTraceImage[m_currentImageIndex]->GetImageView(m_renderData->m_probeRayTraceImageViewDescriptor).get());
                    
            imageIndex = srgLayout->FindShaderInputImageIndex(AZ::Name("m_probeIrradiance"));
            m_blendIrradianceSrg->SetImageView(imageIndex, m_irradianceImage[m_currentImageIndex]->GetImageView(m_renderData->m_probeIrradianceImageViewDescriptor).get());

            SetGridConstants(m_blendIrradianceSrg);

            // Note: must be compiled every frame since the probe ray transform is changing
            m_blendIrradianceSrg->Compile();
        }

        void DiffuseProbeGrid::UpdateBlendDistanceSrg(const Data::Asset<RPI::ShaderResourceGroupAsset>& srgAsset)
        {
            if (!m_blendDistanceSrg)
            {
                m_blendDistanceSrg = RPI::ShaderResourceGroup::Create(srgAsset);
                AZ_Error("DiffuseProbeGrid", m_blendDistanceSrg.get(), "Failed to create BlendDistance shader resource group");
            }

            const RHI::ShaderResourceGroupLayout* srgLayout = m_blendDistanceSrg->GetLayout();
            RHI::ShaderInputImageIndex imageIndex;

            imageIndex = srgLayout->FindShaderInputImageIndex(AZ::Name("m_probeRayTrace"));
            m_blendDistanceSrg->SetImageView(imageIndex, m_rayTraceImage[m_currentImageIndex]->GetImageView(m_renderData->m_probeRayTraceImageViewDescriptor).get());

            imageIndex = srgLayout->FindShaderInputImageIndex(AZ::Name("m_probeDistance"));
            m_blendDistanceSrg->SetImageView(imageIndex, m_distanceImage[m_currentImageIndex]->GetImageView(m_renderData->m_probeDistanceImageViewDescriptor).get());

            SetGridConstants(m_blendDistanceSrg);

            // Note: must be compiled every frame since the probe ray transform is changing
            m_blendDistanceSrg->Compile();
        }

        void DiffuseProbeGrid::UpdateBorderUpdateSrgs(const Data::Asset<RPI::ShaderResourceGroupAsset>& rowSrgAsset,
                                                      const Data::Asset<RPI::ShaderResourceGroupAsset>& columnSrgAsset)
        {
            // border update row irradiance
            {
                if (!m_borderUpdateRowIrradianceSrg)
                {
                    m_borderUpdateRowIrradianceSrg = RPI::ShaderResourceGroup::Create(rowSrgAsset);
                    AZ_Error("DiffuseProbeGrid", m_borderUpdateRowIrradianceSrg.get(), "Failed to create BorderUpdateRowIrradiance shader resource group");
                }

                const RHI::ShaderResourceGroupLayout* srgLayout = m_borderUpdateRowIrradianceSrg->GetLayout();
                RHI::ShaderInputConstantIndex constantIndex;
                RHI::ShaderInputImageIndex imageIndex;

                imageIndex = srgLayout->FindShaderInputImageIndex(AZ::Name("m_probeTexture"));
                m_borderUpdateRowIrradianceSrg->SetImageView(imageIndex, m_irradianceImage[m_currentImageIndex]->GetImageView(m_renderData->m_probeIrradianceImageViewDescriptor).get());

                constantIndex = srgLayout->FindShaderInputConstantIndex(AZ::Name("m_numTexels"));
                m_borderUpdateRowIrradianceSrg->SetConstant(constantIndex, DefaultNumIrradianceTexels);

                m_borderUpdateRowIrradianceSrg->Compile();
            }

            // border update column irradiance
            {
                if (!m_borderUpdateColumnIrradianceSrg)
                {
                    m_borderUpdateColumnIrradianceSrg = RPI::ShaderResourceGroup::Create(columnSrgAsset);
                    AZ_Error("DiffuseProbeGrid", m_borderUpdateColumnIrradianceSrg.get(), "Failed to create BorderUpdateColumnRowIrradiance shader resource group");
                }

                const RHI::ShaderResourceGroupLayout* srgLayout = m_borderUpdateColumnIrradianceSrg->GetLayout();
                RHI::ShaderInputConstantIndex constantIndex;
                RHI::ShaderInputImageIndex imageIndex;

                imageIndex = srgLayout->FindShaderInputImageIndex(AZ::Name("m_probeTexture"));
                m_borderUpdateColumnIrradianceSrg->SetImageView(imageIndex, m_irradianceImage[m_currentImageIndex]->GetImageView(m_renderData->m_probeIrradianceImageViewDescriptor).get());

                constantIndex = srgLayout->FindShaderInputConstantIndex(AZ::Name("m_numTexels"));
                m_borderUpdateColumnIrradianceSrg->SetConstant(constantIndex, DefaultNumIrradianceTexels);

                m_borderUpdateColumnIrradianceSrg->Compile();
            }

            // border update row distance
            {
                if (!m_borderUpdateRowDistanceSrg)
                {
                    m_borderUpdateRowDistanceSrg = RPI::ShaderResourceGroup::Create(rowSrgAsset);
                    AZ_Error("DiffuseProbeGrid", m_borderUpdateRowDistanceSrg.get(), "Failed to create BorderUpdateRowDistance shader resource group");
                }

                const RHI::ShaderResourceGroupLayout* srgLayout = m_borderUpdateRowDistanceSrg->GetLayout();
                RHI::ShaderInputConstantIndex constantIndex;
                RHI::ShaderInputImageIndex imageIndex;

                imageIndex = srgLayout->FindShaderInputImageIndex(AZ::Name("m_probeTexture"));
                m_borderUpdateRowDistanceSrg->SetImageView(imageIndex, m_distanceImage[m_currentImageIndex]->GetImageView(m_renderData->m_probeDistanceImageViewDescriptor).get());

                constantIndex = srgLayout->FindShaderInputConstantIndex(AZ::Name("m_numTexels"));
                m_borderUpdateRowDistanceSrg->SetConstant(constantIndex, DefaultNumDistanceTexels);

                m_borderUpdateRowDistanceSrg->Compile();
            }

            // border update column distance
            {
                if (!m_borderUpdateColumnDistanceSrg)
                {
                    m_borderUpdateColumnDistanceSrg = RPI::ShaderResourceGroup::Create(columnSrgAsset);
                    AZ_Error("DiffuseProbeGrid", m_borderUpdateColumnDistanceSrg.get(), "Failed to create BorderUpdateColumnRowDistance shader resource group");
                }

                const RHI::ShaderResourceGroupLayout* srgLayout = m_borderUpdateColumnDistanceSrg->GetLayout();
                RHI::ShaderInputConstantIndex constantIndex;
                RHI::ShaderInputImageIndex imageIndex;

                imageIndex = srgLayout->FindShaderInputImageIndex(AZ::Name("m_probeTexture"));
                m_borderUpdateColumnDistanceSrg->SetImageView(imageIndex, m_distanceImage[m_currentImageIndex]->GetImageView(m_renderData->m_probeDistanceImageViewDescriptor).get());

                constantIndex = srgLayout->FindShaderInputConstantIndex(AZ::Name("m_numTexels"));
                m_borderUpdateColumnDistanceSrg->SetConstant(constantIndex, DefaultNumDistanceTexels);

                m_borderUpdateColumnDistanceSrg->Compile();
            }
        }

        void DiffuseProbeGrid::UpdateRelocationSrg(const Data::Asset<RPI::ShaderResourceGroupAsset>& srgAsset)
        {
            if (!m_relocationSrg)
            {
                m_relocationSrg = RPI::ShaderResourceGroup::Create(srgAsset);
                AZ_Error("DiffuseProbeGrid", m_relocationSrg.get(), "Failed to create Relocation shader resource group");
            }

            const RHI::ShaderResourceGroupLayout* srgLayout = m_relocationSrg->GetLayout();
            RHI::ShaderInputConstantIndex constantIndex;
            RHI::ShaderInputImageIndex imageIndex;

            imageIndex = srgLayout->FindShaderInputImageIndex(AZ::Name("m_probeRayTrace"));
            m_relocationSrg->SetImageView(imageIndex, m_rayTraceImage[m_currentImageIndex]->GetImageView(m_renderData->m_probeRayTraceImageViewDescriptor).get());

            imageIndex = srgLayout->FindShaderInputImageIndex(AZ::Name("m_probeRelocation"));
            m_relocationSrg->SetImageView(imageIndex, m_relocationImage[m_currentImageIndex]->GetImageView(m_renderData->m_probeRelocationImageViewDescriptor).get());

            float probeDistanceScale = (aznumeric_cast<float>(m_remainingRelocationIterations) / DefaultNumRelocationIterations);
            constantIndex = srgLayout->FindShaderInputConstantIndex(AZ::Name("m_probeDistanceScale"));
            m_relocationSrg->SetConstant(constantIndex, probeDistanceScale);

            SetGridConstants(m_relocationSrg);

            m_relocationSrg->Compile();
        }

        void DiffuseProbeGrid::UpdateRenderObjectSrg()
        {
            if (!m_updateRenderObjectSrg)
            {
                return;
            }

            if (!m_renderObjectSrg)
            {
                m_renderObjectSrg = RPI::ShaderResourceGroup::Create(m_renderData->m_srgAsset);
                AZ_Error("DiffuseProbeGrid", m_renderObjectSrg.get(), "Failed to create render shader resource group");
            }

            const RHI::ShaderResourceGroupLayout* srgLayout = m_renderObjectSrg->GetLayout();
            RHI::ShaderInputConstantIndex constantIndex;
            RHI::ShaderInputImageIndex imageIndex;

            constantIndex = srgLayout->FindShaderInputConstantIndex(Name("m_modelToWorld"));
            AZ::Matrix3x4 modelToWorld = AZ::Matrix3x4::CreateFromMatrix3x3AndTranslation(Matrix3x3::CreateIdentity(), m_position) * AZ::Matrix3x4::CreateScale(m_extents);
            m_renderObjectSrg->SetConstant(constantIndex, modelToWorld);

            constantIndex = srgLayout->FindShaderInputConstantIndex(Name("m_aabbMin"));
            m_renderObjectSrg->SetConstant(constantIndex, m_aabbWs.GetMin());

            constantIndex = srgLayout->FindShaderInputConstantIndex(Name("m_aabbMax"));
            m_renderObjectSrg->SetConstant(constantIndex, m_aabbWs.GetMax());

            constantIndex = srgLayout->FindShaderInputConstantIndex(Name("m_enableDiffuseGI"));
            m_renderObjectSrg->SetConstant(constantIndex, m_enabled);

            constantIndex = srgLayout->FindShaderInputConstantIndex(Name("m_ambientMultiplier"));
            m_renderObjectSrg->SetConstant(constantIndex, m_ambientMultiplier);

            imageIndex = srgLayout->FindShaderInputImageIndex(Name("m_probeIrradiance"));
            m_renderObjectSrg->SetImageView(imageIndex, m_irradianceImage[m_currentImageIndex]->GetImageView(m_renderData->m_probeIrradianceImageViewDescriptor).get());

            imageIndex = srgLayout->FindShaderInputImageIndex(Name("m_probeDistance"));
            m_renderObjectSrg->SetImageView(imageIndex, m_distanceImage[m_currentImageIndex]->GetImageView(m_renderData->m_probeDistanceImageViewDescriptor).get());

            imageIndex = srgLayout->FindShaderInputImageIndex(Name("m_probeOffsets"));
            m_renderObjectSrg->SetImageView(imageIndex, m_relocationImage[m_currentImageIndex]->GetImageView(m_renderData->m_probeRelocationImageViewDescriptor).get());

            SetGridConstants(m_renderObjectSrg);

            m_renderObjectSrg->Compile();
            m_updateRenderObjectSrg = false;
        }

        AZ::Matrix4x4 DiffuseProbeGrid::ComputeRandomRotation()
        {
            // Fast Random Rotation Matrices by James Arvo
            // http://citeseerx.ist.psu.edu/viewdoc/download?doi=10.1.1.53.1357&rep=rep1&type=pdf
            // http://www.realtimerendering.com/resources/GraphicsGems/gemsiii/rand_rotation.c
            float x1 = m_random.GetRandomFloat();
            float x2 = m_random.GetRandomFloat();
            float x3 = m_random.GetRandomFloat();

            float theta = 2.0f * AZ::Constants::Pi * x1;
            float phi = 2.0f * AZ::Constants::Pi * x2;
            float z = 2.0f * x3;
            float r = AZ::Sqrt(z);
            AZ::Vector3 V(AZ::Sin(phi) * r, AZ::Cos(phi) * r, AZ::Sqrt(2.0f - z));

            float sinTheta = AZ::Sin(theta);
            float cosTheta = AZ::Cos(theta);
            AZ::Vector3 S(V.GetX() * cosTheta - V.GetY() * sinTheta, V.GetX() * sinTheta + V.GetY() * cosTheta, V.GetZ());

            AZ::Matrix4x4 transform;
            transform.SetRow(0, AZ::Vector4(V.GetX() * S.GetX() - cosTheta, V.GetX() * S.GetY() - sinTheta, V.GetX() * V.GetZ(), 0.0f));
            transform.SetRow(1, AZ::Vector4(V.GetY() * S.GetX() + sinTheta, V.GetY() * S.GetY() - cosTheta, V.GetY() * V.GetZ(), 0.0f));
            transform.SetRow(2, AZ::Vector4(V.GetZ() * S.GetX(), V.GetZ() * S.GetY(), 1.0f - z, 0.0f));
            transform.SetRow(3, AZ::Vector4(0.0f, 0.0f, 0.0f, 1.0f));
            return transform;
        }
    }   // namespace Render
}   // namespace AZ
