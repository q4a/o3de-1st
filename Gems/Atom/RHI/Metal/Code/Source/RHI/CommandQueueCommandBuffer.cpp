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

#include <AzCore/Debug/EventTrace.h>
#include <RHI/CommandQueue.h>

namespace AZ
{
    namespace Metal
    {
        CommandQueueCommandBuffer::~CommandQueueCommandBuffer()
        {
        }
    
        void CommandQueueCommandBuffer::Init(id<MTLCommandQueue> hwQueue)
        {
            m_hwQueue = hwQueue;
        }

        id <MTLCommandBuffer> CommandQueueCommandBuffer::AcquireMTLCommandBuffer()
        {
            AZ_Assert(m_mtlCommandBuffer==nil, "Previous command buffer was not commited");
            //Create a new command buffer
            m_mtlCommandBuffer = [m_hwQueue commandBuffer];
            
            //we call retain here as this CB is active across the autoreleasepools of multiple threads. Calling
            //retain here means that if the current thread's autoreleasepool gets drained this CB will not die.
            //Only when this CB is committed and release is called on it this will it get reclaimed.
            [m_mtlCommandBuffer retain];
            AZ_Assert(m_mtlCommandBuffer != nil, "Could not create the command buffer");
            return m_mtlCommandBuffer;
        }
    
        id <MTLCommandEncoder> CommandQueueCommandBuffer::AcquireSubRenderEncoder(MTLRenderPassDescriptor* renderPassDescriptor, const char * scopeName)
        {
            if(!m_mtlParallelEncoder)
            {
                //Create the parallel encoder which will be used to create all the sub render encoders.
                m_mtlParallelEncoder = [m_mtlCommandBuffer parallelRenderCommandEncoderWithDescriptor:renderPassDescriptor];
                AZ_Assert(m_mtlParallelEncoder != nil, "Could not create the ParallelRenderCommandEncoder");
            }
            
            //Each context will get a sub render encoder.
            id <MTLRenderCommandEncoder> renderCommandEncoder = [m_mtlParallelEncoder renderCommandEncoder];
            renderCommandEncoder.label = [NSString stringWithCString:scopeName encoding:NSUTF8StringEncoding];
            AZ_Assert(renderCommandEncoder != nil, "Could not create the RenderCommandEncoder");
            return renderCommandEncoder;
        }

        void CommandQueueCommandBuffer::FlushParallelEncoder()
        {
            if (m_mtlParallelEncoder)
            {
                [m_mtlParallelEncoder endEncoding];
                m_mtlParallelEncoder = nil;
            }
        }
         
        void CommandQueueCommandBuffer::CommitMetalCommandBuffer()
        {
            [m_mtlCommandBuffer commit];
            
            MTLCommandBufferStatus stat = m_mtlCommandBuffer.status;
            if (stat == MTLCommandBufferStatusError)
            {
                int eCode = m_mtlCommandBuffer.error.code;
                switch (eCode)
                {
                    case MTLCommandBufferErrorNone:
                        break;
                    case MTLCommandBufferErrorInternal:
                        AZ_Assert(false, "Internal error has occurred");
                        break;
                    case MTLCommandBufferErrorTimeout:
                        AZ_Assert(false,"Execution of this command buffer took more time than system allows. execution interrupted and aborted.");
                        break;
                    case MTLCommandBufferErrorPageFault:
                        AZ_Assert(false,"Execution of this command generated an unserviceable GPU page fault. This error maybe caused by buffer read/write attribute mismatch or outof boundary access");
                        break;
                    case MTLCommandBufferErrorBlacklisted:
                        AZ_Assert(false,"Access to this device has been revoked because this client has been responsible for too many timeouts or hangs");
                        break;
                    case MTLCommandBufferErrorNotPermitted:
                        AZ_Assert(false,"This process does not have aceess to use device");
                        break;
                    case MTLCommandBufferErrorOutOfMemory:
                        AZ_Assert(false,"Insufficient memory");
                        break;
                    case MTLCommandBufferErrorInvalidResource:
                        AZ_Assert(false,"The command buffer referenced an invlid resource. This error is most commonly caused when caller deletes a resource before executing a command buffer that refers to it");
                        break;
                    default:
                        break;
                }
            }
            
            //Release to match the retain at creation.
            [m_mtlCommandBuffer release];
            m_mtlCommandBuffer = nil;
        }
    
        const id<MTLCommandBuffer> CommandQueueCommandBuffer::GetMtlCommandBuffer() const
        {
            return m_mtlCommandBuffer;
        }
    }
}
