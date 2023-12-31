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

#include <Atom/RPI.Public/PipelineState.h>
#include <Atom/Feature/TransformService/TransformServiceFeatureProcessor.h>
#include <Atom/RPI.Public/Scene.h>
#include <AzCore/Math/Random.h>
#include <AzCore/Math/Aabb.h>
#include <Atom/RHI/DrawPacketBuilder.h>

namespace AZ
{
    namespace Render
    {
        class DiffuseProbeGridFeatureProcessor;

        struct DiffuseProbeGridRenderData
        {
            // image pool
            RHI::Ptr<RHI::ImagePool> m_imagePool;

            AZStd::array<RHI::StreamBufferView, 1> m_boxPositionBufferView;
            RHI::IndexBufferView m_boxIndexBufferView;
            uint32_t m_boxIndexCount = 0;

            // image views
            RHI::ImageViewDescriptor m_probeRayTraceImageViewDescriptor;
            RHI::ImageViewDescriptor m_probeIrradianceImageViewDescriptor;
            RHI::ImageViewDescriptor m_probeDistanceImageViewDescriptor;
            RHI::ImageViewDescriptor m_probeRelocationImageViewDescriptor;

            // render pipeline state
            RPI::Ptr<RPI::PipelineStateForDraw> m_pipelineState;

            // render Srg asset
            Data::Asset<RPI::ShaderResourceGroupAsset> m_srgAsset;

            // render drawlist tag
            RHI::DrawListTag m_drawListTag;
        };

        //! This class manages contains the functionality necessary to update diffuse probes and
        //! generate diffuse global illumination.
        class DiffuseProbeGrid final
        {
        public:
            DiffuseProbeGrid() = default;
            ~DiffuseProbeGrid();

            void Init(RPI::Scene* scene, DiffuseProbeGridRenderData* diffuseProbeGridRenderData);
            void Simulate(RPI::Scene* scene, uint32_t probeIndex);
            void Render(RPI::ViewPtr view);

            void SetTransform(const AZ::Transform& transform);

            bool ValidateExtents(const AZ::Vector3& newExtents);
            const AZ::Vector3& GetExtents() const { return m_extents; }
            void SetExtents(const AZ::Vector3& extents);

            const AZ::Aabb& GetAabbWs() const { return m_aabbWs; }

            bool ValidateProbeSpacing(const AZ::Vector3& newSpacing);
            const AZ::Vector3& GetProbeSpacing() const { return m_probeSpacing; }
            void SetProbeSpacing(const AZ::Vector3& probeSpacing);

            float GetNormalBias() const { return m_normalBias; }
            void SetNormalBias(float normalBias);

            float GetViewBias() const { return m_viewBias; }
            void SetViewBias(float viewBias);

            float GetAmbientMultiplier() const { return m_ambientMultiplier; }
            void SetAmbientMultiplier(float ambientMultiplier);

            void Enable(bool enabled);

            bool GetGIShadows() const { return m_giShadows; }
            void SetGIShadows(bool giShadows) { m_giShadows = giShadows; }

            uint32_t GetNumRaysPerProbe() const { return m_numRaysPerProbe; }

            uint32_t GetRemainingRelocationIterations() const { return m_remainingRelocationIterations; }
            void DecrementRemainingRelocationIterations() { --m_remainingRelocationIterations; }
            void ResetRemainingRelocationIterations() { m_remainingRelocationIterations = DefaultNumRelocationIterations; }

            // compute total number of probes in the grid
            uint32_t GetTotalProbeCount() const;

            // compute probe counts for a 2D texture layout
            void GetTexture2DProbeCount(uint32_t& probeCountX, uint32_t& probeCountY) const;

            // apply probe grid settings to a Srg
            void SetGridConstants(Data::Instance<RPI::ShaderResourceGroup>& srg);            

