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

set(AUDIOSYSTEM_COMPILEDEFINITIONS
    $<IF:$<CONFIG:Release>,AUDIO_RELEASE,ENABLE_AUDIO_LOGGING>
)

ly_get_list_relative_pal_filename(pal_dir ${CMAKE_CURRENT_LIST_DIR}/Platform/${PAL_PLATFORM_NAME})

ly_add_target(
    NAME AudioSystem.Static STATIC
    NAMESPACE Gem
    OUTPUT_NAME Gem.AudioSystem.Static
    FILES_CMAKE
        audiosystem_files.cmake
        ${pal_dir}/platform_${PAL_PLATFORM_NAME_LOWERCASE}_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            Source
            Source/Engine
            ${pal_dir}
        PUBLIC
            Include
            Include/Engine
    PLATFORM_INCLUDE_FILES
            ${pal_dir}/platform_${PAL_PLATFORM_NAME_LOWERCASE}.cmake
    COMPILE_DEFINITIONS
        PUBLIC
            ${AUDIOSYSTEM_COMPILEDEFINITIONS}
    BUILD_DEPENDENCIES
        PRIVATE
            Legacy::CryCommon
)

ly_add_target(
    NAME AudioSystem ${PAL_TRAIT_MONOLITHIC_DRIVEN_MODULE_TYPE}
    NAMESPACE Gem
    OUTPUT_NAME Gem.AudioSystem.6f63f2b6d07f4b89b4b7c86ebee7feb8.v0.1.0
    FILES_CMAKE
        audiosystem_shared_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            Source
            Source/Engine
            ${pal_dir}
        PUBLIC
            Include
            Include/Engine
    COMPILE_DEFINITIONS
        PUBLIC
            ${AUDIOSYSTEM_COMPILEDEFINITIONS}
    BUILD_DEPENDENCIES
        PRIVATE
            Legacy::CryCommon
            Gem::AudioSystem.Static
)

################################################################################
# Server
################################################################################
if (PAL_TRAIT_BUILD_SERVER_SUPPORTED)
    # Stub gem for server. Audio system is client only
    ly_add_target(
        NAME AudioSystem.Server MODULE
        NAMESPACE Gem
        OUTPUT_NAME Gem.AudioSystem.Server.6f63f2b6d07f4b89b4b7c86ebee7feb8.v0.1.0
        FILES_CMAKE
            audiosystem_stub_files.cmake
        BUILD_DEPENDENCIES
            PRIVATE
                AZ::AzCore
    )
endif ()

