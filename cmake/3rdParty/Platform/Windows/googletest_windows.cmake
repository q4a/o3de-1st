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

set(GOOGLETEST_LIB_PATH ${BASE_PATH}/lib/Windows/$<IF:$<CONFIG:Debug>,Debug,Release>)
set(GOOGLETEST_LINK_OPTIONS $<$<STREQUAL:${PAL_TRAIT_COMPILER_ID},Clang>:-Wl,>/ignore:4099)

set(GOOGLETEST_GTEST_LIBS ${GOOGLETEST_LIB_PATH}/$<IF:$<CONFIG:Debug>,gtestd,gtest>.lib)
set(GOOGLETEST_GTESTMAIN_LIBS ${GOOGLETEST_LIB_PATH}/$<IF:$<CONFIG:Debug>,gtest_maind,gtest_main>.lib)
set(GOOGLETEST_GMOCK_LIBS ${GOOGLETEST_LIB_PATH}/$<IF:$<CONFIG:Debug>,gmockd,gmock>.lib)
set(GOOGLETEST_GMOCKMAIN_LIBS ${GOOGLETEST_LIB_PATH}/$<IF:$<CONFIG:Debug>,gmockd,gmock>.lib)