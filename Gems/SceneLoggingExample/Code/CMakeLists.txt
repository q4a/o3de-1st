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

if (NOT PAL_TRAIT_BUILD_HOST_TOOLS)
    return()
endif()

ly_add_target(
    NAME SceneLoggingExample.Static STATIC
    NAMESPACE Gem
    FILES_CMAKE
        sceneloggingexample_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            .
        PUBLIC
            Include
    BUILD_DEPENDENCIES
        PUBLIC
            AZ::AzCore
            AZ::SceneCore
            Legacy::CryCommon
)

ly_add_target(
    NAME SceneLoggingExample ${PAL_TRAIT_MONOLITHIC_DRIVEN_MODULE_TYPE}
    NAMESPACE Gem
    OUTPUT_NAME Gem.SceneLoggingExample.35d8f6e49ae04c9382c61a42d4355c2f.v0.1.0
    FILES_CMAKE
        sceneloggingexample_shared_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            .
        PUBLIC
            Include
    BUILD_DEPENDENCIES
        PRIVATE
            Gem::SceneLoggingExample.Static
)
