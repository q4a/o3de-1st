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
    NAME GemRegistry.Static STATIC
    NAMESPACE AZ
    FILES_CMAKE
        gemregistry_files.cmake
    INCLUDE_DIRECTORIES
        PUBLIC
            include
    BUILD_DEPENDENCIES
        PUBLIC
            AZ::AzCore
            AZ::AzFramework
)

ly_add_target(
    NAME GemRegistry ${PAL_TRAIT_MONOLITHIC_DRIVEN_MODULE_TYPE}
    NAMESPACE AZ
    FILES_CMAKE
        gemregistry_shared_files.cmake
    INCLUDE_DIRECTORIES
        PUBLIC
            include
    BUILD_DEPENDENCIES
        PRIVATE
            AZ::GemRegistry.Static
)

################################################################################
# Tests
################################################################################
if(PAL_TRAIT_BUILD_TESTS_SUPPORTED)

    ly_add_target(
        NAME GemRegistry.Tests ${PAL_TRAIT_TEST_TARGET_TYPE}
        NAMESPACE AZ
        FILES_CMAKE
            gemregistry_test_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                Tests
                source
        BUILD_DEPENDENCIES
            PRIVATE
                AZ::AzTest
                AZ::GemRegistry.Static
    )
    ly_add_googletest(
        NAME AZ::GemRegistry.Tests
    )
endif()

