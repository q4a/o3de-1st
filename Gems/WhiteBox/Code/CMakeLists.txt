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

ly_get_list_relative_pal_filename(pal_source_dir ${CMAKE_CURRENT_LIST_DIR}/Source/Platform/${PAL_PLATFORM_NAME})
ly_get_list_relative_pal_filename(common_dir ${CMAKE_CURRENT_LIST_DIR}/Platform/Common)

include(${pal_source_dir}/PAL_${PAL_PLATFORM_NAME_LOWERCASE}.cmake)

if(NOT PAL_TRAIT_WHITEBOX_SUPPORTED)
    ly_add_target(
        NAME WhiteBox ${PAL_TRAIT_MONOLITHIC_DRIVEN_MODULE_TYPE}
        NAMESPACE Gem
        OUTPUT_NAME Gem.WhiteBox.c5833dbda2e045d3a5f16b7414280c27.v0.1.0
        FILES_CMAKE
            whitebox_unsupported_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                Source
            PUBLIC
                Include
        BUILD_DEPENDENCIES
            PRIVATE
                AZ::AzCore
    )
    if(PAL_TRAIT_BUILD_HOST_TOOLS)
        ly_add_target(
            NAME WhiteBox.Editor MODULE
            NAMESPACE Gem
            OUTPUT_NAME Gem.WhiteBox.Editor.c5833dbda2e045d3a5f16b7414280c27.v0.1.0
            FILES_CMAKE
                whitebox_unsupported_files.cmake
            INCLUDE_DIRECTORIES
                PRIVATE
                    Source
                PUBLIC
                    Include
            COMPILE_DEFINITIONS
                PRIVATE
                    WHITE_BOX_EDITOR
            BUILD_DEPENDENCIES
                PRIVATE
                    AZ::AzCore
        )
    endif()
    return()
endif()

ly_add_target(
    NAME WhiteBox.Static STATIC
    NAMESPACE Gem
    FILES_CMAKE
        whitebox_supported_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            Source
        PUBLIC
            Include
    BUILD_DEPENDENCIES
        PUBLIC
            AZ::AzCore
            Gem::Atom_RPI.Public
            Gem::Atom_Feature_Common.Static
            Gem::AtomLyIntegration_CommonFeatures.Static
            Legacy::CryCommon
)

ly_add_target(
    NAME WhiteBox ${PAL_TRAIT_MONOLITHIC_DRIVEN_MODULE_TYPE}
    NAMESPACE Gem
    OUTPUT_NAME Gem.WhiteBox.c5833dbda2e045d3a5f16b7414280c27.v0.1.0
    FILES_CMAKE
        whitebox_shared_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            Source
        PUBLIC
            Include
    BUILD_DEPENDENCIES
        PRIVATE
            Gem::WhiteBox.Static
)

if(PAL_TRAIT_BUILD_HOST_TOOLS)
    ly_add_target(
        NAME WhiteBox.Editor.Static STATIC
        NAMESPACE Gem
        FILES_CMAKE
            whitebox_editor_supported_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                Source
            PUBLIC
                Include
        COMPILE_DEFINITIONS
            PRIVATE
                _USE_MATH_DEFINES
                OM_STATIC_BUILD
            PUBLIC
                WHITE_BOX_EDITOR
        BUILD_DEPENDENCIES
            PUBLIC
                3rdParty::OpenMesh
                AZ::AzToolsFramework
                Legacy::CryCommon
                Legacy::EditorCommon
                Legacy::Editor.Headers
                Gem::WhiteBox.Static
    )

    ly_add_target(
        NAME WhiteBox.Editor MODULE
        NAMESPACE Gem
        OUTPUT_NAME Gem.WhiteBox.Editor.c5833dbda2e045d3a5f16b7414280c27.v0.1.0
        FILES_CMAKE
            whitebox_editor_shared_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                Source
            PUBLIC
                Include
        BUILD_DEPENDENCIES
            PRIVATE
                Gem::WhiteBox.Editor.Static
    )
endif()

################################################################################
# Tests
################################################################################
if(PAL_TRAIT_BUILD_TESTS_SUPPORTED)
    ly_add_target(
        NAME WhiteBox.Tests ${PAL_TRAIT_TEST_TARGET_TYPE}
        NAMESPACE Gem
        FILES_CMAKE
            whitebox_tests_supported_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                Source
                Tests
        BUILD_DEPENDENCIES
            PRIVATE
                AZ::AzTestShared
                AZ::AzTest
                Gem::WhiteBox.Static
    )

    ly_add_googletest(
        NAME Gem::WhiteBox.Tests
    )

    if (PAL_TRAIT_BUILD_HOST_TOOLS)
        ly_add_target(
            NAME WhiteBox.Editor.Tests ${PAL_TRAIT_TEST_TARGET_TYPE}
            NAMESPACE Gem
            FILES_CMAKE
                whitebox_editor_tests_supported_files.cmake
            INCLUDE_DIRECTORIES
                PRIVATE
                    Source
                    Tests
            BUILD_DEPENDENCIES
                PRIVATE
                    AZ::AzTestShared
                    AZ::AzManipulatorTestFramework.Static
                    Gem::WhiteBox.Editor.Static
        )

        ly_add_googletest(
            NAME Gem::WhiteBox.Editor.Tests
        )

        ly_add_target(
            NAME WhiteBox.Editor.Physics.Tests ${PAL_TRAIT_TEST_TARGET_TYPE}
            NAMESPACE Gem
            FILES_CMAKE
                whitebox_editor_physics_tests_supported_files.cmake
            INCLUDE_DIRECTORIES
                PRIVATE
                    Source
                    Tests
            BUILD_DEPENDENCIES
                PRIVATE
                    AZ::AzTestShared
                    AZ::AzManipulatorTestFramework.Static
                    Gem::WhiteBox.Editor.Static
        )

        ly_add_googletest(
            NAME Gem::WhiteBox.Editor.Physics.Tests
        )

    endif()
endif()
