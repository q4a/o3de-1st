#
# All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
# its licensors.
#
# For complete copyright and license terms please see the LICENSE at the root of this
# distribution (the "License"). All use of this software is governed by the License,
# or, if provided, by the license below or the license accompanying this file. Do not
# remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#

set(FILES
    Source/RHI/Buffer.cpp
    Source/RHI/Buffer.h
    Source/RHI/BufferPool.cpp
    Source/RHI/BufferPool.h
    Source/RHI/BufferPoolResolver.cpp
    Source/RHI/BufferPoolResolver.h
    Source/RHI/BufferView.cpp
    Source/RHI/BufferView.h
    Source/RHI/CommandList.cpp
    Source/RHI/CommandList.h
    Source/RHI/CommandPool.cpp
    Source/RHI/CommandPool.h
    Source/RHI/CommandListAllocator.cpp
    Source/RHI/CommandListAllocator.h
    Source/RHI/AsyncUploadQueue.cpp
    Source/RHI/AsyncUploadQueue.h
    Source/RHI/CommandQueue.cpp
    Source/RHI/CommandQueue.h
    Source/RHI/CommandQueueContext.cpp
    Source/RHI/CommandQueueContext.h
    Source/RHI/Conversion.h
    Source/RHI/Conversion.cpp
    Source/RHI/Formats.inl
    Source/RHI/DescriptorPool.cpp
    Source/RHI/DescriptorPool.h
    Source/RHI/DescriptorSet.cpp
    Source/RHI/DescriptorSet.h
    Source/RHI/DescriptorSetLayout.cpp
    Source/RHI/DescriptorSetLayout.h
    Source/RHI/DescriptorSetAllocator.h
    Source/RHI/DescriptorSetAllocator.cpp
    Source/RHI/Device.cpp
    Source/RHI/Device.h
    Source/RHI/Fence.cpp
    Source/RHI/Fence.h
    Source/RHI/Framebuffer.cpp
    Source/RHI/Framebuffer.h
    Source/RHI/FrameGraphCompiler.cpp
    Source/RHI/FrameGraphCompiler.h
    Source/RHI/FrameGraphExecuteGroupHandler.cpp
    Source/RHI/FrameGraphExecuteGroupHandler.h
    Source/RHI/FrameGraphExecuteGroupHandlerBase.cpp
    Source/RHI/FrameGraphExecuteGroupHandlerBase.h
    Source/RHI/FrameGraphExecuteGroupMergedHandler.cpp
    Source/RHI/FrameGraphExecuteGroupMergedHandler.h
    Source/RHI/FrameGraphExecuteGroupBase.cpp
    Source/RHI/FrameGraphExecuteGroupBase.h
    Source/RHI/FrameGraphExecuteGroup.cpp
    Source/RHI/FrameGraphExecuteGroup.h
    Source/RHI/FrameGraphExecuteGroupMerged.cpp
    Source/RHI/FrameGraphExecuteGroupMerged.h
    Source/RHI/FrameGraphExecuter.cpp
    Source/RHI/FrameGraphExecuter.h
    Source/RHI/Image.cpp
    Source/RHI/Image.h
    Source/RHI/ImagePool.cpp
    Source/RHI/ImagePool.h
    Source/RHI/ImagePoolResolver.cpp
    Source/RHI/ImagePoolResolver.h
    Source/RHI/ImageView.cpp
    Source/RHI/ImageView.h
    Source/RHI/StreamingImagePool.cpp
    Source/RHI/StreamingImagePool.h
    Source/RHI/IndirectBufferSignature.cpp
    Source/RHI/IndirectBufferSignature.h
    Source/RHI/IndirectBufferWriter.cpp
    Source/RHI/IndirectBufferWriter.h
    Source/RHI/QueryPool.cpp
    Source/RHI/QueryPool.h
    Source/RHI/Query.cpp
    Source/RHI/Query.h
    Source/RHI/Queue.cpp
    Source/RHI/Queue.h
    Source/RHI/ReleaseQueue.h
    Source/RHI/RenderPass.cpp
    Source/RHI/RenderPass.h
    Source/RHI/RenderPassBuilder.cpp
    Source/RHI/RenderPassBuilder.h
    Source/RHI/ResourcePoolResolver.h
    Source/RHI/MemoryTypeView.h
    Source/RHI/MemoryView.h
    Source/RHI/Memory.cpp
    Source/RHI/Memory.h
    Source/RHI/MemoryTypeAllocator.h
    Source/RHI/MemoryAllocator.h
    Source/RHI/MemoryPageAllocator.cpp
    Source/RHI/MemoryPageAllocator.h
    Source/RHI/Instance.cpp
    Source/RHI/Instance.h
    Source/RHI/PhysicalDevice.cpp
    Source/RHI/PhysicalDevice.h
    Source/RHI/PipelineLayout.cpp
    Source/RHI/PipelineLayout.h
    Source/RHI/ComputePipeline.cpp
    Source/RHI/ComputePipeline.h
    Source/RHI/GraphicsPipeline.cpp
    Source/RHI/GraphicsPipeline.h
    Source/RHI/Pipeline.cpp
    Source/RHI/Pipeline.h
    Source/RHI/PipelineLibrary.cpp
    Source/RHI/PipelineLibrary.h
    Source/RHI/PipelineState.cpp
    Source/RHI/PipelineState.h
    Source/RHI/Sampler.cpp
    Source/RHI/Sampler.h
    Source/RHI/SemaphoreAllocator.cpp
    Source/RHI/SemaphoreAllocator.h
    Source/RHI/Semaphore.cpp
    Source/RHI/Semaphore.h
    Source/RHI/Scope.cpp
    Source/RHI/Scope.h
    Source/RHI/ShaderResourceGroup.cpp
    Source/RHI/ShaderResourceGroup.h
    Source/RHI/ShaderResourceGroupPool.cpp
    Source/RHI/ShaderResourceGroupPool.h
    Source/RHI/ShaderModule.cpp
    Source/RHI/ShaderModule.h
    Source/RHI/WSISurface.cpp
    Source/RHI/WSISurface.h
    Source/RHI/SwapChain.cpp
    Source/RHI/SwapChain.h
    Source/RHI/SystemComponent.cpp
    Source/RHI/SystemComponent.h
    Source/RHI/Vulkan.cpp
    Source/RHI/Vulkan.h
    Source/RHI/AliasedHeap.cpp
    Source/RHI/AliasedHeap.h
    Source/RHI/AliasingBarrierTracker.h
    Source/RHI/AliasingBarrierTracker.cpp
    Source/RHI/TransientAttachmentPool.cpp
    Source/RHI/TransientAttachmentPool.h
    Source/RHI/SignalEvent.cpp
    Source/RHI/SignalEvent.h
    Source/RHI/NullDescriptorManager.cpp
    Source/RHI/NullDescriptorManager.h
    Source/RHI/MergedShaderResourceGroupPool.cpp
    Source/RHI/MergedShaderResourceGroupPool.h
    Source/RHI/MergedShaderResourceGroup.cpp
    Source/RHI/MergedShaderResourceGroup.h
    Source/RHI/ReleaseContainer.h
    Source/RHI/BufferMemory.cpp
    Source/RHI/BufferMemory.h
    Source/RHI/BufferMemoryPageAllocator.cpp
    Source/RHI/BufferMemoryPageAllocator.h
    Source/RHI/BufferMemoryView.h
    Source/RHI/BufferMemoryAllocator.h
)
