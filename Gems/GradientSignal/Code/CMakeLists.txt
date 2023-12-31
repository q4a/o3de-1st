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
    NAME GradientSignal.Static STATIC
    NAMESPACE Gem
    FILES_CMAKE
        gradientsignal_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            Source
        PUBLIC
            Include
    BUILD_DEPENDENCIES
        PUBLIC
            Legacy::CryCommon
            Gem::LmbrCentral
            Gem::SurfaceData
            Gem::ImageProcessing.Headers
            Gem::ImageProcessingAtom.Headers
)

ly_add_target(
    NAME GradientSignal ${PAL_TRAIT_MONOLITHIC_DRIVEN_MODULE_TYPE}
    NAMESPACE Gem
    OUTPUT_NAME Gem.GradientSignal.8825563d9d964ec3be3bab681f3bd9f2.v0.1.0
    FILES_CMAKE
        gradientsignal_shared_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            Source
        PUBLIC
            Include
    BUILD_DEPENDENCIES
        PRIVATE
            Gem::GradientSignal.Static
        PUBLIC
            Gem::ImageProcessing.Headers # ImageProcessing/PixelFormats.h is part of a header in Includes
            Gem::ImageProcessingAtom.Headers # Atom/ImageProcessing/PixelFormats.h is part of a header in Includes
)

if(PAL_TRAIT_BUILD_HOST_TOOLS)
    
    ly_add_target(
        NAME GradientSignal.Editor.Static STATIC
        NAMESPACE Gem
        FILES_CMAKE
            gradientsignal_editor_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                Source
            PUBLIC
                Include
        COMPILE_DEFINITIONS
            PUBLIC
                GRADIENTSIGNAL_EDITOR
        BUILD_DEPENDENCIES
            PUBLIC
                3rdParty::Qt::Widgets
                Legacy::CryCommon
                AZ::AzToolsFramework
                Gem::LmbrCentral
                Gem::SurfaceData
                AZ::AssetBuilderSDK
                Gem::GradientSignal.Static
    )

    ly_add_target(
        NAME GradientSignal.Editor MODULE
        NAMESPACE Gem
        OUTPUT_NAME Gem.GradientSignal.Editor.8825563d9d964ec3be3bab681f3bd9f2.v0.1.0
        FILES_CMAKE
            gradientsignal_editor_shared_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                Source
            PUBLIC
                Include
        BUILD_DEPENDENCIES
            PRIVATE
                Gem::GradientSignal.Editor.Static
    )

endif()

################################################################################
# Tests
################################################################################
if(PAL_TRAIT_BUILD_TESTS_SUPPORTED)

    ly_add_target(
        NAME GradientSignal.Tests ${PAL_TRAIT_TEST_TARGET_TYPE}
        NAMESPACE Gem
        FILES_CMAKE
            gradientsignal_tests_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                .
                Source
        BUILD_DEPENDENCIES
            PRIVATE
                AZ::AzTest
                Gem::GradientSignal.Static
    )
    ly_add_googletest(
        NAME Gem::GradientSignal.Tests
    )

    if(PAL_TRAIT_BUILD_HOST_TOOLS)
        ly_add_target(
            NAME GradientSignal.Editor.Tests ${PAL_TRAIT_TEST_TARGET_TYPE}
            NAMESPACE Gem
            FILES_CMAKE
                gradientsignal_editor_tests_files.cmake
            INCLUDE_DIRECTORIES
                PRIVATE
                    .
                    Source
            BUILD_DEPENDENCIES
                PRIVATE
                    AZ::AzTest
                    Gem::GradientSignal.Static
                    Gem::GradientSignal.Editor.Static
        )
        ly_add_googletest(
            NAME Gem::GradientSignal.Editor.Tests
        )
    endif()

endif()
