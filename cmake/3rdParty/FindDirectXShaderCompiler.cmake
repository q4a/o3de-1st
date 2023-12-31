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

ly_add_external_target(
    NAME dxcGL
    PACKAGE DirectXShaderCompiler
    VERSION 1.0.1-az.1
)

ly_add_external_target(
    NAME dxcMetal
    PACKAGE DirectXShaderCompiler
    VERSION 1.0.1-az.1
)

# In this case, dxc, the target uses OUTPUT_SUBDIRECTORY, because all the RUNTIME_DEPENDENCIES will be
# copied to the same output subfolder
ly_add_external_target(
    NAME dxc
    PACKAGE DirectXShaderCompiler
    VERSION 2020.08.07
    OUTPUT_SUBDIRECTORY
        Builders/DirectXShaderCompiler
)

# For dxcAz, OUTPUT_SUBDIRECTORY is NOT used, because the RUNTIME_DEPENDENCIES will be copied to
# two different directories.
ly_add_external_target(
    NAME dxcAz
    PACKAGE DirectXShaderCompiler
    VERSION 5.0.0-az
)


