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

#include <Atom/RHI/BufferPool.h>
#include <RHI/BufferMemoryAllocator.h>

namespace AZ
{
    namespace DX12
    {
        class BufferPoolResolver;

        class BufferPool final
            : public RHI::BufferPool
        {
            using Base = RHI::BufferPool;
        public:
            AZ_RTTI(BufferPool, "{BC251841-AADD-4A4A-A4FF-4F94897541D5}", Base);
            AZ_CLASS_ALLOCATOR(BufferPool, AZ::SystemAllocator, 0);
            virtual ~BufferPool() = default;

            static RHI::Ptr<BufferPool> Create();

        private:
            BufferPool() = default;

            Device& GetDevice() const;

            //////////////////////////////////////////////////////////////////////////
            // FrameSchedulerEventBus::Handler
            void OnFrameEnd() override;
            //////////////////////////////////////////////////////////////////////////

            //////////////////////////////////////////////////////////////////////////
            // RHI::BufferPool
            RHI::ResultCode InitInternal(RHI::Device& device, const RHI::BufferPoolDescriptor& descriptor) override;
            void ShutdownInternal() override;
            RHI::ResultCode InitBufferInternal(RHI::Buffer& buffer, const RHI::BufferDescriptor& rhiDescriptor) override;
            void ShutdownResourceInternal(RHI::Resource& resource) override;
            RHI::ResultCode OrphanBufferInternal(RHI::Buffer& buffer) override;
            RHI::ResultCode MapBufferInternal(const RHI::BufferMapRequest& mapRequest, RHI::BufferMapResponse& response) override;
            void UnmapBufferInternal(RHI::Buffer& buffer) override;
            RHI::ResultCode StreamBufferInternal(const RHI::BufferStreamRequest& request) override;
            //////////////////////////////////////////////////////////////////////////

            BufferPoolResolver* GetResolver();

            BufferMemoryAllocator m_allocator;
        };
    }
}