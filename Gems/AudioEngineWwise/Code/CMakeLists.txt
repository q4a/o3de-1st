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
ly_get_list_relative_pal_filename(common_dir ${CMAKE_CURRENT_LIST_DIR}/Platform/Common)

include(${pal_dir}/PAL_${PAL_PLATFORM_NAME_LOWERCASE}.cmake) #for PAL_TRAIT_AUDIO_ENGINE_WWISE Traits

set(AUDIOENGINEWWISE_COMPILEDEFINITIONS
    $<IF:$<CONFIG:Release>,WWISE_RELEASE,ENABLE_AUDIO_LOGGING>
)

################################################################################
# Server / Unsupported
################################################################################
if (PAL_TRAIT_BUILD_SERVER_SUPPORTED OR PAL_TRAIT_AUDIO_ENGINE_WWISE_USE_STUB)
    # Stub gem for server and unsupported platforms. Audio Engine Wwise is client only
    ly_add_target(
        NAME AudioEngineWwise.Stub ${PAL_TRAIT_MONOLITHIC_DRIVEN_MODULE_TYPE}
        NAMESPACE Gem
        OUTPUT_NAME Gem.AudioEngineWwise.Stub
        FILES_CMAKE
            audioenginewwise_stub_files.cmake
        BUILD_DEPENDENCIES
            PRIVATE
                AZ::AzCore
    )
endif()

if (PAL_TRAIT_AUDIO_ENGINE_WWISE_USE_STUB)
    add_library(Gem::AudioEngineWwise ALIAS AudioEngineWwise.Stub) #setup an alias so the stub will be used if something references AudioEngineWwise
    return()
endif()

ly_add_target(
    NAME AudioEngineWwise.Static STATIC
    NAMESPACE Gem
    OUTPUT_NAME Gem.AudioEngineWwise.Static
    FILES_CMAKE
        audioenginewwise_files.cmake
        ${pal_dir}/platform_${PAL_PLATFORM_NAME_LOWERCASE}_files.cmake
    PLATFORM_INCLUDE_FILES
        ${pal_dir}/platform_${PAL_PLATFORM_NAME_LOWERCASE}.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            Source
            Source/Engine
            ${pal_dir}
            ${common_dir}
    COMPILE_DEFINITIONS
        PUBLIC
            ${AUDIOENGINEWWISE_COMPILEDEFINITIONS}
    BUILD_DEPENDENCIES
        PUBLIC
            3rdParty::Wwise
            Legacy::CryCommon
            Gem::AudioSystem.Static
)

ly_add_target(
    NAME AudioEngineWwise ${PAL_TRAIT_MONOLITHIC_DRIVEN_MODULE_TYPE}
    NAMESPACE Gem
    OUTPUT_NAME Gem.AudioEngineWwise.67a80e2ac865406c990f2715feb55f7f.v0.1.0
    FILES_CMAKE
        audioenginewwise_shared_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            Source
            Source/Engine
            ${pal_dir}
            ${common_dir}
    COMPILE_DEFINITIONS
        PUBLIC
            ${AUDIOENGINEWWISE_COMPILEDEFINITIONS}
    BUILD_DEPENDENCIES
        PRIVATE
            Gem::AudioEngineWwise.Static
)

