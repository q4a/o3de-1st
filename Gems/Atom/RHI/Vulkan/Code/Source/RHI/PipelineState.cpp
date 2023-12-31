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
#include "Atom_RHI_Vulkan_precompiled.h"
#include <Atom/RHI.Reflect/PipelineLayoutDescriptor.h>
#include <RHI/DescriptorPool.h>
#include <RHI/DescriptorSetLayout.h>
#include <RHI/Device.h>
#include <RHI/GraphicsPipeline.h>
#include <RHI/ComputePipeline.h>
#include <RHI/PipelineLayout.h>
#include <RHI/PipelineLibrary.h>
#include <RHI/PipelineState.h>

namespace AZ
{
    namespace Vulkan
    {
        RHI::Ptr<PipelineState> PipelineState::Create()
        {
            return aznew PipelineState();
        }

        PipelineLayout* PipelineState::GetPipelineLayout() const
        {
            return m_pipeline->GetPipelineLayout();
        }

        Pipeline* PipelineState::GetPipeline() const
        {
            return m_pipeline.get();
        }

        PipelineLibrary* PipelineState::GetPipelineLibrary() const
        {
            return m_pipeline->GetPipelineLibrary();
        }

        RHI::ResultCode PipelineState::InitInternal(RHI::Device& device, const RHI::PipelineStateDescriptorForDraw& descriptor, RHI::PipelineLibrary* pipelineLibrary)
        {
            Pipeline::Descriptor pipelineDescriptor;
            pipelineDescriptor.m_pipelineDescritor = &descriptor;
            pipelineDescriptor.m_device = static_cast<Device*>(&device);
            pipelineDescriptor.m_pipelineLibrary = static_cast<PipelineLibrary*>(pipelineLibrary);
            RHI::Ptr<GraphicsPipeline> pipeline = GraphicsPipeline::Create();
            RHI::ResultCode result = pipeline->Init(pipelineDescriptor);
            RETURN_RESULT_IF_UNSUCCESSFUL(result);

            m_pipeline = pipeline;            
            return result;
        }

        RHI::ResultCode PipelineState::InitInternal(RHI::Device& device, const RHI::PipelineStateDescriptorForDispatch& descriptor, RHI::PipelineLibrary* pipelineLibrary)
        {
            Pipeline::Descriptor pipelineDescriptor;
            pipelineDescriptor.m_pipelineDescritor = &descriptor;
            pipelineDescriptor.m_device = static_cast<Device*>(&device);
            pipelineDescriptor.m_pipelineLibrary = static_cast<PipelineLibrary*>(pipelineLibrary);
            RHI::Ptr<ComputePipeline> pipeline = ComputePipeline::Create();
            RHI::ResultCode result = pipeline->Init(pipelineDescriptor);
            RETURN_RESULT_IF_UNSUCCESSFUL(result);

            m_pipeline = pipeline;
            return result;
        }

        RHI::ResultCode PipelineState::InitInternal([[maybe_unused]] RHI::Device& device, [[maybe_unused]] const RHI::PipelineStateDescriptorForRayTracing& descriptor, [[maybe_unused]] RHI::PipelineLibrary* pipelineLibrary)
        {
            // [GFX TODO][ATOM-5151] Implement Vulkan-RT
            AZ_Assert(false, "Not implemented");
            return RHI::ResultCode::Fail;
        }

        void PipelineState::ShutdownInternal()
        {
            auto& device = static_cast<Device&>(GetDevice());
            if (m_pipeline)
            {
                device.QueueForRelease(m_pipeline);
                m_pipeline = nullptr;
            }
        }
    }
}
