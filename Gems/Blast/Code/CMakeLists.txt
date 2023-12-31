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

ly_get_list_relative_pal_filename(pal_dir ${CMAKE_CURRENT_LIST_DIR}/Platform/${PAL_PLATFORM_NAME})

include(${pal_dir}/PAL_${PAL_PLATFORM_NAME_LOWERCASE}.cmake) # for PAL_TRAIT_BLAST Traits

if(NOT PAL_TRAIT_BLAST_SUPPORTED)
    include(blast_unsupported.cmake)
    return()
endif()

ly_add_target(
    NAME Blast.Static STATIC
    NAMESPACE Gem
    FILES_CMAKE
        blast_files.cmake
    INCLUDE_DIRECTORIES
        PUBLIC
            Include
        PRIVATE
            .
            Source
    BUILD_DEPENDENCIES
        PUBLIC
            3rdParty::Blast
            3rdParty::PhysX
            AZ::AzCore
            AZ::AzFramework
            Gem::Atom_Feature_Common.Static
            Gem::AtomLyIntegration_CommonFeatures.Static
            Gem::PhysX.Static
)

ly_add_target(
    NAME Blast ${PAL_TRAIT_MONOLITHIC_DRIVEN_MODULE_TYPE}
    NAMESPACE Gem
    OUTPUT_NAME Gem.Blast.414bd211c99d4f74aef3a266b9ca208c.v0.1.0
    FILES_CMAKE
        blast_shared_files.cmake
    INCLUDE_DIRECTORIES
        PUBLIC
            Include
        PRIVATE
            .
            Source
    BUILD_DEPENDENCIES
        PRIVATE
            Gem::Blast.Static
)

if(PAL_TRAIT_BUILD_HOST_TOOLS)

    ly_add_target(
        NAME Blast.Editor.Static STATIC
        NAMESPACE Gem
        AUTOMOC
        AUTOUIC
        FILES_CMAKE
            blast_editor_files.cmake
        COMPILE_DEFINITIONS
            PUBLIC
                BLAST_EDITOR
        INCLUDE_DIRECTORIES
            PRIVATE
                .
                Source
            PUBLIC
                Include
        BUILD_DEPENDENCIES
            PUBLIC
                AZ::AzToolsFramework
                AZ::GFxFramework
                AZ::SceneCore
                AZ::SceneData
                Legacy::Editor.Headers
                Gem::Atom_RPI.Edit
                Gem::Blast.Static
    )

    ly_add_target(
        NAME Blast.Editor MODULE
        NAMESPACE Gem
        AUTOMOC
        OUTPUT_NAME Gem.Blast.Editor.414bd211c99d4f74aef3a266b9ca208c.v0.1.0
        FILES_CMAKE
            blast_editor_shared_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                .
                Source
            PUBLIC
                Include
        BUILD_DEPENDENCIES
            PUBLIC
                Gem::Blast.Editor.Static
                Gem::PythonAssetBuilder.Editor
        RUNTIME_DEPENDENCIES
                3rdParty::assimp
    )

endif()

################################################################################
# Tests
################################################################################
if(PAL_TRAIT_BUILD_TESTS_SUPPORTED)

    ly_add_target(
        NAME Blast.Tests MODULE
        NAMESPACE Gem
        FILES_CMAKE
            blast_tests_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                Tests
                .
                Source
        BUILD_DEPENDENCIES
            PRIVATE
                AZ::AzTestShared
                AZ::AzTest
                AZ::AzPhysicsTests
                Gem::Atom_Feature_Common.Tests
                Gem::Blast.Static
    )
    ly_add_googletest(
        NAME Gem::Blast.Tests
        TEST_SUITE sandbox
    )
    
    if (PAL_TRAIT_BUILD_HOST_TOOLS)
        ly_add_target(
            NAME Blast.Editor.Tests MODULE
            NAMESPACE Gem
            FILES_CMAKE
                blast_editor_tests_files.cmake
            INCLUDE_DIRECTORIES
                PRIVATE
                    Tests
                    Source
                    .
            BUILD_DEPENDENCIES
                PRIVATE
                    3rdParty::Qt::Test
                    AZ::AzTestShared
                    AZ::AzTest
                    AZ::AzPhysicsTests
                    AZ::AzToolsFrameworkTestCommon
                    Gem::Blast.Editor.Static
        )
        ly_add_googletest(
            NAME Gem::Blast.Editor.Tests
        )
    endif()
endif()
