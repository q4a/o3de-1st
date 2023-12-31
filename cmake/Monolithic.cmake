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

set(LY_MONOLITHIC_GAME FALSE CACHE BOOL "Indicates if the game will be built monolithically (other targets are not supported)")

if(LY_MONOLITHIC_GAME)
    add_compile_definitions(AZ_MONOLITHIC_BUILD)
    set(PAL_TRAIT_MONOLITHIC_DRIVEN_LIBRARY_TYPE STATIC)
    set(PAL_TRAIT_MONOLITHIC_DRIVEN_MODULE_TYPE STATIC)
    # Disable targets that are not supported with monolithic
    set(PAL_TRAIT_BUILD_HOST_TOOLS FALSE)
    set(PAL_TRAIT_BUILD_HOST_GUI_TOOLS FALSE)
    set(PAL_TRAIT_BUILD_TESTS_SUPPORTED FALSE)
    set(PAL_TRAIT_BUILD_SERVER_SUPPORTED FALSE)
else()
    set(PAL_TRAIT_MONOLITHIC_DRIVEN_LIBRARY_TYPE SHARED)
    set(PAL_TRAIT_MONOLITHIC_DRIVEN_MODULE_TYPE MODULE)
endif()