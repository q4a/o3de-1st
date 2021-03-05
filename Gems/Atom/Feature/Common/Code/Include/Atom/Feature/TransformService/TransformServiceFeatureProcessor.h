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

#include <Atom/Feature/TransformService/TransformServiceFeatureProcessorInterface.h>
#include <Atom/RHI/Buffer.h>
#include <Atom/RHI/BufferPool.h>
#include <Atom/RPI.Public/FeatureProcessor.h>
#include <Atom/RPI.Public/Shader/ShaderResourceGroup.h>

namespace AZ
{
    namespace Render
    {
        //! This feature processor handles static and dynamic non-skinned meshes.
        class TransformServiceFeatureProcessor final
            : public TransformServiceFeatureProcessorInterface
        {
        public:

            AZ_RTTI(AZ::Render::TransformServiceFeatureProcessor, "{D8A2C353-2850-42F8-AA21-3979CBECBF80}", AZ::Render::TransformServiceFeatureProcessorInterface);

            static void Reflect(AZ::ReflectContext* context);

            TransformServiceFeatureProcessor() = default;
            virtual ~TransformServiceFeatureProcessor() = default;

            // FeatureProcessor overrides ...
            //! Creates pools, buffers, and buffer views
            void Activate() override;
            //! Releases GPU resources.
            void Deactivate() override;
            //! Binds buffers
            void Render(const FeatureProcessor::RenderPacket& packet) override;

            // RPI::SceneNotificationBus overrides ...
            void OnBeginPrepareRender() override;
            void OnEndPrepareRender() override;

            // TransformServiceFeatureProcessorInterface overrides ...
            ObjectId ReserveObjectId() override;
            void ReleaseObjectId(ObjectId& id) override;
            void SetTransformForId(ObjectId id, const AZ::Transform& transform) override;
            AZ::Transform GetTransformForId(ObjectId id) const override;

        private:

            // Holds both regular 4x3 transforms and 3x3 normal transforms with padding at the end of each float3.
            union Float4x3
            {
                float m_transform[12] = { 0.0f };
                uint32_t m_nextFreeSlot;
            };

            // Flag value for when the buffers have no empty spaces.
            static const uint32_t NoAvailableTransformIndices = -1;

            TransformServiceFeatureProcessor(const TransformServiceFeatureProcessor&) = delete;

            // Prepare GPU buffers for object transformation matrices
            // Create the buffers if they don't exist. Otherwise, resize them if they are not large enough for the matrices
            void PrepareBuffers();
            
            Data::Instance<RPI::ShaderResourceGroup> m_sceneSrg;
            RHI::ShaderInputBufferIndex m_objectToWorldBufferIndex;
            RHI::ShaderInputBufferIndex m_objectToWorldInverseTransposeBufferIndex;
            RHI::ShaderInputBufferIndex m_objectToWorldHistoryBufferIndex;

            // Stores transforms that are uploaded to a GPU buffer. Used slots have float12(matrix3x4) values, empty slots
            // have a uint32_t that points to the next empty slot like a linked list. m_firstAvailableMeshTransformIndex stores the first
            // empty slot, unless there are none then it's NoAvailableTransformIndices. This allows mesh object SRGs to be compiled once
            // with an index to their transform, and updates to the transform just update the buffer, not individual mesh SRGs.
            AZStd::vector<Float4x3> m_objectToWorldTransforms;
            AZStd::vector<Float4x3> m_objectToWorldInverseTransposeTransforms;
            AZStd::vector<Float4x3> m_objectToWorldHistoryTransforms;

            static const size_t TransformValueSize = sizeof(decltype(m_objectToWorldTransforms)::value_type);
            static const size_t NormalValueSize = sizeof(decltype(m_objectToWorldInverseTransposeTransforms)::value_type);

            Data::Instance<RPI::Buffer> m_objectToWorldBuffer;
            Data::Instance<RPI::Buffer> m_objectToWorldInverseTransposeBuffer;
            Data::Instance<RPI::Buffer> m_objectToWorldHistoryBuffer;

            uint32_t m_firstAvailableTransformIndex = NoAvailableTransformIndices;
            bool m_deviceBufferNeedsUpdate = false;
            bool m_historyBufferNeedsUpdate = false;
            bool m_isWriteable = true;     //prevents write access during certain parts of the frame (for threadsafety)
        };
    }
}