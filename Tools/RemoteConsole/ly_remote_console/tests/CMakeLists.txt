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
# Ly Remote Console tests.
#

# Unit tests.
ly_add_pytest(
    NAME RemoteConsole_UnitTests_main_no_gpu
    PATH ${CMAKE_CURRENT_LIST_DIR}/unit/
)

if(PAL_TRAIT_BUILD_HOST_TOOLS AND PAL_TRAIT_BUILD_TESTS_SUPPORTED AND AutomatedTesting IN_LIST LY_PROJECTS)
    # Integration tests.
#    ly_add_pytest(
#        NAME RemoteConsole_IntegTests_periodic_no_gpu
#        PATH ${CMAKE_CURRENT_LIST_DIR}/integ/test_remote_console.py
#        TEST_SERIAL
#        TEST_SUITE periodic
#        RUNTIME_DEPENDENCIES
#            Legacy::Editor
#            AssetProcessor
#            AutomatedTesting.GameLauncher
#            AutomatedTesting.Assets
#            Legacy::CryRenderNULL
#    )
endif()