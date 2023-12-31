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

ly_get_list_relative_pal_filename(common_dir ${CMAKE_CURRENT_LIST_DIR}/Platform/Common)
ly_get_list_relative_pal_filename(pal_dir ${CMAKE_CURRENT_LIST_DIR}/Platform/${PAL_PLATFORM_NAME})

ly_add_target(
    NAME LmbrCentral.Static STATIC
    NAMESPACE Gem
    FILES_CMAKE
        lmbrcentral_files.cmake
        ${pal_dir}/platform_${PAL_PLATFORM_NAME_LOWERCASE}_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            Source
            ${pal_dir}
        PUBLIC
            include
    BUILD_DEPENDENCIES
        PRIVATE
            Legacy::CryCommon
            AZ::AzFramework
)

ly_add_target(
    NAME LmbrCentral ${PAL_TRAIT_MONOLITHIC_DRIVEN_MODULE_TYPE}
    NAMESPACE Gem
    OUTPUT_NAME Gem.LmbrCentral.ff06785f7145416b9d46fde39098cb0c.v0.1.0
    FILES_CMAKE
        lmbrcentral_shared_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            Source
            ${pal_dir}
        PUBLIC
            include
    BUILD_DEPENDENCIES
        PRIVATE
            Legacy::CryCommon
            AZ::AzFramework
            Gem::LmbrCentral.Static
)

if (PAL_TRAIT_BUILD_HOST_TOOLS)
    ly_add_target(
        NAME LmbrCentral.Editor.Static STATIC
        NAMESPACE Gem
        FILES_CMAKE
            lmbrcentral_editor_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                Source
                ${pal_dir}
            PUBLIC
                include
        COMPILE_DEFINITIONS
            PUBLIC
                LMBR_CENTRAL_EDITOR
        BUILD_DEPENDENCIES
            PRIVATE
                ${additional_dependencies}
            PUBLIC
                3rdParty::Qt::Widgets
                Legacy::CryCommon
                Legacy::Editor.Headers
                AZ::AzToolsFramework
                AZ::AssetBuilderSDK
    )

    ly_add_target(
        NAME LmbrCentral.Editor MODULE
        NAMESPACE Gem
        OUTPUT_NAME Gem.LmbrCentral.Editor.ff06785f7145416b9d46fde39098cb0c.v0.1.0
        FILES_CMAKE
            lmbrcentral_editor_shared_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                Source
                ${pal_dir}
            PUBLIC
                include
        BUILD_DEPENDENCIES
            PRIVATE
                3rdParty::Qt::Gui
                AZ::AzCore
                Gem::LmbrCentral.Static
                Gem::LmbrCentral.Editor.Static
                ${additional_dependencies}
    )

    unset(QT_LRELEASE_EXECUTABLE CACHE)
    find_program(QT_LRELEASE_EXECUTABLE lrelease HINTS "${QT_PATH}/bin")
    mark_as_advanced(QT_LRELEASE_EXECUTABLE) # Hiding from GUI
    if(NOT QT_LRELEASE_EXECUTABLE)
        message(FATAL_ERROR "Qt's lrelease executbale not found")
    endif()
    include(${pal_dir}/lrelease_${PAL_PLATFORM_NAME_LOWERCASE}.cmake)

    ly_add_target_files(
        TARGETS LmbrCentral.Editor
        FILES ${QT_LRELEASE_EXECUTABLE}
    )

endif()

################################################################################
# Tests
################################################################################
if(PAL_TRAIT_BUILD_TESTS_SUPPORTED)
    ly_add_target(
        NAME LmbrCentral.Tests ${PAL_TRAIT_TEST_TARGET_TYPE}
        NAMESPACE Gem
        FILES_CMAKE
            lmbrcentral_tests_files.cmake
            lmbrcentral_shared_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                Source
                Tests
        BUILD_DEPENDENCIES
            PRIVATE
                AZ::AzTest
                AZ::AzTestShared
                Legacy::CryCommon
                AZ::AzFramework
                Gem::LmbrCentral.Static
    )
    ly_add_googletest(
        NAME Gem::LmbrCentral.Tests
    )

    if (PAL_TRAIT_BUILD_HOST_TOOLS)
        ly_add_target(
            NAME LmbrCentral.Editor.Tests ${PAL_TRAIT_TEST_TARGET_TYPE}
            NAMESPACE Gem
            FILES_CMAKE
                lmbrcentral_editor_tests_files.cmake
            INCLUDE_DIRECTORIES
                PRIVATE
                    .
                    Source
                    Tests
            COMPILE_DEFINITIONS
                PRIVATE
                    LMBR_CENTRAL_EDITOR
            BUILD_DEPENDENCIES
                PRIVATE
                    3rdParty::Qt::Gui
                    3rdParty::Qt::Widgets
                    Legacy::CryCommon
                    Legacy::Editor.Headers
                    AZ::AzTest
                    AZ::AzCore
                    AZ::AzTestShared
                    AZ::AzToolsFramework
                    AZ::AzToolsFrameworkTestCommon
                    AZ::AssetBuilderSDK
                    Gem::LmbrCentral.Static
                    Gem::LmbrCentral.Editor.Static
        )
        ly_add_googletest(
            NAME Gem::LmbrCentral.Editor.Tests
        )
    endif()
endif()
