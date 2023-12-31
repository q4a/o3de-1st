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

# Platform specific cmake file for configuring target compiler/link properties
# based on the active platform
# NOTE: functions in cmake are global, therefore adding functions to this file
# is being avoided to prevent overriding functions declared in other targets platfrom
# specific cmake files

if (LY_MONOLITHIC_GAME) # Only Atom is supported in monolithic builds
    set(LY_BUILD_DEPENDENCIES
        PUBLIC
            Legacy::CryRenderOther
    )
else()
    set(LY_RUNTIME_DEPENDENCIES
        Legacy::CryRenderMetal
    )
endif()
