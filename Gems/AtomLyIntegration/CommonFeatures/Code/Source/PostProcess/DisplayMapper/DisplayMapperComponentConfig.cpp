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

#include <AtomLyIntegration/CommonFeatures/PostProcess/DisplayMapper/DisplayMapperComponentConfig.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>

namespace AZ
{
    namespace Render
    {
        void DisplayMapperComponentConfig::Reflect(ReflectContext* context)
        {
            if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<DisplayMapperComponentConfig, ComponentConfig>()
                    ->Version(0)
                    ->Field("DisplayMapperOperationType", &DisplayMapperComponentConfig::m_displayMapperOperation)
                    ->Field("LdrColorGradingLutEnabled", &DisplayMapperComponentConfig::m_ldrColorGradingLutEnabled)
                    ->Field("LdrColorGradingLut", &DisplayMapperComponentConfig::m_ldrColorGradingLut)
                    ;
            }
        }

    } // namespace Render
} // namespace AZ
