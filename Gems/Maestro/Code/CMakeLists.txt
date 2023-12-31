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
    NAME Maestro.Static STATIC
    NAMESPACE Gem
    OUTPUT_NAME Gem.Maestro.Static
    FILES_CMAKE
        maestro_static_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            Source
        PUBLIC
            Include
    BUILD_DEPENDENCIES
        PRIVATE
            Legacy::CryCommon
            Gem::LmbrCentral
)

ly_add_target(
    NAME Maestro ${PAL_TRAIT_MONOLITHIC_DRIVEN_MODULE_TYPE}
    NAMESPACE Gem
    OUTPUT_NAME Gem.Maestro.3b9a978ed6f742a1acb99f74379a342c.v0.1.0
    FILES_CMAKE
        maestro_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            Source
        PUBLIC
            Include
    BUILD_DEPENDENCIES
        PRIVATE
            Legacy::CryCommon
            Gem::Maestro.Static
            Gem::LmbrCentral
)

if (PAL_TRAIT_BUILD_HOST_TOOLS)
    ly_add_target(
        NAME Maestro.Editor MODULE
        NAMESPACE Gem
        OUTPUT_NAME Gem.Maestro.Editor.3b9a978ed6f742a1acb99f74379a342c.v0.1.0
        FILES_CMAKE
            maestro_files.cmake
            maestro_editor_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                Source
            PUBLIC
                Include
        COMPILE_DEFINITIONS
            PRIVATE
                MAESTRO_EDITOR
        BUILD_DEPENDENCIES
            PRIVATE
                3rdParty::Qt::Widgets
                Legacy::CryCommon
                Legacy::Editor.Headers
                AZ::AzFramework
                AZ::AzToolsFramework
                AZ::AssetBuilderSDK
                Gem::Maestro.Static
                Gem::LmbrCentral
    )
endif()

################################################################################
# Tests
################################################################################
if(PAL_TRAIT_BUILD_TESTS_SUPPORTED)

    ly_add_target(
        NAME Maestro.Tests ${PAL_TRAIT_TEST_TARGET_TYPE}
        NAMESPACE Gem
        FILES_CMAKE
            maestro_tests_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                Tests
                Source
        BUILD_DEPENDENCIES
            PRIVATE
                AZ::AzTest
                Legacy::CryCommon
                Gem::Maestro.Static
    )
    ly_add_googletest(
        NAME Gem::Maestro.Tests
    )
endif()