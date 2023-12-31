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
#include <Tests/Buffer.h>

namespace UnitTest
{
    using namespace AZ;

    RHI::ResultCode BufferView::InitInternal(RHI::Device&, const RHI::Resource&)
    {
        return RHI::ResultCode::Success;
    }

    RHI::ResultCode BufferView::InvalidateInternal()
    {
        return RHI::ResultCode::Success;
    }

    void BufferView::ShutdownInternal() {}

    bool Buffer::IsMapped() const
    {
        return m_isMapped;
    }

    void* Buffer::Map()
    {
        m_isMapped = true;
        return m_data.data();
    }

    void Buffer::Unmap()
    {
        m_isMapped = false;
    }

    const AZStd::vector<uint8_t>& Buffer::GetData() const
    {
        return m_data;
    }

    RHI::ResultCode BufferPool::InitInternal(RHI::Device&, const RHI::BufferPoolDescriptor&)
    {
        return RHI::ResultCode::Success;
    }

    void BufferPool::ShutdownInternal() {}

    RHI::ResultCode BufferPool::InitBufferInternal(RHI::Buffer& bufferBase, const RHI::BufferDescriptor& descriptor)
    {
        AZ_Assert(IsInitialized(), "Buffer Pool is not initialized");

        Buffer& buffer = static_cast<Buffer&>(bufferBase);
        buffer.m_data.resize(descriptor.m_byteCount);

        return RHI::ResultCode::Success;
    }

    void BufferPool::ShutdownResourceInternal(RHI::Resource& resourceBase)
    {
        Buffer& buffer = static_cast<Buffer&>(resourceBase);
        buffer.m_data.clear();
    }

    RHI::ResultCode BufferPool::MapBufferInternal(const RHI::BufferMapRequest& request, RHI::BufferMapResponse& response)
    {
        Buffer& buffer = static_cast<Buffer&>(*request.m_buffer);
        response.m_data = static_cast<uint8_t*>(buffer.Map()) + request.m_byteOffset;

        return RHI::ResultCode::Success;
    }

    void BufferPool::UnmapBufferInternal(RHI::Buffer& bufferBase)
    {
        Buffer& buffer = static_cast<Buffer&>(bufferBase);
        buffer.Unmap();
    }

    RHI::ResultCode BufferPool::OrphanBufferInternal(RHI::Buffer&)
    {
        return RHI::ResultCode::Success;
    }

    AZ::RHI::ResultCode BufferPool::StreamBufferInternal([[maybe_unused]] const AZ::RHI::BufferStreamRequest& request)
    {
        return RHI::ResultCode::Success;
    }
}
