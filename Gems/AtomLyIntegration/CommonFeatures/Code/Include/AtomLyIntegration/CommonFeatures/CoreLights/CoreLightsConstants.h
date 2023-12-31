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

#include <AzCore/Math/MathUtils.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <Atom/Feature/CoreLights/CoreLightsConstants.h>

namespace AZ
{
    namespace Render
    {
        static constexpr const char* const AreaLightComponentTypeId = "{744B3961-6242-4461-983F-2817D9D29C30}";
        static constexpr const char* const EditorAreaLightComponentTypeId = "{8B605C0C-9027-4E0B-BA8C-19E396F8F262}";
        static constexpr const char* const PointLightComponentTypeId = "{0A0E44AB-F583-481F-8AE8-68C4B1F9CD05}";
        static constexpr const char* const EditorPointLightComponentTypeId = "{C4D354BE-5247-41FD-9A8D-550C6772EE5B}";
        static constexpr const char* const SpotLightComponentTypeId = "{441DF0EC-6B70-451E-AEBE-6452A17BB852}";
        static constexpr const char* const EditorSpotLightComponentTypeId = "{9A32D37B-C5D2-43A7-B574-E2EA1CDC7D64}";
        static constexpr const char* const DirectionalLightComponentTypeId = "{13054592-2753-46C2-B19E-59670D4CE03D}";
        static constexpr const char* const EditorDirectionalLightComponentTypeId = "{45B97527-6E72-411B-BC23-00068CF01580}";

        enum class LightAttenuationRadiusMode : uint8_t
        {
            Explicit,
            Automatic,
        };

        inline void CoreLightConstantsReflect(ReflectContext* context)
        {
            if (auto behaviorContext = azrtti_cast<BehaviorContext*>(context))
            {
                behaviorContext
                    ->Enum<(int)LightAttenuationRadiusMode::Automatic>("LightAttenuationRadiusMode_Automatic")
                    ->Enum<(int)LightAttenuationRadiusMode::Explicit>("LightAttenuationRadiusMode_Explicit")
                    ;
            }
        }

    } // namespace Render
} // namespace AZ
