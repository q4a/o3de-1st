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
    NAME CustomAssetExample ${PAL_TRAIT_MONOLITHIC_DRIVEN_MODULE_TYPE}
    NAMESPACE Gem
    OUTPUT_NAME Gem.CustomAssetExample.ad082dd50c6545849729e9afeaaeaa1d.v0.1.0
    FILES_CMAKE
        customassetexample_shared_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            Source
        PUBLIC
            Include
    BUILD_DEPENDENCIES
        PRIVATE
            AZ::AzCore
)

if(PAL_TRAIT_BUILD_HOST_TOOLS)
    ly_add_target(
        NAME CustomAssetExample.Editor MODULE
        NAMESPACE Gem
        OUTPUT_NAME Gem.CustomAssetExample.Editor.ad082dd50c6545849729e9afeaaeaa1d.v0.1.0
        FILES_CMAKE
            customassetexample_editor_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                Source
            PUBLIC
                Include
        BUILD_DEPENDENCIES
            PRIVATE
                AZ::AzCore
                AZ::AssetBuilderSDK
    )
endif()
