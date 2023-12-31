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

#include <AzCore/Memory/Memory.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzCore/std/string/string.h>
#include <Atom/RPI.Reflect/Model/ModelAsset.h>

namespace AZ
{
    namespace Render
    {
        //! ModelPreset describes a model that can be displayed in the viewport
        struct ModelPreset final
        {
            AZ_TYPE_INFO(AZ::Render::ModelPreset, "{A7304AE2-EC26-44A4-8C00-89D9731CCB13}");
            AZ_CLASS_ALLOCATOR(ModelPreset, AZ::SystemAllocator, 0);
            static void Reflect(AZ::ReflectContext* context);

            bool m_autoSelect = false;
            AZStd::string m_displayName;
            AZ::Data::Asset<AZ::RPI::ModelAsset> m_modelAsset;
        };

        using ModelPresetPtr = AZStd::shared_ptr<ModelPreset>;
        using ModelPresetPtrVector = AZStd::vector<ModelPresetPtr>;
    }
}
