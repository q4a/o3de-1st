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

add_subdirectory(NumericalMethods)

ly_get_list_relative_pal_filename(pal_source_dir ${CMAKE_CURRENT_LIST_DIR}/Source/Platform/${PAL_PLATFORM_NAME})

include(${pal_source_dir}/PAL_${PAL_PLATFORM_NAME_LOWERCASE}.cmake) # for PAL_TRAIT_PHYSX_SUPPORTED

if(PAL_TRAIT_PHYSX_SUPPORTED)
    set(physx_files physx_files.cmake)
    set(physx_shared_files physx_shared_files.cmake)
    set(physx_editor_files physx_editor_files.cmake)
else()
    set(physx_files physx_unsupported_files.cmake)
    set(physx_shared_files physx_unsupported_shared_files.cmake)
    set(physx_editor_files physx_unsupported_files.cmake)
endif()

ly_add_target(
    NAME PhysX.Static STATIC
    NAMESPACE Gem
    FILES_CMAKE
        ${physx_files}
    COMPILE_DEFINITIONS
        PUBLIC
            PHYSX_ENABLE_MULTI_THREADING
    INCLUDE_DIRECTORIES
        PUBLIC
            .
            Source
            ${pal_source_dir}
            Include
    BUILD_DEPENDENCIES
        PUBLIC
            3rdParty::PhysX
            AZ::AzCore
            AZ::AzFramework
            Legacy::CryCommon
            Gem::LmbrCentral
)

ly_add_target(
    NAME PhysX ${PAL_TRAIT_MONOLITHIC_DRIVEN_MODULE_TYPE}
    NAMESPACE Gem
    OUTPUT_NAME Gem.PhysX.4e08125824434932a0fe3717259caa47.v0.1.0
    FILES_CMAKE
        ${physx_shared_files}
    COMPILE_DEFINITIONS
        PUBLIC
            PHYSX_ENABLE_MULTI_THREADING
    INCLUDE_DIRECTORIES
        PRIVATE
            ${pal_source_dir}
        PUBLIC
            Include
    BUILD_DEPENDENCIES
        PRIVATE
            Gem::PhysX.Static
)

if(PAL_TRAIT_BUILD_HOST_TOOLS)

    ly_add_target(
        NAME PhysX.Editor.Static STATIC
        NAMESPACE Gem
        AUTOMOC
        AUTOUIC
        FILES_CMAKE
            ${physx_editor_files}
        COMPILE_DEFINITIONS
            PUBLIC
                PHYSX_EDITOR
                ENABLE_NON_COMPILED_CGF
        INCLUDE_DIRECTORIES
            PRIVATE
                .
                Source
                ${pal_source_dir}
            PUBLIC
                Include
        BUILD_DEPENDENCIES
            PUBLIC
                3rdParty::PhysX
                3rdParty::poly2tri
                3rdParty::vhacd
                Legacy::Editor.Headers
                AZ::AzCore
                AZ::AzFramework
                AZ::AzToolsFramework
                AZ::GFxFramework
                AZ::AssetBuilderSDK
                AZ::SceneCore
                AZ::SceneData
                Legacy::CryCommon
                Legacy::CryCommonTools
                Legacy::Cry3DEngine.CGF.Static
                Gem::LmbrCentral
                Gem::PhysX.NumericalMethods
                Gem::PhysX.Static
                Gem::AtomLyIntegration_CommonFeatures.Static
        RUNTIME_DEPENDENCIES
            AZ::SceneCore
    )

    ly_add_target(
        NAME PhysX.Editor MODULE
        NAMESPACE Gem
        AUTOMOC
        OUTPUT_NAME Gem.PhysX.Editor.4e08125824434932a0fe3717259caa47.v0.1.0
        FILES_CMAKE
            physx_editor_shared_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                .
                Source
                ${pal_source_dir}
            PUBLIC
                Include
        BUILD_DEPENDENCIES
            PRIVATE
                Gem::PhysX.Editor.Static
    )

endif()

################################################################################
# Tests
################################################################################
if(PAL_TRAIT_BUILD_TESTS_SUPPORTED)

    ly_add_target(
        NAME PhysX.Tests ${PAL_TRAIT_TEST_TARGET_TYPE}
        NAMESPACE Gem
        FILES_CMAKE
            physx_tests_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                Tests
        BUILD_DEPENDENCIES
            PRIVATE
                AZ::AzTestShared
                AZ::AzTest
                AZ::AzPhysicsTests
                Gem::PhysX.Static
        RUNTIME_DEPENDENCIES
            Gem::LmbrCentral
    )
    ly_add_googletest(
        NAME Gem::PhysX.Tests
    )
    ly_add_googlebenchmark(
        NAME Gem::PhysX.Benchmarks
        TARGET Gem::PhysX.Tests
        TIMEOUT 2400 #40mins
    )

    list(APPEND testTargets PhysX.Tests)
    
    if (PAL_TRAIT_BUILD_HOST_TOOLS)
        ly_add_target(
            NAME PhysX.Editor.Tests ${PAL_TRAIT_TEST_TARGET_TYPE}
            NAMESPACE Gem
            FILES_CMAKE
                physx_editor_tests_files.cmake
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
                    Gem::PhysX.Static
                    Gem::PhysX.Editor.Static
            RUNTIME_DEPENDENCIES
                Gem::LmbrCentral.Editor
        )
        ly_add_googletest(
            NAME Gem::PhysX.Editor.Tests
        )
    endif()
    
    ly_add_target_files(
        TARGETS
            ${testTargets}
        FILES
            ${CMAKE_CURRENT_SOURCE_DIR}/Tests/RagdollConfiguration.xml
        OUTPUT_SUBDIRECTORY
            Test.Assets/Gems/PhysX/Code/Tests
            )
endif()

ly_add_source_properties(
    SOURCES
        Editor/CollisionLayersWidget.cpp
        Source/Collision.cpp
        Source/Configuration/PhysXConfiguration.cpp
    PROPERTY COMPILE_DEFINITIONS 
    VALUES TOUCHBENDING_LAYER_BIT=${LY_TOUCHBENDING_LAYER_BIT}
)
