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
#include <Atom/RHI/FrameGraphExecuter.h>
#include <Atom/RHI/Buffer.h>
#include <Atom/RHI/FrameGraph.h>
#include <Atom/RHI/FrameGraphExecuteGroup.h>
#include <Atom/RHI/Image.h>
#include <AzCore/Debug/EventTrace.h>

namespace AZ
{
    namespace RHI
    {
        void FrameGraphExecuter::SetJobPolicy(JobPolicy jobPolicy)
        {
            m_jobPolicy = jobPolicy;
        }

        AZStd::array_view<AZStd::unique_ptr<FrameGraphExecuteGroup>> FrameGraphExecuter::GetGroups() const
        {
            return m_groups;
        }

        JobPolicy FrameGraphExecuter::GetJobPolicy() const
        {
            return m_jobPolicy;
        }

        uint32_t FrameGraphExecuter::GetGroupCount() const
        {
            return static_cast<uint32_t>(m_groups.size());
        }

        ResultCode FrameGraphExecuter::Init(const FrameGraphExecuterDescriptor& descriptor)
        {
            if (Validation::IsEnabled())
            {
                if (IsInitialized())
                {
                    AZ_Error("FrameGraphExecuter", false, "FrameGraphExecuter is already initialized!");
                    return RHI::ResultCode::InvalidOperation;
                }
            }

            m_descriptor = descriptor;
            const RHI::ResultCode resultCode = InitInternal(descriptor);

            if (resultCode == RHI::ResultCode::Success)
            {
                DeviceObject::Init(*descriptor.m_device);
            }

            return resultCode;
        }

        void FrameGraphExecuter::Shutdown()
        {
            if (IsInitialized())
            {
                AZ_Assert(m_pendingGroups.empty(), "Pending contexts in queue.");
                ShutdownInternal();
                DeviceObject::Shutdown();
            }
        }

        void FrameGraphExecuter::Begin(const FrameGraph& frameGraph)
        {
            AZ_TRACE_METHOD();
            BeginInternal(frameGraph);
        }

        void FrameGraphExecuter::End()
        {
            AZ_TRACE_METHOD();
            AZ_Assert(m_pendingGroups.empty(), "Pending contexts in queue.");
            m_groups.clear();
            EndInternal();
        }

        FrameGraphExecuteGroup* FrameGraphExecuter::BeginGroup(uint32_t groupIndex)
        {
            FrameGraphExecuteGroup& group = *m_groups[groupIndex];
            AZ_Assert(group.IsComplete() == false, "Context group cannot be reused.");
            group.BeginInternal();
            return &group;
        }

        void FrameGraphExecuter::EndGroup(uint32_t groupIndex)
        {
            FrameGraphExecuteGroup& group = *m_groups[groupIndex];
            AZ_Assert(group.IsComplete(), "Ending a context group before all child contexts have ended!");
            group.EndInternal();
            group.m_isSubmittable = true;

            AZStd::lock_guard<AZStd::mutex> lock(m_pendingContextGroupLock);
            while (m_pendingGroups.size() && m_pendingGroups.front()->IsSubmittable())
            {
                ExecuteGroupInternal(*m_pendingGroups.front());
                m_pendingGroups.pop();
            }
        }
    }
}
