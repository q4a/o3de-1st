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

#include <AzCore/std/containers/array.h>
#include <AzCore/std/containers/queue.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/parallel/thread.h>
#include <AzCore/std/parallel/conditional_variable.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/std/functional.h>

#include <Atom/RHI/Scope.h>

namespace AZ
{
    namespace RHI
    {
        struct ExecuteWorkRequest
        {
            // A set of swap chains to present after executing the command lists.
            AZStd::vector<SwapChain*> m_swapChainsToPresent;
        };
        
        struct CommandQueueDescriptor
        {
            RHI::HardwareQueueClass m_hardwareQueueClass = RHI::HardwareQueueClass::Graphics;
            int m_maxFrameQueueDepth = RHI::Limits::Device::FrameCountMax;
        };

        //! Base class that provides the backend API the ability to add
        //! commands to a queue which are executed on a separate thread
        class CommandQueue
            : public RHI::DeviceObject
        {
            using Base = RHI::DeviceObject;
        public:            
            AZ_CLASS_ALLOCATOR(CommandQueue, AZ::SystemAllocator, 0);

            CommandQueue() = default;
            virtual ~CommandQueue() = default;
            ResultCode Init(Device& device, const CommandQueueDescriptor& descriptor);
            void Shutdown();
            
            using Command = AZStd::function<void(void* commandQueue)>;
            void QueueCommand(Command command);
            void FlushCommands();
            
            RHI::HardwareQueueClass GetHardwareQueueClass() const;
            const CommandQueueDescriptor& GetDescriptor() const;

        protected:
            //////////////////////////////////////////////////////////////////////////
            // Functions that must be implemented by each RHI.
            virtual ResultCode InitInternal(Device& device, const CommandQueueDescriptor& descriptor) = 0;
            virtual void ExecuteWork(const ExecuteWorkRequest& request) = 0;
            virtual void WaitForIdle() = 0;
            virtual void ShutdownInternal() = 0;
            virtual void* GetNativeQueue() = 0;
            //////////////////////////////////////////////////////////////////////////

        private:
            void ProcessQueue();
            CommandQueueDescriptor m_descriptor;

            AZStd::thread m_thread;
            AZStd::mutex m_workQueueMutex;
            AZStd::queue<Command> m_workQueue;
            AZStd::condition_variable m_workQueueCondition;
            AZStd::atomic_bool m_isWorkQueueEmpty;
            AZStd::atomic_bool m_isQuitting;
        };
    }
}
