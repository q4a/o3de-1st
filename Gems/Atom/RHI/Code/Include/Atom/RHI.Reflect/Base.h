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

#include <Atom/RHI.Reflect/Bits.h>
#include <AzCore/Math/Crc.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/Outcome/Outcome.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/smart_ptr/intrusive_ptr.h>

namespace UnitTest
{
    class RHITestFixture;
    class RPITestFixture;
}

namespace AZ
{
    namespace RHI
    {
        class BuildOptions
        {
        public:
#ifdef AZ_DEBUG_BUILD
            static constexpr bool IsDebugBuild = true;
#else
            static constexpr bool IsDebugBuild = false;
#endif

#ifdef AZ_PROFILE_BUILD
            static constexpr bool IsProfileBuild = true;
#else
            static constexpr bool IsProfileBuild = false;
#endif
        };

        class Validation
        {
            friend class UnitTest::RHITestFixture;
            friend class UnitTest::RPITestFixture;
        public:
            static bool IsEnabled()
            {
                return s_isEnabled;
            }
        private:
            static bool s_isEnabled;
        };

        template <typename T>
        using Ptr = AZStd::intrusive_ptr<T>;

        template <typename T>
        using ConstPtr = AZStd::intrusive_ptr<const T>;

        /**
         * A set of general result codes used by methods which may fail.
         */
        enum class ResultCode : uint32_t
        {
            // The operation succeeded.
            Success = 0,

            // The operation failed with an unknown error.
            Fail,

            // The operation failed due being out of memory.
            OutOfMemory,

            // The operation failed because the feature is unimplemented on the particular platform.
            Unimplemented,

            // The operation failed because the API object is not in a state to accept the call.
            InvalidOperation,

            // The operation failed due to invalid arguments.
            InvalidArgument,

            // The operation is not ready
            NotReady
        };

        using MessageOutcome = AZ::Outcome<void, AZStd::string>;

        using APIType = Crc32;

        enum class DrawListSortType : uint8_t
        {
            KeyThenDepth = 0,
            KeyThenReverseDepth,
            DepthThenKey,
            ReverseDepthThenKey
        };

    }

    AZ_TYPE_INFO_SPECIALIZE(RHI::DrawListSortType, "{D43AF0B7-7314-4B57-AA98-6209235B91BB}");
}
