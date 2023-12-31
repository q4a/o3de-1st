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
#include <Atom/RHI/Query.h>
#include <Atom/RHI/QueryPool.h>

namespace AZ
{
    namespace RHI
    {
        void Query::ReportMemoryUsage(MemoryStatisticsBuilder& builder) const
        {
            AZ_UNUSED(builder);
        }

        QueryHandle Query::GetHandle() const
        {
            return m_handle;
        }

        const QueryPool* Query::GetQueryPool() const
        {
            return static_cast<const QueryPool*>(GetPool());
        }

        QueryPool* Query::GetQueryPool()
        {
            return static_cast<QueryPool*>(GetPool());
        }

        ResultCode Query::Begin(CommandList& commandList, QueryControlFlags flags /*= QueryControlFlags::None*/)
        {
            if (Validation::IsEnabled())
            {
                if (m_currentCommandList)
                {
                    AZ_Error("RHI", false, "Query was never ended");
                    return ResultCode::Fail;
                }

                auto poolType = GetQueryPool()->GetDescriptor().m_type;
                if (poolType != QueryType::Occlusion && RHI::CheckBitsAny(flags, QueryControlFlags::PreciseOcclusion))
                {
                    AZ_Error("RHI", false, "Precise Occlusion is only available for occlusion type queries");
                    return ResultCode::InvalidArgument;
                }

                if (poolType == QueryType::Timestamp)
                {
                    AZ_Error("RHI", false, "Begin is not valid for timestamp queries");
                    return ResultCode::Fail;
                }
            }
            
            m_currentCommandList = &commandList;
            return BeginInternal(commandList, flags);
        }

        ResultCode Query::End(CommandList& commandList)
        {
            if (Validation::IsEnabled())
            {
                if (GetQueryPool()->GetDescriptor().m_type == QueryType::Timestamp)
                {
                    AZ_Error("RHI", false, "End operation is not valid for timestamp queries");
                    return ResultCode::Fail;
                }

                if (!m_currentCommandList)
                {
                    AZ_Error("RHI", false, "Query must begin before it can end");
                    return ResultCode::Fail;
                }

                // Check that the command list used for begin is the same one.
                if (m_currentCommandList != &commandList)
                {
                    AZ_Error("RHI", false, "A different command list was passed when ending the query");
                    return ResultCode::InvalidArgument;
                }
            }

            auto result = EndInternal(commandList);
            m_currentCommandList = nullptr;
            return result;
        }

        ResultCode Query::WriteTimestamp(CommandList& commandList)
        {
            if (Validation::IsEnabled())
            {
                if (GetQueryPool()->GetDescriptor().m_type != QueryType::Timestamp)
                {
                    AZ_Error("RHI", false, "Only timestamp queries support WriteTimestamp");
                    return ResultCode::Fail;
                }
            }

            auto result = WriteTimestampInternal(commandList);
            return result;
        }
    }
}
