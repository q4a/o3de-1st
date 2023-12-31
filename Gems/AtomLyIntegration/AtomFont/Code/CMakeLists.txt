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

ly_add_target(
    NAME AtomFont ${PAL_TRAIT_MONOLITHIC_DRIVEN_LIBRARY_TYPE}
    NAMESPACE Gem
    FILES_CMAKE
        atomfont_files.cmake
        ${pal_dir}/platform_${PAL_PLATFORM_NAME_LOWERCASE}_files.cmake
    INCLUDE_DIRECTORIES
        PUBLIC
            Include
            Source
    BUILD_DEPENDENCIES
        PRIVATE
            3rdParty::FreeType2
            AZ::AzCore
            AZ::AtomCore
            Legacy::CryCommon
            Gem::Atom_RHI.Reflect
            Gem::Atom_RPI.Public
            Gem::Atom_Bootstrap.Headers
)

################################################################################
# Tests
################################################################################
if(PAL_TRAIT_BUILD_TESTS_SUPPORTED)

    ly_add_target(
        NAME AtomFont.Tests ${PAL_TRAIT_TEST_TARGET_TYPE}
        NAMESPACE Gem
        FILES_CMAKE
            atomfont_test_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                Include
                Tests
        BUILD_DEPENDENCIES
            PRIVATE
                AZ::AzTest
    )
    ly_add_googletest(
        NAME Gem::AtomFont.Tests
    )
endif()

