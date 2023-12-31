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

if(NOT PAL_TRAIT_BUILD_HOST_TOOLS)
    return()
endif()

ly_get_list_relative_pal_filename(pal_source_dir ${CMAKE_CURRENT_LIST_DIR}/Source/Platform/${PAL_PLATFORM_NAME})

include(${pal_source_dir}/PAL_${PAL_PLATFORM_NAME_LOWERCASE}.cmake) # PAL_TRAIT_ATOM_MATERIAL_EDITOR_APPLICATION_SUPPORTED

if(NOT PAL_TRAIT_ATOM_MATERIAL_EDITOR_APPLICATION_SUPPORTED)
    return()
endif()


ly_add_target(
    NAME MaterialEditor.Document STATIC
    NAMESPACE Gem
    AUTOMOC
    FILES_CMAKE
        materialeditordocument_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            .
            Source
        PUBLIC
            Include
    BUILD_DEPENDENCIES
        PUBLIC
            Gem::AtomToolsFramework.Static
            Gem::AtomToolsFramework.Editor
            Gem::Atom_RPI.Edit
            Gem::Atom_RPI.Public
            Gem::Atom_RHI.Reflect
)

ly_add_target(
    NAME MaterialEditor.Window STATIC
    NAMESPACE Gem
    AUTOMOC
    AUTOUIC
    AUTORCC
    FILES_CMAKE
        materialeditorwindow_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            .
            Source
        PUBLIC
            Include
    BUILD_DEPENDENCIES
        PUBLIC
            Gem::AtomToolsFramework.Static
            Gem::AtomToolsFramework.Editor
            Gem::Atom_RPI.Public
            Gem::Atom_Feature_Common.Public
)

ly_add_target(
    NAME MaterialEditor.Viewport STATIC
    NAMESPACE Gem
    AUTOMOC
    AUTOUIC
   FILES_CMAKE
        materialeditorviewport_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            .
            Source
        Public
            Include
    BUILD_DEPENDENCIES
        PUBLIC
            Gem::AtomToolsFramework.Static
            Gem::AtomToolsFramework.Editor
            Gem::Atom_RHI.Public
            Gem::Atom_RPI.Public
            Gem::Atom_Feature_Common.Static
            Gem::Atom_Component_DebugCamera.Static
            Gem::AtomLyIntegration_CommonFeatures.Static
)

ly_add_target(
    NAME MaterialEditor EXECUTABLE
    NAMESPACE Gem
    FILES_CMAKE
        materialeditor_files.cmake
        Source/Platform/${PAL_PLATFORM_NAME}/platform_${PAL_PLATFORM_NAME_LOWERCASE}_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            .
            Source
            Source/Platform/${PAL_PLATFORM_NAME}
        PUBLIC
            Include
    BUILD_DEPENDENCIES
        PRIVATE
            Gem::AtomToolsFramework.Static
            Gem::AtomToolsFramework.Editor
            Gem::MaterialEditor.Window
            Gem::MaterialEditor.Viewport
            Gem::MaterialEditor.Document
)

# Add build dependency to Editor for the MaterialEditor application since
# Editor opens up the MaterialEditor
ly_add_dependencies(Editor Gem::MaterialEditor)

ly_add_target_files(
    TARGETS
        MaterialEditor
    FILES
        ${CMAKE_CURRENT_LIST_DIR}/../MaterialEditor.xml
    OUTPUT_SUBDIRECTORY
        Gems/Atom/Tools/MaterialEditor
)

ly_add_target_dependencies(
    TARGETS
        MaterialEditor
    DEPENDENCIES_FILES
        tool_dependencies.cmake
        Source/Platform/${PAL_PLATFORM_NAME}/tool_dependencies_${PAL_PLATFORM_NAME_LOWERCASE}.cmake
)

# Adds the MaterialEditor target as a C preprocessor define so that it can be used as a Settings Registry
# specialization in order to look up the generated .setreg which contains the dependencies
# specified for the target.
if(TARGET MaterialEditor)
    set_source_files_properties(
        Source/MaterialEditorApplication.cpp
        PROPERTIES
            COMPILE_DEFINITIONS
                LY_CMAKE_TARGET="MaterialEditor"
    )
else()
    message(FATAL_ERROR "Cannot set LY_CMAKE_TARGET define to MaterialEditor as the target doesn't exist anymore."
        " Perhaps it has been renamed")
endif()