            // Srgs
            const Data::Instance<RPI::ShaderResourceGroup>& GetRayTraceSrg() { return m_rayTraceSrg; }
            const Data::Instance<RPI::ShaderResourceGroup>& GetBlendIrradianceSrg() { return m_blendIrradianceSrg; }
            const Data::Instance<RPI::ShaderResourceGroup>& GetBlendDistanceSrg() { return m_blendDistanceSrg; }
            const Data::Instance<RPI::ShaderResourceGroup>& GetBorderUpdateRowIrradianceSrg() { return m_borderUpdateRowIrradianceSrg; }
            const Data::Instance<RPI::ShaderResourceGroup>& GetBorderUpdateColumnIrradianceSrg() { return m_borderUpdateColumnIrradianceSrg; }
            const Data::Instance<RPI::ShaderResourceGroup>& GetBorderUpdateRowDistanceSrg() { return m_borderUpdateRowDistanceSrg; }
            const Data::Instance<RPI::ShaderResourceGroup>& GetBorderUpdateColumnDistanceSrg() { return m_borderUpdateColumnDistanceSrg; }
            const Data::Instance<RPI::ShaderResourceGroup>& GetRelocationSrg() { return m_relocationSrg; }

            // Srg updates
            void UpdateRayTraceSrg(const Data::Asset<RPI::ShaderResourceGroupAsset>& srgAsset, const RPI::Scene* scene);
            void UpdateBlendIrradianceSrg(const Data::Asset<RPI::ShaderResourceGroupAsset>& srgAsset);
            void UpdateBlendDistanceSrg(const Data::Asset<RPI::ShaderResourceGroupAsset>& srgAsset);
            void UpdateBorderUpdateSrgs(const Data::Asset<RPI::ShaderResourceGroupAsset>& rowSrgAsset, const Data::Asset<RPI::ShaderResourceGroupAsset>& columnSrgAsset);
            void UpdateRelocationSrg(const Data::Asset<RPI::ShaderResourceGroupAsset>& srgAsset);
            void UpdateRenderObjectSrg();

            // textures
            const RHI::Ptr<RHI::Image>& GetRayTraceImage() { return m_rayTraceImage[m_currentImageIndex]; }
            const RHI::Ptr<RHI::Image>& GetIrradianceImage() { return m_irradianceImage[m_currentImageIndex]; }
            const RHI::Ptr<RHI::Image>& GetDistanceImage() { return m_distanceImage[m_currentImageIndex]; }
            const RHI::Ptr<RHI::Image>& GetRelocationImage() { return m_relocationImage[m_currentImageIndex]; }

            // attachment Ids
            const RHI::AttachmentId GetRayTraceImageAttachmentId() const { return m_rayTraceImageAttachmentId; }
            const RHI::AttachmentId GetIrradianceImageAttachmentId() const { return m_irradianceImageAttachmentId; }
            const RHI::AttachmentId GetDistanceImageAttachmentId() const { return m_distanceImageAttachmentId; }
            const RHI::AttachmentId GetRelocationImageAttachmentId() const { return m_relocationImageAttachmentId; }

            const DiffuseProbeGridRenderData* GetRenderData() const { return m_renderData; }

            static constexpr uint32_t DefaultNumIrradianceTexels = 6;
            static constexpr uint32_t DefaultNumDistanceTexels = 14;
            static constexpr uint32_t DefaultNumRelocationIterations = 100;

        private:
            void UpdateTextures();
            void ComputeProbeCount(const AZ::Vector3& extents, const AZ::Vector3& probeSpacing, uint32_t& probeCountX, uint32_t& probeCountY, uint32_t& probeCountZ);
            bool ValidateProbeCount(const AZ::Vector3& extents, const AZ::Vector3& probeSpacing);
            void UpdateProbeCount();
            Matrix4x4 ComputeRandomRotation();

            // transform service
            TransformServiceFeatureProcessor::ObjectId m_objectId;
            TransformServiceFeatureProcessor* m_transformService = nullptr;

            // probe grid position
            AZ::Vector3 m_position = AZ::Vector3(0.0f, 0.0f, 0.0f);

