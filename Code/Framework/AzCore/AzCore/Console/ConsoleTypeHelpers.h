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

#include <AzCore/base.h>
#include <AzCore/Console/IConsoleTypes.h>
#include <AzCore/std/string/string.h>

namespace AZ
{
    namespace ConsoleTypeHelpers
    {
        //! Helper function for converting a typed value to a string representation.
        //! @param value the value instance to convert to a string
        //! @return the string representation of the value
        template <typename TYPE>
        CVarFixedString ValueToString(const TYPE& value);

        //! Helper function for converting a set of strings to a value.
        //! @param outValue  the value instance to write to
        //! @param arguments the value instance to convert to a string
        //! @return boolean true on success, false if there was a conversion error
        template <typename TYPE>
        bool StringSetToValue(TYPE& outValue, const AZ::ConsoleCommandContainer& arguments);

        //! Helper function for converting a typed value to a string representation.
        //! @param outValue the value instance to write to
        //! @param string   the string to 
        //! @return boolean true on success, false if there was a conversion error
        template <typename _TYPE>
        bool StringToValue(_TYPE& outValue, AZStd::string_view string);
    }
}

#include <AzCore/Console/ConsoleTypeHelpers.inl>
