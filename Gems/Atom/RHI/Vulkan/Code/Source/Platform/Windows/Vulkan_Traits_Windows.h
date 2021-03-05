/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
#pragma once

#define AZ_TRAIT_ATOM_SHADERBUILDER_DXC "Builders/DirectXShaderCompilerAz/dxc.exe"
#define AZ_TRAIT_ATOM_VULKAN_DISABLE_DUAL_SOURCE_BLENDING 0
#define AZ_TRAIT_ATOM_VULKAN_DLL "vulkan.dll"
#define AZ_TRAIT_ATOM_VULKAN_DLL_1 "vulkan-1.dll"
#define AZ_TRAIT_ATOM_VULKAN_LAYER_LUNARG_STD_VALIDATION_SUPPORT 1
#define AZ_TRAIT_ATOM_VULKAN_MIN_GPU_MEM (4 * 1024 * 1024 * 1024LL) //4GB