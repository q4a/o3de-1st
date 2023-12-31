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
#include "RHI/Atom_RHI_DX12_precompiled.h"
#include <RHI/CommandListPool.h>
#include <RHI/CommandList.h>
#include <RHI/Conversions.h>
#include <RHI/DescriptorContext.h>
#include <AzCore/Debug/EventTrace.h>

namespace AZ
{
    namespace DX12
    {
        namespace Internal
        {
            ///////////////////////////////////////////////////////////////////
            // CommandAllocatorPool
            ///////////////////////////////////////////////////////////////////

            void CommandAllocatorFactory::Init(const Descriptor& descriptor)
            {
                m_descriptor = descriptor;
            }

            RHI::Ptr<ID3D12CommandAllocator> CommandAllocatorFactory::CreateObject()
            {
                AZ_TRACE_METHOD();
                Microsoft::WRL::ComPtr<ID3D12CommandAllocator> allocator;
                AssertSuccess(m_descriptor.m_dx12Device->CreateCommandAllocator(
                    ConvertHardwareQueueClass(m_descriptor.m_hardwareQueueClass),
                    IID_GRAPHICS_PPV_ARGS(allocator.GetAddressOf())));

                return allocator.Get();
            }

            void CommandAllocatorFactory::ResetObject(ID3D12CommandAllocator& allocator)
            {
                AZ_TRACE_METHOD();
                allocator.Reset();
            }

            ///////////////////////////////////////////////////////////////////
            // CommandListPool
            ///////////////////////////////////////////////////////////////////

            void CommandListFactory::ResetObject(CommandList& commandList, ID3D12CommandAllocator* allocator)
            {
                commandList.Reset(allocator);
            }

            RHI::Ptr<CommandList> CommandListFactory::CreateObject(ID3D12CommandAllocator* allocator)
            {
                RHI::Ptr<CommandList> commandList = CommandList::Create();
                commandList->Init(*m_descriptor.m_device, m_descriptor.m_hardwareQueueClass, m_descriptor.m_descriptorContext, allocator);
                return commandList;
            }

            bool CommandListFactory::CollectObject([[maybe_unused]] CommandList& commandList)
            {
                return true;
            }

            void CommandListFactory::ShutdownObject(CommandList& commandList, bool isPoolShutdown)
            {
                (void)isPoolShutdown;
                commandList.Shutdown();
            }

            void CommandListFactory::Init(const Descriptor& descriptor)
            {
                m_descriptor = descriptor;
            }

            ///////////////////////////////////////////////////////////////////
            // CommandListSubAllocator
            ///////////////////////////////////////////////////////////////////

            void CommandListSubAllocator::Init(
                CommandAllocatorPool& commandAllocatorPool,
                CommandListPool& commandListPool)
            {
                m_commandAllocatorPool = &commandAllocatorPool;
                m_commandListPool = &commandListPool;
            }

            CommandList* CommandListSubAllocator::Allocate()
            {
                if (!m_currentAllocator)
                {
                    m_currentAllocator = m_commandAllocatorPool->Allocate();
                }

                CommandList* commandList = m_commandListPool->Allocate(m_currentAllocator);
                m_activeLists.push_back(commandList);
                return commandList;
            }

            void CommandListSubAllocator::Reset()
            {
                if (m_currentAllocator)
                {
                    m_commandListPool->DeAllocate(m_activeLists.data(), (uint32_t)m_activeLists.size());
                    m_activeLists.clear();

                    m_commandAllocatorPool->DeAllocate(m_currentAllocator);
                    m_currentAllocator = nullptr;
                }
            }
        }

        ///////////////////////////////////////////////////////////////////
        // CommandListAllocator
        ///////////////////////////////////////////////////////////////////

        void CommandListAllocator::Init(const Descriptor& descriptor)
        {
            AZ_Assert(m_isInitialized == false, "CommandListAllocator already initialized!");

            for (uint32_t queueIdx = 0; queueIdx < RHI::HardwareQueueClassCount; ++queueIdx)
            {
                Internal::CommandListPool& commandListPool = m_commandListPools[queueIdx];
                Internal::CommandAllocatorPool& commandAllocatorPool = m_commandAllocatorPools[queueIdx];

                Internal::CommandListPool::Descriptor commandListPoolDescriptor;
                commandListPoolDescriptor.m_device = descriptor.m_device;
                commandListPoolDescriptor.m_hardwareQueueClass = static_cast<RHI::HardwareQueueClass>(queueIdx);
                commandListPoolDescriptor.m_descriptorContext = descriptor.m_descriptorContext;
                commandListPoolDescriptor.m_collectLatency = descriptor.m_frameCountMax;
                commandListPool.Init(commandListPoolDescriptor);

                Internal::CommandAllocatorPool::Descriptor commandAllocatorPoolDescriptor;
                commandAllocatorPoolDescriptor.m_hardwareQueueClass = static_cast<RHI::HardwareQueueClass>(queueIdx);
                commandAllocatorPoolDescriptor.m_dx12Device = descriptor.m_descriptorContext->GetDevice();
                commandAllocatorPoolDescriptor.m_collectLatency = descriptor.m_frameCountMax;
                commandAllocatorPool.Init(commandAllocatorPoolDescriptor);

                m_commandListSubAllocators[queueIdx].SetInitFunction([this, &commandListPool, &commandAllocatorPool]
                    (Internal::CommandListSubAllocator& subAllocator)
                {
                    subAllocator.Init(commandAllocatorPool, commandListPool);
                });
            }

            m_isInitialized = true;
        }

        void CommandListAllocator::Shutdown()
        {
            if (m_isInitialized)
            {
                for (uint32_t queueIdx = 0; queueIdx < RHI::HardwareQueueClassCount; ++queueIdx)
                {
                    m_commandListSubAllocators[queueIdx].ForEach([](Internal::CommandListSubAllocator& commandListSubAllocator)
                    {
                        commandListSubAllocator.Reset();
                    });

                    m_commandListSubAllocators[queueIdx].Clear();
                    m_commandListPools[queueIdx].Shutdown();
                    m_commandAllocatorPools[queueIdx].Shutdown();
                }
                m_isInitialized = false;
            }
        }

        CommandList* CommandListAllocator::Allocate(RHI::HardwareQueueClass hardwareQueueClass)
        {
            AZ_Assert(m_isInitialized, "CommandListAllocator is not initialized!");
            return m_commandListSubAllocators[static_cast<uint32_t>(hardwareQueueClass)].GetStorage().Allocate();
        }

        void CommandListAllocator::Collect()
        {
            for (uint32_t queueIdx = 0; queueIdx < RHI::HardwareQueueClassCount; ++queueIdx)
            {
                m_commandListSubAllocators[queueIdx].ForEach([](Internal::CommandListSubAllocator& commandListSubAllocator)
                {
                    commandListSubAllocator.Reset();
                });

                m_commandListPools[queueIdx].Collect();
                m_commandAllocatorPools[queueIdx].Collect();
            }
        }
    }
}
