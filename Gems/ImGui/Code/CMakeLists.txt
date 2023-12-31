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

set(config_base_defines $<IF:$<CONFIG:release>,IMGUI_DISABLED,IMGUI_ENABLED>)

ly_get_list_relative_pal_filename(pal_source_dir ${CMAKE_CURRENT_LIST_DIR}/Source/Platform/${PAL_PLATFORM_NAME})

# This library is the 3rdParty imgui that is in the Gem's External
ly_add_target(
    NAME ImGui.imguilib ${PAL_TRAIT_MONOLITHIC_DRIVEN_LIBRARY_TYPE}
    NAMESPACE Gem
    OUTPUT_NAME imguilib
    FILES_CMAKE
        imgui_lib_files.cmake
        ${pal_source_dir}/platform_${PAL_PLATFORM_NAME_LOWERCASE}_files.cmake
    PLATFORM_INCLUDE_FILES
        ${pal_source_dir}/imgui_${PAL_PLATFORM_NAME_LOWERCASE}.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            Source
        INTERFACE
            ../External/ImGui/v1.70
        PUBLIC
            Include
    COMPILE_DEFINITIONS
        PUBLIC
            ${config_base_defines}
        INTERFACE
            $<$<NOT:$<BOOL:${LY_MONOLITHIC_GAME}>>:IMGUI_API_IMPORT>
            IMGUI_INCLUDE_IMGUI_USER_H
    BUILD_DEPENDENCIES
        PRIVATE
            AZ::AzCore
)

ly_add_target(
    NAME ImGui.ImGuiLYUtils STATIC
    NAMESPACE Gem
    FILES_CMAKE
        imgui_lyutils_static_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            Include
            Source
    BUILD_DEPENDENCIES
        PUBLIC
            Gem::ImGui.imguilib
            Legacy::CryCommon
)

ly_add_target(
    NAME ImGui.Static STATIC
    NAMESPACE Gem
    FILES_CMAKE
        imgui_common_files.cmake
    PLATFORM_INCLUDE_FILES
        ${pal_source_dir}/platform_${PAL_PLATFORM_NAME_LOWERCASE}.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            Source
        PUBLIC
            Include
    BUILD_DEPENDENCIES
        PUBLIC
            Gem::ImGui.ImGuiLYUtils
            Gem::LmbrCentral
)

ly_add_target(
    NAME ImGui ${PAL_TRAIT_MONOLITHIC_DRIVEN_MODULE_TYPE}
    NAMESPACE Gem
    OUTPUT_NAME Gem.ImGui.bab8807a1bc646b3909f3cc200ffeedf.v0.1.0
    FILES_CMAKE
        imgui_shared_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            Source
        PUBLIC 
            Include
    BUILD_DEPENDENCIES
        PRIVATE
            Gem::ImGui.Static
)

if(PAL_TRAIT_BUILD_HOST_TOOLS)
    ly_add_target(
        NAME ImGui.Editor MODULE
        NAMESPACE Gem
        OUTPUT_NAME Gem.ImGui.Editor.bab8807a1bc646b3909f3cc200ffeedf.v0.1.0
        AUTOMOC
        AUTOUIC
        FILES_CMAKE
            imgui_editor_files.cmake
        COMPILE_DEFINITIONS
            PRIVATE
                IMGUI_GEM_EDITOR
        INCLUDE_DIRECTORIES
            PRIVATE
                .
                Editor
                Source
            PUBLIC
                Include
        BUILD_DEPENDENCIES
            PRIVATE
                Gem::ImGui.Static
                3rdParty::Qt::Widgets
                AZ::AzToolsFramework
                Legacy::Editor.Headers
    )
endif()