################################################################################
# Tests
################################################################################
if (PAL_TRAIT_BUILD_TESTS_SUPPORTED)
    ly_get_list_relative_pal_filename(common_dir ${CMAKE_CURRENT_LIST_DIR}/Platform/Common)
    ly_add_target(
        NAME AudioSystem.Tests ${PAL_TRAIT_TEST_TARGET_TYPE}
        NAMESPACE Gem
        FILES_CMAKE
            audiosystem_tests_files.cmake
            audiosystem_shared_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                Tests
                Source
                Source/Engine
                Include/Engine
        BUILD_DEPENDENCIES
            PRIVATE
                AZ::AzTest
                AZ::AzFramework
                Legacy::CryCommon
                Gem::AudioSystem.Static
                Gem::LmbrCentral
    )
    ly_add_googletest(
        NAME Gem::AudioSystem.Tests
    )

    list(APPEND testTargets AudioSystem.Tests)

    ly_add_target_files(
        TARGETS
            ${testTargets}
        FILES
            ${CMAKE_CURRENT_SOURCE_DIR}/Tests/ATLData/Legacy/MultipleMultiple/ATLTestControls.xml
        OUTPUT_SUBDIRECTORY
            Test.Assets/Gems/AudioSystem/ATLData/Legacy/MultipleMultiple
    )
    ly_add_target_files(
        TARGETS
            ${testTargets}
        FILES
            ${CMAKE_CURRENT_SOURCE_DIR}/Tests/ATLData/Legacy/MultipleOne/ATLTestControls.xml
        OUTPUT_SUBDIRECTORY
            Test.Assets/Gems/AudioSystem/ATLData/Legacy/MultipleOne
    )
    ly_add_target_files(
        TARGETS
            ${testTargets}
        FILES
            ${CMAKE_CURRENT_SOURCE_DIR}/Tests/ATLData/Legacy/OneMultiple/ATLTestControls.xml
        OUTPUT_SUBDIRECTORY
            Test.Assets/Gems/AudioSystem/ATLData/Legacy/OneMultiple
    )
    ly_add_target_files(
        TARGETS
            ${testTargets}
        FILES
            ${CMAKE_CURRENT_SOURCE_DIR}/Tests/ATLData/Legacy/OneOne/ATLTestControls.xml
        OUTPUT_SUBDIRECTORY
            Test.Assets/Gems/AudioSystem/ATLData/Legacy/OneOne
    )
    ly_add_target_files(
        TARGETS
            ${testTargets}
        FILES
            ${CMAKE_CURRENT_SOURCE_DIR}/Tests/ATLData/MultipleMultiple/ATLTestControls.xml
        OUTPUT_SUBDIRECTORY
            Test.Assets/Gems/AudioSystem/ATLData/MultipleMultiple
    )
    ly_add_target_files(
        TARGETS
            ${testTargets}
        FILES
            ${CMAKE_CURRENT_SOURCE_DIR}/Tests/ATLData/MultipleOne/ATLTestControls.xml
        OUTPUT_SUBDIRECTORY
            Test.Assets/Gems/AudioSystem/ATLData/MultipleOne
    )
    ly_add_target_files(
        TARGETS
            ${testTargets}
        FILES
            ${CMAKE_CURRENT_SOURCE_DIR}/Tests/ATLData/OneMultiple/ATLTestControls.xml
        OUTPUT_SUBDIRECTORY
            Test.Assets/Gems/AudioSystem/ATLData/OneMultiple
    )
    ly_add_target_files(
        TARGETS
            ${testTargets}
        FILES
            ${CMAKE_CURRENT_SOURCE_DIR}/Tests/ATLData/OneOne/ATLTestControls.xml
        OUTPUT_SUBDIRECTORY
            Test.Assets/Gems/AudioSystem/ATLData/OneOne
    )
endif ()

################################################################################
# Editor
################################################################################
if (PAL_TRAIT_BUILD_HOST_TOOLS)
    ly_add_target(
        NAME AudioSystem.Editor.Static STATIC
        NAMESPACE Gem
        AUTOMOC
        AUTOUIC
        AUTORCC
        FILES_CMAKE
            audiosystem_editor_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                .
                Source
                Source/Engine
                Include/Engine
                Source/Editor
                ${pal_dir}
            PUBLIC
                Include/Editor
        COMPILE_DEFINITIONS
            PUBLIC
                AUDIO_SYSTEM_EDITOR
        BUILD_DEPENDENCIES
            PUBLIC
                3rdParty::Qt::Core
                3rdParty::Qt::Gui
                3rdParty::Qt::Widgets
                AZ::AzCore
                AZ::AzToolsFramework
                Legacy::CryCommon
                Legacy::EditorCore
                Legacy::EditorCommon
                Gem::AudioSystem.Static
    )
    ly_add_target(
        NAME AudioSystem.Editor MODULE
        NAMESPACE Gem
        OUTPUT_NAME Gem.AudioSystem.Editor.6f63f2b6d07f4b89b4b7c86ebee7feb8.v0.1.0
        FILES_CMAKE
            audiosystem_editor_shared_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                Source
                Source/Editor
            PUBLIC
                Include/Editor
        BUILD_DEPENDENCIES
            PRIVATE
                3rdParty::Qt::Core
                Gem::AudioSystem.Editor.Static
    )

    if (PAL_TRAIT_BUILD_TESTS_SUPPORTED)
        ly_add_target(
            NAME AudioSystem.Editor.Tests ${PAL_TRAIT_TEST_TARGET_TYPE}
            NAMESPACE Gem
            FILES_CMAKE
                audiosystem_editor_tests_files.cmake
            INCLUDE_DIRECTORIES
                PRIVATE
                    Tests
                    Source/Editor
                    Source/Audio
                    Source
            BUILD_DEPENDENCIES
                PRIVATE
                    AZ::AzTest
                    3rdParty::Qt::Core
                    Gem::AudioSystem.Editor.Static
        )
        ly_add_googletest(
            NAME Gem::AudioSystem.Editor.Tests
        )
    endif()
endif ()