            // extents of the probe grid
            AZ::Vector3 m_extents = AZ::Vector3(0.0f, 0.0f, 0.0f);

            // probe grid AABB (world space), built from position and extents
            AZ::Aabb m_aabbWs = AZ::Aabb::CreateNull();

            // per-axis spacing of probes in the grid
            AZ::Vector3 m_probeSpacing;

            // per-axis number of probes in the grid
            uint32_t m_probeCountX = 0;
            uint32_t m_probeCountY = 0;
            uint32_t m_probeCountZ = 0;

            // grid settings
            bool     m_enabled = true;
            float    m_normalBias = 0.6f;
            float    m_viewBias = 0.01f;
            uint32_t m_numRaysPerProbe = 288;
            float    m_probeMaxRayDistance = 10.0f;
            float    m_probeDistanceExponent = 50.0f;
            float    m_probeHysteresis = 0.95f;
            float    m_probeChangeThreshold = 0.2f;
            float    m_probeBrightnessThreshold = 1.0f;
            float    m_probeIrradianceEncodingGamma = 5.0f;
            float    m_probeInverseIrradianceEncodingGamma = 1.0f / m_probeIrradianceEncodingGamma;
            float    m_probeMinFrontfaceDistance = 1.0f;
            float    m_probeBackfaceThreshold = 0.25f;
            float    m_ambientMultiplier = 1.0f;
            bool     m_giShadows = true;

            // rotation transform applied to probe rays
            AZ::Matrix4x4 m_probeRayRotationTransform;
            AZ::SimpleLcgRandom m_random;

            // probe relocation settings
            uint32_t m_remainingRelocationIterations = DefaultNumRelocationIterations;

            // render data
            DiffuseProbeGridRenderData* m_renderData = nullptr;

            // render draw packet
            RHI::ConstPtr<RHI::DrawPacket> m_drawPacket;

            // sort key for the draw item
            const RHI::DrawItemSortKey InvalidSortKey = static_cast<RHI::DrawItemSortKey>(-1);
            RHI::DrawItemSortKey m_sortKey = InvalidSortKey;

            // textures
            static const uint32_t MaxTextureDimension = 8192;
            static const uint32_t ImageFrameCount = 3;
            RHI::Ptr<RHI::Image> m_rayTraceImage[ImageFrameCount];
            RHI::Ptr<RHI::Image> m_irradianceImage[ImageFrameCount];
            RHI::Ptr<RHI::Image> m_distanceImage[ImageFrameCount];
            RHI::Ptr<RHI::Image> m_relocationImage[ImageFrameCount];
            uint32_t m_currentImageIndex = 0;
            bool m_updateTextures = false;

            // Srgs
            Data::Instance<RPI::ShaderResourceGroup> m_rayTraceSrg;
            Data::Instance<RPI::ShaderResourceGroup> m_blendIrradianceSrg;
            Data::Instance<RPI::ShaderResourceGroup> m_blendDistanceSrg;
            Data::Instance<RPI::ShaderResourceGroup> m_borderUpdateRowIrradianceSrg;
            Data::Instance<RPI::ShaderResourceGroup> m_borderUpdateColumnIrradianceSrg;
            Data::Instance<RPI::ShaderResourceGroup> m_borderUpdateRowDistanceSrg;
            Data::Instance<RPI::ShaderResourceGroup> m_borderUpdateColumnDistanceSrg;
            Data::Instance<RPI::ShaderResourceGroup> m_relocationSrg;
            Data::Instance<RPI::ShaderResourceGroup> m_renderObjectSrg;
            bool m_updateRenderObjectSrg = true;

            // attachment Ids
            RHI::AttachmentId m_rayTraceImageAttachmentId;
            RHI::AttachmentId m_irradianceImageAttachmentId;
            RHI::AttachmentId m_distanceImageAttachmentId;
            RHI::AttachmentId m_relocationImageAttachmentId;
        };
    }   // namespace Render
}   // namespace AZ
