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

ly_add_target(
    NAME StartingPointCamera.Static STATIC
    NAMESPACE Gem
    FILES_CMAKE
        startingpointcamera_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            Source
        PUBLIC
            Include
    BUILD_DEPENDENCIES
        PRIVATE
            AZ::AzCore
            Gem::CameraFramework.Static
            Gem::LmbrCentral
            Legacy::CryCommon
)

ly_add_target(
    NAME StartingPointCamera ${PAL_TRAIT_MONOLITHIC_DRIVEN_MODULE_TYPE}
    NAMESPACE Gem
    OUTPUT_NAME Gem.StartingPointCamera.834070b9537d44df83559e2045c3859f.v0.1.0
    FILES_CMAKE
        startingpointcamera_shared_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            Source
        PUBLIC
            Include
    BUILD_DEPENDENCIES
        PRIVATE
            AZ::AzCore
            AZ::AzFramework
            Gem::StartingPointCamera.Static
            Gem::CameraFramework.Static
            Gem::LmbrCentral.Static
            Legacy::CryCommon
)
