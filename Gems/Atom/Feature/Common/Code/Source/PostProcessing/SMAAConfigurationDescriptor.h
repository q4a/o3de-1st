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

#include <Atom/Feature/PostProcessing/SMAAFeatureProcessorInterface.h>

namespace AZ
{

    namespace Render
    {
        //! A descriptor used to configure the SMAA feature
        struct SMAAConfigurationDescriptor final
        {
            AZ_TYPE_INFO(SMAAConfigurationDescriptor, "{0A546684-7E31-4C00-874C-7DFB3D12A0A6}");
            static void Reflect(AZ::ReflectContext* context);

            //! Configuration name
            AZStd::string m_name;

            uint32_t m_enable;
            uint32_t m_edgeDetectionMode;
            uint32_t m_outputMode;
            uint32_t m_quality;
        };
    } // namespace Render
} // namespace AZ
