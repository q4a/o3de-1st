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

set(SCRIPT_CANVAS_DEV_COMMON_DEFINES
    SCRIPTCANVASDEVELOPER
    NOT_USE_CRY_MEMORY_MANAGER
    AZCORE_ENABLE_MEMORY_TRACKING
)

ly_add_target(
    NAME ScriptCanvasDeveloper.Static STATIC
    NAMESPACE Gem
    FILES_CMAKE
        scriptcanvasdeveloper_gem_common_files.cmake
    INCLUDE_DIRECTORIES
        PUBLIC
            Include
    COMPILE_DEFINITIONS
        PRIVATE
            ${SCRIPT_CANVAS_DEV_COMMON_DEFINES}
    BUILD_DEPENDENCIES
        PUBLIC
            Gem::ScriptCanvas
        PRIVATE
            AZ::AzCore
            AZ::AzFramework
)

ly_add_target(
    NAME ScriptCanvasDeveloper ${PAL_TRAIT_MONOLITHIC_DRIVEN_MODULE_TYPE}
    NAMESPACE Gem
    OUTPUT_NAME Gem.ScriptCanvasDeveloperGem.f905c05883b94fd6bddc91052c3c5a86.v0.1.0
    FILES_CMAKE
        scriptcanvasdeveloper_gem_game_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            Include
            Source
    COMPILE_DEFINITIONS
        PRIVATE
            ${SCRIPT_CANVAS_DEV_COMMON_DEFINES}
    BUILD_DEPENDENCIES
        PRIVATE
            AZ::AzCore
            Gem::ScriptCanvasDeveloper.Static
)

if(PAL_TRAIT_BUILD_HOST_TOOLS)
    ly_add_target(
        NAME ScriptCanvasDeveloper.Editor MODULE
        NAMESPACE Gem
        OUTPUT_NAME Gem.ScriptCanvasDeveloperGem.Editor.f905c05883b94fd6bddc91052c3c5a86.v0.1.0
        FILES_CMAKE
            scriptcanvasdeveloper_gem_editor_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                Source
                Editor/Include
                Editor/Source
        COMPILE_DEFINITIONS
            PRIVATE
                SCRIPTCANVASDEVELOPER_EDITOR
                ${SCRIPT_CANVAS_DEV_COMMON_DEFINES}
        BUILD_DEPENDENCIES
            PRIVATE
                AZ::AzCore
                AZ::AzFramework
                AZ::AzToolsFramework
                Gem::ScriptCanvasDeveloper.Static
                Gem::ScriptCanvasGem.Editor.Static
                Gem::GraphCanvasWidgets
    )
endif()
