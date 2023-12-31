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
    NAME FastNoise.Static STATIC
    NAMESPACE Gem
    FILES_CMAKE
        fastnoise_files.cmake
    INCLUDE_DIRECTORIES
        PUBLIC
            Source
            .       # To give acess to External/FastNoise
            Include
    BUILD_DEPENDENCIES
        PUBLIC
            Legacy::CryCommon
            Gem::GradientSignal
            Gem::LmbrCentral
)

ly_add_target(
    NAME FastNoise ${PAL_TRAIT_MONOLITHIC_DRIVEN_MODULE_TYPE}
    NAMESPACE Gem
    OUTPUT_NAME Gem.FastNoise.c5f23032407f49ca8d8de1733423565c.v0.1.0
    FILES_CMAKE
        fastnoise_shared_files.cmake
    INCLUDE_DIRECTORIES
        PUBLIC
            Include
    BUILD_DEPENDENCIES
        PRIVATE
            Gem::FastNoise.Static
)

if(PAL_TRAIT_BUILD_HOST_TOOLS)
    ly_add_target(
        NAME FastNoise.Editor.Static STATIC
        NAMESPACE Gem
        FILES_CMAKE
            fastnoise_editor_files.cmake
        COMPILE_DEFINITIONS
            PUBLIC
                FASTNOISE_EDITOR
        INCLUDE_DIRECTORIES
            PRIVATE
                Source
            PUBLIC
                Include
        BUILD_DEPENDENCIES
            PUBLIC
                Gem::FastNoise.Static
                AZ::AzToolsFramework
                Gem::SurfaceData
    )

    ly_add_target(
        NAME FastNoise.Editor MODULE
        NAMESPACE Gem
        OUTPUT_NAME Gem.FastNoise.Editor.c5f23032407f49ca8d8de1733423565c.v0.1.0
        FILES_CMAKE
            fastnoise_editor_shared_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                Source
            PUBLIC
                Include
        BUILD_DEPENDENCIES
            PRIVATE
                FastNoise.Editor.Static
    )
endif()

################################################################################
# Tests
################################################################################
if(PAL_TRAIT_BUILD_TESTS_SUPPORTED)
    if(PAL_TRAIT_BUILD_HOST_TOOLS)
        ly_add_target(
            NAME FastNoise.Editor.Tests ${PAL_TRAIT_TEST_TARGET_TYPE}
            NAMESPACE Gem
            FILES_CMAKE
                fastnoise_tests_files.cmake
            COMPILE_DEFINITIONS
                PUBLIC
                    FASTNOISE_EDITOR
            INCLUDE_DIRECTORIES
                PRIVATE
                    Tests
            BUILD_DEPENDENCIES
                PRIVATE
                    AZ::AzTest
                    FastNoise.Editor.Static
        )
        ly_add_googletest(
            NAME Gem::FastNoise.Editor.Tests
        )
    else()
        ly_add_target(
            NAME FastNoise.Tests ${PAL_TRAIT_TEST_TARGET_TYPE}
            NAMESPACE Gem
            FILES_CMAKE
                fastnoise_tests_files.cmake
            INCLUDE_DIRECTORIES
                PRIVATE
                    Tests
            BUILD_DEPENDENCIES
                PRIVATE
                    AZ::AzTest
                    Gem::FastNoise.Static
        )
        ly_add_googletest(
            NAME Gem::FastNoise.Tests
        )
    endif()
endif()
