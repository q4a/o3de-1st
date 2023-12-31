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

#include <Atom/RPI.Public/AuxGeom/AuxGeomFeatureProcessorInterface.h>

namespace AZ
{
    namespace Render
    {
        class AuxGeomDrawQueue;
        class DynamicPrimitiveProcessor;
        class FixedShapeProcessor;

        /**
         * FeatureProcessor for Auxiliary Geometry
         */
        class AuxGeomFeatureProcessor final
            : public RPI::AuxGeomFeatureProcessorInterface
        {
        public: // functions

            AZ_RTTI(AuxGeomFeatureProcessor, "{75E17417-C8E3-4B64-8469-7662D1E0904A}", RPI::AuxGeomFeatureProcessorInterface);
            AZ_FEATURE_PROCESSOR(AuxGeomFeatureProcessor);

            static void Reflect(AZ::ReflectContext* context);

            AuxGeomFeatureProcessor() = default;
            virtual ~AuxGeomFeatureProcessor() = default;

            // RPI::FeatureProcessor
            void Activate() override;
            void Deactivate() override;
            void Render(const FeatureProcessor::RenderPacket& fpPacket) override;

            // RPI::AuxGeomFeatureProcessorInterface
            RPI::AuxGeomDrawPtr GetDrawQueue() override; // returns the scene DrawQueue
            RPI::AuxGeomDrawPtr GetDrawQueueForView(const RPI::View* view) override;

            RPI::AuxGeomDrawPtr GetOrCreateDrawQueueForView(const RPI::View* view) override;
            void ReleaseDrawQueueForView(const RPI::View* view) override;

            // RPI::SceneNotificationBus::Handler overrides...
            void OnRenderPipelineAdded(RPI::RenderPipelinePtr pipeline) override;
            void OnRenderPipelineRemoved(RPI::RenderPipeline* pipeline) override;

        private: // functions

            AuxGeomFeatureProcessor(const AuxGeomFeatureProcessor&) = delete;
            void OnSceneRenderPipelinesChanged();

        private: // data

            static const char* s_featureProcessorName;
            
            //! Cache a pointer to the AuxGeom draw queue for our scene
            RPI::AuxGeomDrawPtr m_sceneDrawQueue = nullptr;

            //! Map used to store the AuxGeomDrawQueue & DynamicPrimitiveProcessor for each view
            // [GFX TODO][ATOM-4435] remove DynamicPrimitiveProcessor per view if we can get orphan buffers to support multiple
            // orphanings per frame. 
            // Only the DPP suffers from the issue so no need for a per view FixedShapeProcessor. 
            struct ViewDrawData
            {
                RPI::AuxGeomDrawPtr m_drawQueue;
                AZStd::unique_ptr<DynamicPrimitiveProcessor> m_dynPrimProc;
            };
            AZStd::map<const RPI::View*, ViewDrawData> m_viewDrawDataMap; // using View* as key to not hold a reference to the view

            //! The object that handles the dynamic primitive geometry data
            AZStd::unique_ptr<DynamicPrimitiveProcessor> m_dynamicPrimitiveProcessor;

            //! The object that handles fixed shape geometry data
            AZStd::unique_ptr<FixedShapeProcessor> m_fixedShapeProcessor;
        };

        inline RPI::AuxGeomDrawPtr AuxGeomFeatureProcessor::GetDrawQueue()
        {
            return m_sceneDrawQueue;
        }

    } // namespace Render
} // namespace AZ
