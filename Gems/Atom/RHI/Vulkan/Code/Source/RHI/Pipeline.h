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

#include <Atom/RHI/DeviceObject.h>
#include <Atom/RHI/PipelineStateDescriptor.h>
#include <Atom/RHI.Reflect/PipelineLayoutDescriptor.h>
#include <AzCore/Memory/PoolAllocator.h>
#include <AzCore/std/containers/list.h>
#include <RHI/PipelineLayout.h>
#include <RHI/ShaderModule.h>

namespace AZ
{
    namespace Vulkan
    {
        class ShaderStageFunction;
        class PipelineLibrary;

        class Pipeline
            : public RHI::DeviceObject
        {
            using Base = RHI::DeviceObject;

        public:
            AZ_CLASS_ALLOCATOR(Pipeline, AZ::ThreadPoolAllocator, 0);
            AZ_RTTI(Pipeline, "A5FCFA0C-D5B3-4833-A572-BCE8FB612C13", Base);

            ~Pipeline() = default;

            struct Descriptor
            {
                const RHI::PipelineStateDescriptor* m_pipelineDescritor = nullptr;
                Device* m_device = nullptr;
                PipelineLibrary* m_pipelineLibrary = nullptr;
            };

            RHI::ResultCode Init(const Descriptor& descriptor);

            PipelineLayout* GetPipelineLayout() const;
            PipelineLibrary* GetPipelineLibrary() const;
            VkPipeline GetNativePipeline() const;

        protected:
            Pipeline() = default;

            //////////////////////////////////////////////////////////////////////////
            //! Implementation API
            virtual RHI::ResultCode InitInternal(const Descriptor& descriptor, const PipelineLayout& pipelineLayout) = 0;
            virtual RHI::PipelineStateType GetType() = 0;
            //////////////////////////////////////////////////////////////////////////

            //////////////////////////////////////////////////////////////////////////
            // RHI::Object
            void SetNameInternal(const AZStd::string_view& name) override;
            //////////////////////////////////////////////////////////////////////////

            //////////////////////////////////////////////////////////////////////////
            // RHI::DeviceObject
            void Shutdown() override;
            //////////////////////////////////////////////////////////////////////////

            VkPipeline& GetNativePipelineRef();

            void FillPipelineShaderStageCreateInfo(
                const ShaderStageFunction& function, 
                RHI::ShaderStage stage, 
                uint32_t subStageIndex, 
                VkPipelineShaderStageCreateInfo& createInfo);


            PipelineLibrary* m_pipelineLibrary;
            RHI::Ptr<PipelineLayout> m_pipelineLayout;
            AZStd::list<RHI::Ptr<ShaderModule>> m_shaderModules;
            VkPipeline m_nativePipeline = VK_NULL_HANDLE;
        };
    }
}
