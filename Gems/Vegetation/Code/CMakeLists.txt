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
    NAME Vegetation.Static STATIC
    NAMESPACE Gem
    FILES_CMAKE
        vegetation_files.cmake
    COMPILE_DEFINITIONS
        PUBLIC
            # if we are in debug or profile, enable VEG_PROFILE_ENABLED
            $<$<IN_LIST:$<CONFIG>,debug;profile>:VEG_PROFILE_ENABLED>
    INCLUDE_DIRECTORIES
        PRIVATE
            Source
        PUBLIC
            Include
    BUILD_DEPENDENCIES
        PUBLIC
            Legacy::CryCommon
            Gem::LmbrCentral
            Gem::GradientSignal
            Gem::SurfaceData
)

ly_add_target(
    NAME Vegetation ${PAL_TRAIT_MONOLITHIC_DRIVEN_MODULE_TYPE}
    NAMESPACE Gem
    OUTPUT_NAME Gem.Vegetation.f394e7cf54424bba89615381bba9511b.v0.1.0
    FILES_CMAKE
        vegetation_shared_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            Source
        PUBLIC
            Include
    BUILD_DEPENDENCIES
        PRIVATE
            Gem::Vegetation.Static
)

if(PAL_TRAIT_BUILD_HOST_TOOLS)
    ly_add_target(
        NAME Vegetation.Editor MODULE
        NAMESPACE Gem
        OUTPUT_NAME Gem.Vegetation.Editor.f394e7cf54424bba89615381bba9511b.v0.1.0
        FILES_CMAKE
            vegetation_editor_files.cmake
        COMPILE_DEFINITIONS
            PRIVATE
                VEGETATION_EDITOR
        INCLUDE_DIRECTORIES
            PRIVATE
                Source
            PUBLIC
                Include
        BUILD_DEPENDENCIES
            PRIVATE
                Gem::Vegetation.Static
                AZ::AzToolsFramework
    )
endif()

################################################################################
# Tests
################################################################################
if(PAL_TRAIT_BUILD_TESTS_SUPPORTED)
    ly_add_target(
        NAME Vegetation.Tests ${PAL_TRAIT_TEST_TARGET_TYPE}
        NAMESPACE Gem
        FILES_CMAKE
            vegetation_tests_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                Tests
                Source
                .
        BUILD_DEPENDENCIES
            PRIVATE
                AZ::AzTest
                Gem::Vegetation.Static
    )
    ly_add_googletest(
        NAME Gem::Vegetation.Tests
    )
endif()
