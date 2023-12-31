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
    NAME Atom_AtomBridge.Static STATIC
    NAMESPACE Gem
    FILES_CMAKE
        atombridge_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            Source
        PUBLIC
            Include
    COMPILE_DEFINITIONS
        PRIVATE
            ENABLE_ATOM_DEBUG_DISPLAY=0
    BUILD_DEPENDENCIES
        PUBLIC
            AZ::AtomCore
            AZ::AzFramework
            Gem::Atom_RPI.Public
            Gem::Atom_Bootstrap.Headers
            Legacy::CryCommon
)

ly_add_target(
    NAME Atom_AtomBridge ${PAL_TRAIT_MONOLITHIC_DRIVEN_MODULE_TYPE}
    NAMESPACE Gem
    OUTPUT_NAME Gem.Atom_AtomBridge.b55b2738aa4a46c8b034fe98e6e5158b.v0.1.0
    FILES_CMAKE
        atombridge_shared_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            Source
        PUBLIC
            Include
    COMPILE_DEFINITIONS
        PRIVATE
            ENABLE_ATOM_DEBUG_DISPLAY=0
    BUILD_DEPENDENCIES
        PRIVATE
            Gem::Atom_AtomBridge.Static
)

if(PAL_TRAIT_BUILD_HOST_TOOLS)
    ly_add_target(
        NAME Atom_AtomBridge.Editor ${PAL_TRAIT_MONOLITHIC_DRIVEN_MODULE_TYPE}
        NAMESPACE Gem
        OUTPUT_NAME Gem.Atom_AtomBridge.Editor.b55b2738aa4a46c8b034fe98e6e5158b.v0.1.0
        FILES_CMAKE
            atombridge_editor_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                Source
            PUBLIC
                Include
        COMPILE_DEFINITIONS
            PRIVATE
                EDITOR
        BUILD_DEPENDENCIES
            PRIVATE
                AZ::AssetBuilderSDK
                Gem::Atom_Utils.Static
                Gem::Atom_AtomBridge.Static
    )
endif()
