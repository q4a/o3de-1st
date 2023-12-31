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

ly_get_list_relative_pal_filename(pal_dir ${CMAKE_CURRENT_LIST_DIR}/Platform/${PAL_PLATFORM_NAME})

include(${pal_dir}/PAL_${PAL_PLATFORM_NAME_LOWERCASE}.cmake)

if(NOT PAL_TRAIT_BUILD_CRASH_HANDLER_SUPPORTED)
    return()
endif()

add_subdirectory(Support)

ly_add_target(
    NAME CrashHandler STATIC
    NAMESPACE AZ
    FILES_CMAKE
        crash_handler_files.cmake
        ${pal_dir}/crash_handler_${PAL_PLATFORM_NAME_LOWERCASE}_files.cmake
        ${pal_dir}/platform_${PAL_PLATFORM_NAME_LOWERCASE}_files.cmake
    INCLUDE_DIRECTORIES
        PUBLIC
            Shared
        PRIVATE
            ${pal_dir}
    BUILD_DEPENDENCIES
        PRIVATE
            3rdParty::Crashpad
            AZ::AzCore
            AZ::AzFramework
            AZ::CrashSupport
)

string(REPLACE "." ";" version_list "${LY_VERSION_STRING}")
list(GET version_list 0 EXE_VERSION_INFO_0)
list(GET version_list 1 EXE_VERSION_INFO_1)
list(GET version_list 2 EXE_VERSION_INFO_2)
list(GET version_list 3 EXE_VERSION_INFO_3)

ly_add_source_properties(
    SOURCES Shared/CrashHandler.cpp
    PROPERTY COMPILE_DEFINITIONS 
    VALUES
        LY_BUILD=${LY_VERSION_BUILD_NUMBER}
        EXE_VERSION_INFO_0=${EXE_VERSION_INFO_0}
        EXE_VERSION_INFO_1=${EXE_VERSION_INFO_1}
        EXE_VERSION_INFO_2=${EXE_VERSION_INFO_2}
        EXE_VERSION_INFO_3=${EXE_VERSION_INFO_3}
)

ly_add_target(
    NAME CrashUploaderSupport STATIC
    NAMESPACE AZ
    FILES_CMAKE
        crash_uploader_support_files.cmake
    INCLUDE_DIRECTORIES
        PUBLIC
            Uploader/include
        PRIVATE
            Uploader/include/Uploader
    BUILD_DEPENDENCIES
        PRIVATE
            3rdParty::Crashpad::Handler
            AZ::AzCore
            AZ::CrashSupport
)

add_subdirectory(Tools)