################################################################################
# Tests
################################################################################
if (PAL_TRAIT_BUILD_TESTS_SUPPORTED)
   ly_add_target(
        NAME AudioEngineWwise.Tests ${PAL_TRAIT_TEST_TARGET_TYPE}
        NAMESPACE Gem
        FILES_CMAKE
            audioenginewwise_tests_files.cmake
            audioenginewwise_shared_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                Tests
                Source
                Source/Engine
                ${pal_dir}
                ${common_dir}
        BUILD_DEPENDENCIES
            PRIVATE
                AZ::AzTest
                Gem::AudioEngineWwise.Static
    )
    ly_add_googletest(
        NAME Gem::AudioEngineWwise.Tests
    )

    list(APPEND testTargets AudioEngineWwise.Tests)

    ly_add_target_files(
        TARGETS
            ${testTargets}
        FILES
            ${CMAKE_CURRENT_SOURCE_DIR}/Tests/AudioControls/EmptyControl.xml
            ${CMAKE_CURRENT_SOURCE_DIR}/Tests/AudioControls/MissingPreloads.xml
            ${CMAKE_CURRENT_SOURCE_DIR}/Tests/AudioControls/MissingWwiseFileNode.xml
            ${CMAKE_CURRENT_SOURCE_DIR}/Tests/AudioControls/MultiplePreloadsMultipleBanks.xml
            ${CMAKE_CURRENT_SOURCE_DIR}/Tests/AudioControls/MultiplePreloadsOneBank.xml
            ${CMAKE_CURRENT_SOURCE_DIR}/Tests/AudioControls/OnePreloadMultipleBanks.xml
            ${CMAKE_CURRENT_SOURCE_DIR}/Tests/AudioControls/OnePreloadOneBank.xml
        OUTPUT_SUBDIRECTORY
            Test.Assets/Gems/AudioEngineWwise/AudioControls
    )

    ly_add_target_files(
        TARGETS
            ${testTargets}
        FILES
            ${CMAKE_CURRENT_SOURCE_DIR}/Tests/AudioControls/Legacy/MissingAtlPlatformsNode.xml
            ${CMAKE_CURRENT_SOURCE_DIR}/Tests/AudioControls/Legacy/MissingConfigGroupNameAttribute.xml
            ${CMAKE_CURRENT_SOURCE_DIR}/Tests/AudioControls/Legacy/MissingPlatformNameAttributeOnePreload.xml
            ${CMAKE_CURRENT_SOURCE_DIR}/Tests/AudioControls/Legacy/MissingPlatformNode.xml
            ${CMAKE_CURRENT_SOURCE_DIR}/Tests/AudioControls/Legacy/MissingWwiseFileNode.xml
            ${CMAKE_CURRENT_SOURCE_DIR}/Tests/AudioControls/Legacy/MultiplePreloadsMultipleBanks.xml
            ${CMAKE_CURRENT_SOURCE_DIR}/Tests/AudioControls/Legacy/MultiplePreloadsOneBank.xml
            ${CMAKE_CURRENT_SOURCE_DIR}/Tests/AudioControls/Legacy/NoConfigGroups.xml
            ${CMAKE_CURRENT_SOURCE_DIR}/Tests/AudioControls/Legacy/OnePreloadMultipleBanks.xml
            ${CMAKE_CURRENT_SOURCE_DIR}/Tests/AudioControls/Legacy/OnePreloadOneBank.xml
            ${CMAKE_CURRENT_SOURCE_DIR}/Tests/AudioControls/Legacy/WrongConfigGroup.xml
        OUTPUT_SUBDIRECTORY
            Test.Assets/Gems/AudioEngineWwise/AudioControls/Legacy
    )

    ly_add_target_files(
        TARGETS
            ${testTargets}
        FILES
            ${CMAKE_CURRENT_SOURCE_DIR}/Tests/Sounds/wwise/test_bank1.bankdeps
            ${CMAKE_CURRENT_SOURCE_DIR}/Tests/Sounds/wwise/test_bank2.bankdeps
            ${CMAKE_CURRENT_SOURCE_DIR}/Tests/Sounds/wwise/test_bank3.bankdeps
            ${CMAKE_CURRENT_SOURCE_DIR}/Tests/Sounds/wwise/test_bank4.bankdeps
            ${CMAKE_CURRENT_SOURCE_DIR}/Tests/Sounds/wwise/test_bank5.bankdeps
            ${CMAKE_CURRENT_SOURCE_DIR}/Tests/Sounds/wwise/test_bank6.bankdeps
            ${CMAKE_CURRENT_SOURCE_DIR}/Tests/Sounds/wwise/test_bank7.bankdeps
            ${CMAKE_CURRENT_SOURCE_DIR}/Tests/Sounds/wwise/test_bank8.bankdeps
            ${CMAKE_CURRENT_SOURCE_DIR}/Tests/Sounds/wwise/test_bank9.bankdeps
        OUTPUT_SUBDIRECTORY
            Test.Assets/Gems/AudioEngineWwise/sounds/wwise
    )

    ly_add_target_files(
        TARGETS
            ${testTargets}
        FILES
            ${CMAKE_CURRENT_SOURCE_DIR}/Tests/Sounds//wwise/soundbanks/init.bnk
        OUTPUT_SUBDIRECTORY
            Test.Assets/Gems/AudioEngineWwise/sounds/wwise/soundbanks
    )

endif()

################################################################################
# Editor
################################################################################
if (PAL_TRAIT_BUILD_HOST_TOOLS)
    ly_add_target(
        NAME AudioEngineWwise.Editor.Static STATIC
        NAMESPACE Gem
        AUTORCC
        FILES_CMAKE
            audioenginewwise_editor_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                Source/Engine
                Source/Editor
                ${pal_dir}
                Source
        BUILD_DEPENDENCIES
            PRIVATE
                Legacy::EditorCore
            PUBLIC
                AZ::AssetBuilderSDK
                Gem::AudioEngineWwise.Static
                Gem::AudioSystem.Editor
    )

    ly_add_target(
        NAME AudioEngineWwise.Editor MODULE
        NAMESPACE Gem
        OUTPUT_NAME Gem.AudioEngineWwise.Editor.67a80e2ac865406c990f2715feb55f7f.v0.1.0
        FILES_CMAKE
            audioenginewwise_editor_shared_files.cmake
        COMPILE_DEFINITIONS
            PRIVATE
                AUDIO_ENGINE_WWISE_EDITOR
        INCLUDE_DIRECTORIES
            PRIVATE
                Source/Builder
                Source/Engine
                Source/Editor
                ${pal_dir}
                Source
        BUILD_DEPENDENCIES
            PRIVATE
                Gem::AudioEngineWwise.Editor.Static
    )

    if (PAL_TRAIT_BUILD_TESTS_SUPPORTED)
        ly_add_target(
            NAME AudioEngineWwise.Editor.Tests ${PAL_TRAIT_TEST_TARGET_TYPE}
            NAMESPACE Gem
            FILES_CMAKE
                audioenginewwise_editor_tests_files.cmake
            INCLUDE_DIRECTORIES
                PRIVATE
                    Tests
                    Source/Editor
                    Source
            BUILD_DEPENDENCIES
                PRIVATE
                    AZ::AzTest
                    Gem::AudioEngineWwise.Editor.Static
                    Legacy::CryCommon
        )
        ly_add_googletest(
            NAME Gem::AudioEngineWwise.Editor.Tests
        )
    endif()
endif()