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

#include <AzCore/base.h>
#include <AzCore/Memory/Memory.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>

namespace AZ::IO
{
    struct DriveInformation
    {
        AZ_TYPE_INFO(AZ::IO::DriveInformation, "{89BEC62B-4B61-4878-8E40-E4C7A0DC717E}");

        AZStd::vector<AZStd::string> m_paths;
        AZStd::string m_profile;
        size_t m_physicalSectorSize{ AZCORE_GLOBAL_NEW_ALIGNMENT };
        size_t m_logicalSectorSize{ AZCORE_GLOBAL_NEW_ALIGNMENT };
        size_t m_pageSize{ 0 };
        size_t m_maxTransfer{ 0 };
        u32 m_ioChannelCount{ 0 };
        bool m_supportsQueuing{ false };
        bool m_hasSeekPenalty{ true };
    };

    using DriveList = AZStd::vector<DriveInformation>;
} // namespace AZ::IO
