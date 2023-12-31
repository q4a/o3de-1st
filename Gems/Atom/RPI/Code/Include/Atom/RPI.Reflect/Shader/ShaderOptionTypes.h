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

#include <AzCore/RTTI/TypeInfo.h>
#include <AzCore/Name/Name.h>
#include <Atom/RHI.Reflect/Handle.h>

namespace AZ
{
    namespace RPI
    {
        using ShaderOptionIndex = RHI::Handle<uint32_t, class ShaderOptionIndexNamespace>;  //!< ShaderOption index in the group layout
        using ShaderOptionValue = RHI::Handle<uint32_t, class ShaderOptionValueNamespace>;  //!< Numerical representation for a single value in the ShaderOption
        using ShaderOptionValuePair = AZStd::pair<Name/*valueName*/, ShaderOptionValue>;  //!< Provides a string representation for a ShaderOptionValue

        enum class ShaderOptionType : uint32_t
        {
            Unknown,
            Boolean,
            Enumeration,
            IntegerRange,
        };

        const char* ToString(ShaderOptionType shaderOptionType);

    } // namespace RPI

    AZ_TYPE_INFO_SPECIALIZE(RPI::ShaderOptionIndexNamespace, "{CE66656A-CDC3-4B62-9B50-3B9CC014DCE7}");
    AZ_TYPE_INFO_SPECIALIZE(RPI::ShaderOptionValueNamespace, "{154874D8-D9D0-4D57-A22E-55174FFC003F}");
} // namespace AZ
