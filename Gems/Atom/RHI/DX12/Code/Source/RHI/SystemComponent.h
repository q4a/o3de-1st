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

#include <RHI/Device.h>
#include <RHI/CommandQueueContext.h>
#include <Atom/RHI/Factory.h>
#include <Atom/RHI.Reflect/DX12/Base.h>
#include <AzCore/Component/Component.h>

namespace AZ
{
    namespace DX12
    {
        class SystemComponent
            : public Component
            , public RHI::Factory
        {
        public:
            AZ_COMPONENT(SystemComponent, "{17665B3D-940C-44F5-935C-1FB27EF0FFD7}");

            static void GetProvidedServices(ComponentDescriptor::DependencyArrayType& provided);
            static void GetRequiredServices(ComponentDescriptor::DependencyArrayType& required);
            static void Reflect(ReflectContext* context);

            SystemComponent() = default;
            virtual ~SystemComponent() = default;

            //////////////////////////////////////////////////////////////////////////
            // Component
            void Activate() override;
            void Deactivate() override;
            //////////////////////////////////////////////////////////////////////////

            ///////////////////////////////////////////////////////////////////
            // RHI::Factory
            Name GetName() override;
            RHI::APIType GetType() override;
            RHI::APIPriority GetDefaultPriority() override;
            uint32_t GetAPIUniqueIndex() const override { return APIUniqueIndex; }
            RHI::PhysicalDeviceList EnumeratePhysicalDevices() override;
            RHI::Ptr<RHI::Buffer> CreateBuffer() override;
            RHI::Ptr<RHI::BufferPool> CreateBufferPool() override;
            RHI::Ptr<RHI::BufferView> CreateBufferView() override;
            RHI::Ptr<RHI::Device> CreateDevice() override;
            RHI::Ptr<RHI::Fence> CreateFence() override;
            RHI::Ptr<RHI::FrameGraphCompiler> CreateFrameGraphCompiler() override;
            RHI::Ptr<RHI::FrameGraphExecuter> CreateFrameGraphExecuter() override;
            RHI::Ptr<RHI::Image> CreateImage() override;
            RHI::Ptr<RHI::ImagePool> CreateImagePool() override;
            RHI::Ptr<RHI::ImageView> CreateImageView() override;
            RHI::Ptr<RHI::StreamingImagePool> CreateStreamingImagePool() override;
            RHI::Ptr<RHI::PipelineLibrary> CreatePipelineLibrary() override;
            RHI::Ptr<RHI::PipelineState> CreatePipelineState() override;
            RHI::Ptr<RHI::Scope> CreateScope() override;
            RHI::Ptr<RHI::ShaderResourceGroup> CreateShaderResourceGroup() override;
            RHI::Ptr<RHI::ShaderResourceGroupPool> CreateShaderResourceGroupPool() override;
            RHI::Ptr<RHI::SwapChain> CreateSwapChain() override;
            RHI::Ptr<RHI::TransientAttachmentPool> CreateTransientAttachmentPool() override;
            RHI::Ptr<RHI::QueryPool> CreateQueryPool() override;
            RHI::Ptr<RHI::Query> CreateQuery() override;
            RHI::Ptr<RHI::IndirectBufferSignature> CreateIndirectBufferSignature() override;
            RHI::Ptr<RHI::IndirectBufferWriter> CreateIndirectBufferWriter() override;
            RHI::Ptr<RHI::RayTracingBlas> CreateRayTracingBlas() override;
            RHI::Ptr<RHI::RayTracingTlas> CreateRayTracingTlas() override;
            RHI::Ptr<RHI::RayTracingPipelineState> CreateRayTracingPipelineState() override;
            RHI::Ptr<RHI::RayTracingShaderTable> CreateRayTracingShaderTable() override;
            ///////////////////////////////////////////////////////////////////

        private:
            // This function is implemented by each supported platform in a separate file.
            bool CheckSystemRequirements() const;

            const Name m_apiName{APINameString};
        };
    }
}
