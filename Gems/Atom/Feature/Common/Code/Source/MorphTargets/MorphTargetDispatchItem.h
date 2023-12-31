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

#include <Atom/Feature/MorphTargets/MorphTargetInputBuffers.h>

#include <Atom/RPI.Public/Shader/ShaderReloadNotificationBus.h>
#include <Atom/RHI/DispatchItem.h>
#include <Atom/RHI/ConstantsData.h>
#include <AtomCore/Instance/Instance.h>

namespace AZ
{
    namespace RHI
    {
        class BufferView;
        class PipelineState;
    }

    namespace RPI
    {
        class Buffer;
        class ModelLod;
        class Shader;
        class ShaderResourceGroup;
    }

    namespace Render
    {
        class MorphTargetComputePass;

        //! Holds and manages an RHI DispatchItem for a specific morph target, and the resources that are needed to build and maintain it.
        class MorphTargetDispatchItem
            : private RPI::ShaderReloadNotificationBus::Handler
        {
        public:
            AZ_CLASS_ALLOCATOR(MorphTargetDispatchItem, AZ::SystemAllocator, 0);

            MorphTargetDispatchItem() = delete;
            //! Create one dispatch item per morph target
            explicit MorphTargetDispatchItem(
                const AZStd::intrusive_ptr<MorphTargetInputBuffers> inputBuffers,
                const MorphTargetMetaData& morphTargetMetaData,
                RPI::Ptr<MorphTargetComputePass> morphTargetComputePass,
                uint32_t accumulatedDeltaOffsetInBytes,
                float accumulatedDeltaRange
            );
            ~MorphTargetDispatchItem();

            // The event handler cannot be copied
            AZ_DISABLE_COPY_MOVE(MorphTargetDispatchItem);

            bool Init();

            const RHI::DispatchItem& GetRHIDispatchItem() const;

            void SetWeight(float weight);
            float GetWeight() const;
        private:
            bool InitPerInstanceSRG();
            void InitRootConstants(const RHI::ConstantsLayout* rootConstantsLayout);

            // ShaderInstanceNotificationBus::Handler overrides
            void OnShaderReinitialized(const RPI::Shader& shader) override;

            RHI::DispatchItem m_dispatchItem;

            // The morph target shader used for this instance
            Data::Instance<RPI::Shader> m_morphTargetShader;

            // The vertex deltas
            AZStd::intrusive_ptr<MorphTargetInputBuffers> m_inputBuffers;

            // The per-object shader resource group
            Data::Instance<RPI::ShaderResourceGroup> m_instanceSrg;

            // Metadata used to set the root constants for the shader
            MorphTargetMetaData m_morphTargetMetaData;

            AZ::RHI::ConstantsData m_rootConstantData;

            // Byte offset into the SkinnedMeshOutputVertexStream buffer to a location to write the accumulated morph target deltas
            uint32_t m_accumulatedDeltaOffsetInBytes;
            // A conservative value for encoding/decoding the accumulated deltas
            float m_accumulatedDeltaIntegerEncoding;

            // Keep track of the constant index of m_weight since it is updated frequently
            RHI::ShaderInputConstantIndex m_weightIndex;
        };

        float ComputeMorphTargetIntegerEncoding(const AZStd::vector<MorphTargetMetaData>& morphTargetMetaDatas);
    } // namespace Render
} // namespace AZ
