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

#include <Atom/RHI/ImageFrameAttachment.h>
#include <AzCore/Memory/PoolAllocator.h>

namespace AZ
{
    namespace RHI
    {
        class SwapChain;

        /**
         * A swap chain registered into the frame scheduler.
         */
        class SwapChainFrameAttachment final
            : public ImageFrameAttachment
        {
        public:
            AZ_RTTI(SwapChainFrameAttachment, "{6DBAE3A9-45F9-4B0A-AFF4-0965C456D4C0}", ImageFrameAttachment);
            AZ_CLASS_ALLOCATOR(SwapChainFrameAttachment, AZ::PoolAllocator, 0);

            SwapChainFrameAttachment(
                const AttachmentId& attachmentId,
                Ptr<SwapChain> swapChain);

            /// Returns the swap chain referenced by this attachment.
            const SwapChain* GetSwapChain() const;
            SwapChain* GetSwapChain();

        private:
            Ptr<SwapChain> m_swapChain;
        };
    }
}