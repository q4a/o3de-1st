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

#include <AzCore/std/containers/list.h>
#include <AzCore/std/containers/unordered_set.h>
#include <AzCore/std/parallel/mutex.h>
#include <RHI/Buffer.h>
#include <RHI/BufferPool.h>
#include <RHI/CommandList.h>
#include <RHI/Device.h>
#include <RHI/ResourcePoolResolver.h>

namespace AZ
{
    namespace Metal
    {
        class MemoryView;

        class BufferPoolResolver final
            : public ResourcePoolResolver
        {
            using Base = RHI::ResourcePoolResolver;

        public:
            AZ_RTTI(BufferPoolResolver, "{ECC51B75-62AD-4C86-8CAB-D6B492BD2340}", Base);
            AZ_CLASS_ALLOCATOR(BufferPoolResolver, AZ::SystemAllocator, 0);

            BufferPoolResolver(Device& device, const RHI::BufferPoolDescriptor& descriptor);

            ///Get a pointer to write a content to upload to GPU.
            void* MapBuffer(const RHI::BufferMapRequest& request);

            //////////////////////////////////////////////////////////////////////
            ///ResourcePoolResolver
            void Compile() override;
            void Resolve(CommandList& commandList) const override;
            void Deactivate() override;
            void OnResourceShutdown(const RHI::Resource& resource) override;
            //////////////////////////////////////////////////////////////////////

        private:
            struct BufferUploadPacket
            {
                Buffer* m_attachmentBuffer = nullptr;
                RHI::Ptr<Buffer> m_stagingBuffer;
                size_t m_byteOffset = 0;
                size_t m_byteSize = 0;
            };
            
            AZStd::mutex m_uploadPacketsLock;
            AZStd::vector<BufferUploadPacket> m_uploadPackets;
        };
    }
}
