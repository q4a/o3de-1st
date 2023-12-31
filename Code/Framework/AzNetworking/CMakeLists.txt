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
    NAME AzNetworking STATIC
    NAMESPACE AZ
    FILES_CMAKE
        AzNetworking/aznetworking_files.cmake
        ${pal_dir}/platform_${PAL_PLATFORM_NAME_LOWERCASE}_files.cmake
    PLATFORM_INCLUDE_FILES
        ${pal_dir}/platform_${PAL_PLATFORM_NAME_LOWERCASE}.cmake
    INCLUDE_DIRECTORIES
        PUBLIC
            .
            ${common_dir}
            ${pal_dir}
    BUILD_DEPENDENCIES
        PRIVATE
            AZ::AzCore
            3rdParty::zlib
            3rdParty::zstd
            3rdParty::OpenSSL
        PUBLIC
            3rdParty::RapidJSON
            3rdParty::RapidXML
    AUTOGEN_RULES
        *.AutoPackets.xml,AutoPackets_Header.jinja,$path/$fileprefix.AutoPackets.h
        *.AutoPackets.xml,AutoPackets_Inline.jinja,$path/$fileprefix.AutoPackets.inl
        *.AutoPackets.xml,AutoPackets_Source.jinja,$path/$fileprefix.AutoPackets.cpp
)

# Add ENABLE_MULTIPLAYER_COMPRESSION define to the following source files when the LY_ENABLE_MULTIPLAYER_COMPRESSION cache variable has been set
if($CACHE{LY_ENABLE_MULTIPLAYER_COMPRESSION})
    set_property(SOURCE
        CompressionCommon.cpp
        #...
        APPEND PROPERTY 
            COMPILE_DEFINITIONS ENABLE_MULTIPLAYER_COMPRESSION
    )
endif()

################################################################################
# Tests
################################################################################
if(PAL_TRAIT_BUILD_TESTS_SUPPORTED)
    ly_add_target(
        NAME AzNetworking.Tests ${PAL_TRAIT_TEST_TARGET_TYPE}
        NAMESPACE AZ
        FILES_CMAKE
            Tests/aznetworkingtests_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                Tests
        COMPILE_DEFINITIONS
            PRIVATE
                AZ_NUMERICCAST_ENABLED
        BUILD_DEPENDENCIES
            PRIVATE
                AZ::AzNetworking
                AZ::AzTestShared
                AZ::AzTest
    )

    ly_add_googletest(
        NAME AZ::AzNetworking.Tests
    )
endif()

