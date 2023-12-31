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

#include <RHI/PipelineLayout.h>
#include <Atom/RHI/PipelineState.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

namespace AZ
{
    namespace DX12
    {
        struct PipelineStateDrawData
        {
            RHI::MultisampleState m_multisampleState;
            RHI::PrimitiveTopology m_primitiveTopology = RHI::PrimitiveTopology::Undefined;
        };

        struct PipelineStateData
        {
            PipelineStateData()
                : m_type(RHI::PipelineStateType::Draw)
            {}

            RHI::PipelineStateType m_type;
            union
            {
                // Only draw data for now.
                PipelineStateDrawData m_drawData;
            };
        };        

        class PipelineState final
            : public RHI::PipelineState
        {
            friend class PipelineStatePool;
        public:
            AZ_CLASS_ALLOCATOR(PipelineState, AZ::SystemAllocator, 0);

            static RHI::Ptr<PipelineState> Create();

            /// Returns the pipeline layout associated with this PSO.
            const PipelineLayout& GetPipelineLayout() const;

            /// Returns the platform pipeline state object.
            ID3D12PipelineState* Get() const;

            const PipelineStateData& GetPipelineStateData() const;

        private:
            PipelineState() = default;

            //////////////////////////////////////////////////////////////////////////
            // RHI::PipelineState
            RHI::ResultCode InitInternal(RHI::Device& device, const RHI::PipelineStateDescriptorForDraw& descriptor, RHI::PipelineLibrary* pipelineLibrary) override;
            RHI::ResultCode InitInternal(RHI::Device& device, const RHI::PipelineStateDescriptorForDispatch& descriptor, RHI::PipelineLibrary* pipelineLibrary) override;
            RHI::ResultCode InitInternal(RHI::Device& device, const RHI::PipelineStateDescriptorForRayTracing& descriptor, RHI::PipelineLibrary* pipelineLibrary) override;
            void ShutdownInternal() override;
            //////////////////////////////////////////////////////////////////////////

            RHI::ConstPtr<PipelineLayout> m_pipelineLayout;
            RHI::Ptr<ID3D12PipelineState> m_pipelineState;
            PipelineStateData m_pipelineStateData;
        };
    }
}
