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
#include <RHI/Buffer.h>
#include <RHI/IndirectBufferWriter.h>
#include <RHI/IndirectBufferSignature.h>

namespace AZ
{
    namespace Vulkan
    {
        RHI::Ptr<IndirectBufferWriter> IndirectBufferWriter::Create()
        {
            return aznew IndirectBufferWriter();
        }

        void IndirectBufferWriter::SetVertexViewInternal([[maybe_unused]] RHI::IndirectCommandIndex index, [[maybe_unused]] const RHI::StreamBufferView& view)
        {
            AZ_Assert(false, "Vertex View command is not supported on this platform");
        }

        void IndirectBufferWriter::SetIndexViewInternal([[maybe_unused]] RHI::IndirectCommandIndex index, [[maybe_unused]] const RHI::IndexBufferView& indexBufferView)
        {
            AZ_Assert(false, "Index View command is not supported on this platform");
        }

        void IndirectBufferWriter::DrawInternal(RHI::IndirectCommandIndex index, const RHI::DrawLinear& arguments)
        {
            VkDrawIndirectCommand* command = reinterpret_cast<VkDrawIndirectCommand*>(GetCommandMemoryMap(index));
            command->vertexCount = arguments.m_vertexCount;
            command->instanceCount = arguments.m_instanceCount;
            command->firstVertex = arguments.m_vertexOffset;
            command->firstInstance = arguments.m_instanceOffset;
        }

        void IndirectBufferWriter::DrawIndexedInternal(RHI::IndirectCommandIndex index, const RHI::DrawIndexed& arguments)
        {
            VkDrawIndexedIndirectCommand* command = reinterpret_cast<VkDrawIndexedIndirectCommand*>(GetCommandMemoryMap(index));
            command->indexCount = arguments.m_indexCount;
            command->instanceCount = arguments.m_instanceCount;
            command->firstIndex = arguments.m_indexOffset;
            command->vertexOffset = arguments.m_vertexOffset;
            command->firstInstance = arguments.m_instanceOffset;
        }

        void IndirectBufferWriter::DispatchInternal(RHI::IndirectCommandIndex index, const RHI::DispatchDirect& arguments)
        {
            VkDispatchIndirectCommand* command = reinterpret_cast<VkDispatchIndirectCommand*>(GetCommandMemoryMap(index));
            command->x = arguments.GetNumberOfGroupsX();
            command->y = arguments.GetNumberOfGroupsY();
            command->z = arguments.GetNumberOfGroupsZ();
        }

        void IndirectBufferWriter::SetRootConstantsInternal(
            [[maybe_unused]] RHI::IndirectCommandIndex index,
            [[maybe_unused]] const uint8_t* data,
            [[maybe_unused]] uint32_t byteSize)
        {
            AZ_Assert(false, "Inline Constants command is not supported on this platform");
        }

        uint8_t* IndirectBufferWriter::GetCommandMemoryMap(RHI::IndirectCommandIndex index) const
        {
            return GetTargetMemory() + GetCurrentSequenceIndex() * m_sequenceStride + m_signature->GetOffset(index);
        }
    }
}
