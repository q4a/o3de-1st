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

#include <AzCore/Time/ITime.h>

namespace AzNetworking
{
    struct NetworkInterfaceMetrics
    {
        //! Returns the total number of milliseconds spent updating this network interface.
        AZ::TimeMs m_updateTimeMs = AZ::TimeMs{ 0 };
        //! Returns the total number of connections bound to this network interface.
        uint64_t m_connectionCount = 0;
        //! Returns the total number of milliseconds spent sending data on this network interface.
        AZ::TimeMs m_sendTimeMs = AZ::TimeMs{ 0 };
        //! Returns the total number of packets sent on this socket.
        uint64_t m_sendPackets = 0;
        //! Returns the total number of bytes sent on this socket after compression.
        uint64_t m_sendBytes = 0;
        //! Returns the total number of bytes sent on this socket before compression.
        uint64_t m_sendBytesUncompressed = 0;
        //! Returns the total number of compressed packets sent on this socket that showed no gain over uncompressed size.
        uint64_t m_sendCompressedPacketsNoGain = 0;
        //! Returns the delta gain of bytes saved (+) or lost (-) due to compression.
        int64_t m_sendBytesCompressedDelta = 0;
        //! Returns the total number of packets that had to be resent on this network interface due to packet loss.
        uint64_t m_resentPackets = 0;
        //! Returns the total number of milliseconds spent processing received data on this network interface.
        AZ::TimeMs m_recvTimeMs = AZ::TimeMs{ 0 };
        //! Returns the total number of packets received on this socket.
        uint64_t m_recvPackets = 0;
        //! Returns the total number of bytes received on this socket after compression.
        uint64_t m_recvBytes = 0;
        //! Returns the total number of bytes received on this socket before compression.
        uint64_t m_recvBytesUncompressed = 0;
        //! Returns the total number of packets that were discarded due to timeslice budgets.
        uint64_t m_discardedPackets = 0;
    };
}
