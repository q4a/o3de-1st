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

#include <AzToolsFramework/ToolsComponents/EditorComponentAdapter.h>
#include <PostProcess/ShapeWeightModifier/ShapeWeightModifierComponent.h>

namespace AZ
{
    namespace Render
    {
        class EditorShapeWeightModifierComponent final
            : public AzToolsFramework::Components::EditorComponentAdapter<ShapeWeightModifierComponentController, ShapeWeightModifierComponent, ShapeWeightModifierComponentConfig>
        {
        public:
            using BaseClass = AzToolsFramework::Components::EditorComponentAdapter<ShapeWeightModifierComponentController, ShapeWeightModifierComponent, ShapeWeightModifierComponentConfig>;
            AZ_EDITOR_COMPONENT(AZ::Render::EditorShapeWeightModifierComponent, EditorShapeWeightModifierComponentTypeId, BaseClass);

            static void Reflect(AZ::ReflectContext* context);

            EditorShapeWeightModifierComponent() = default;
            EditorShapeWeightModifierComponent(const ShapeWeightModifierComponentConfig& config);

            AZ::u32 OnConfigurationChanged() override;
        };
    }
}
