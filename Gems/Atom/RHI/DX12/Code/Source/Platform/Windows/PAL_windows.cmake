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

# win2012 nodes can build DX11/DX12, but can't run. Disable runtime support if d3d12.dll/d3d11.dll is not available
set(PAL_TRAIT_ATOM_RHI_DX12_SUPPORTED FALSE)
find_file(d3d12_dll
    d3d12.dll
    PATHS
        "$ENV{SystemRoot}\\System32"
)
mark_as_advanced(d3d12_dll)
if(d3d12_dll)
    set(PAL_TRAIT_ATOM_RHI_DX12_SUPPORTED TRUE)
endif()

set(PAL_TRAIT_PIX_AVAILABLE FALSE)
find_file(pix3_header
    pix3.h
    PATHS
        "${CMAKE_CURRENT_SOURCE_DIR}/../External/pix/include/WinPixEventRuntime"
)
mark_as_advanced(pix3_header)
if(pix3_header)
    set(PAL_TRAIT_PIX_AVAILABLE TRUE)
endif()

# Disable windows OS version check until infra can upgrade all our jenkins nodes
# if(NOT CMAKE_SYSTEM_VERSION VERSION_GREATER_EQUAL "10.0.17763")
#   message(FATAL_ERROR "Windows DX12 RHI implementation requires an OS version and SDK matching windows 10 build 1809 or greater")
# endif()