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

#include <Atom/RHI/CopyItem.h>
#include <Atom/RHI.Reflect/AttachmentEnums.h>
#include <Atom/RHI.Reflect/Scissor.h>
#include <Atom/RHI.Reflect/Viewport.h>

#include <Atom/RPI.Reflect/Pass/CopyPassData.h>

#include <Atom/RPI.Public/Pass/RenderPass.h>

namespace AZ
{
    namespace RPI
    {
        //! A copy pass is a leaf pass (pass with no children) used for copying images and buffers on the GPU.
        // [GFX TODO] ATOM-1188 (antonmic): Current implementation only supports image to image copying. With
        // ATOM-1188 we will also enable image to buffer, buffer to image and buffer to buffer copies.
        class CopyPass
            : public RenderPass
        {
            AZ_RPI_PASS(CopyPass);

        public:
            AZ_RTTI(CopyPass, "{7387500D-B1BA-4916-B38C-24F5C8DAF839}", RenderPass);
            AZ_CLASS_ALLOCATOR(CopyPass, SystemAllocator, 0);
            virtual ~CopyPass() = default;

            static Ptr<CopyPass> Create(const PassDescriptor& descriptor);

        protected:
            explicit CopyPass(const PassDescriptor& descriptor);

            // Sets up the copy item to perform an image to image copy
            void CopyBuffer(const RHI::FrameGraphCompileContext& context, const PassScopeProducer& producer);
            void CopyImage(const RHI::FrameGraphCompileContext& context, const PassScopeProducer& producer);
            void CopyBufferToImage(const RHI::FrameGraphCompileContext& context, const PassScopeProducer& producer);
            void CopyImageToBuffer(const RHI::FrameGraphCompileContext& context, const PassScopeProducer& producer);

            // Pass behavior overrides
            void BuildAttachmentsInternal() override;

            // Scope producer functions
            void SetupFrameGraphDependencies(RHI::FrameGraphInterface frameGraph, const PassScopeProducer& producer) override;
            void CompileResources(const RHI::FrameGraphCompileContext& context, const PassScopeProducer& producer) override;
            void BuildCommandList(const RHI::FrameGraphExecuteContext& context, const PassScopeProducer& producer) override;

            // Retrieves the copy item type based on the input and output attachment type
            RHI::CopyItemType GetCopyItemType();

            // The copy item submitted to the command list
            RHI::CopyItem m_copyItem;

            // Potential data provided by the PassRequest
            CopyPassData m_data;
        };
    }   // namespace RPI
}   // namespace AZ
