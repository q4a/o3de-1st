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

#include <Atom/Feature/CoreLights/CoreLightsConstants.h>
#include <Atom/RPI.Public/Pass/ParentPass.h>
#include <AtomCore/std/containers/array_view.h>
#include <AzCore/std/containers/vector.h>
#include <CoreLights/ShadowmapAtlas.h>
#include <CoreLights/ShadowmapPass.h>


namespace AZ
{
    namespace Render
    {
        //! SpotLightShadowmapsPass owns ShadowmapPasses for Spot Lights.
        class SpotLightShadowmapsPass final
            : public RPI::ParentPass
        {
            AZ_RPI_PASS(SpotLightShadowmapsPass);
            using Base = RPI::ParentPass;

        public:
            AZ_CLASS_ALLOCATOR(SpotLightShadowmapsPass, SystemAllocator, 0);
            AZ_RTTI(SpotLightShadowmapsPass, "00024B13-1095-40FA-BEC3-B0F68110BEA2", Base);

            static constexpr uint16_t InvalidIndex = ~0;
            struct ShadowmapSizeWithIndices
            {
                ShadowmapSize m_size = ShadowmapSize::None;
                uint16_t m_shadowIndexInSrg = InvalidIndex;
            };

            virtual ~SpotLightShadowmapsPass();
            static RPI::Ptr<SpotLightShadowmapsPass> Create(const RPI::PassDescriptor& descriptor);

            //! This returns true if this pass is of the given render pipeline. 
            bool IsOfRenderPipeline(const RPI::RenderPipeline& renderPipeline) const;

            //! This returns the pipeline view tag used in shadowmap passes.
            const RPI::PipelineViewTag& GetPipelineViewTagOfChild(size_t childIndex);

            //! This update shadowmap sizes for each spot light index.
            //! @param sizes shadowmap sizes for each spot light index.
            void UpdateShadowmapSizes(const AZStd::vector<ShadowmapSizeWithIndices>& sizes);

            //! This returns the image size(width/height) of shadowmap atlas.
            //! @return image size of the shadowmap atlas.
            ShadowmapSize GetShadowmapAtlasSize() const;

            //! This returns the origin of shadowmap in the atlas.
            //! @param lightIndex index of light in SRG.
            //! @return array slice and origin in the slice for the light.
            ShadowmapAtlas::Origin GetOriginInAtlas(uint16_t index) const;

            //! This exposes the shadowmap atlas.
            ShadowmapAtlas& GetShadowmapAtlas();

        private:
            SpotLightShadowmapsPass() = delete;
            explicit SpotLightShadowmapsPass(const RPI::PassDescriptor& descriptor);

            // RPI::Pass overrides...
            void BuildAttachmentsInternal() override;
            void GetPipelineViewTags(RPI::SortedPipelineViewTags& outTags) const override;
            void GetViewDrawListInfo(RHI::DrawListMask& outDrawListMask, RPI::PassesByDrawList& outPassesByDrawList, const RPI::PipelineViewTag& viewTag) const override;

            RHI::Ptr<ShadowmapPass> CreateChild(size_t childIndex);

            void UpdateChildren();
            void SetChildrenCount(size_t count);
            
            const Name m_slotName{ "Shadowmap" };
            Name m_pipelineViewTagBase;
            Name m_drawListTagName;
            RHI::DrawListTag m_drawListTag;
            AZStd::vector<RPI::PipelineViewTag> m_childrenPipelineViewTags;
            AZStd::vector<ShadowmapSizeWithIndices> m_sizes;

            ShadowmapAtlas m_atlas;
            bool m_updateChildren = true;
        };
    } // namespace Render
} // namespace AZ
