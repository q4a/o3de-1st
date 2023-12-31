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

#include "Vegetation_precompiled.h"
#include "EditorMeshBlockerComponent.h"
#include <AzCore/Serialization/Utils.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyEditorAPI.h>
#include <LmbrCentral/Dependency/DependencyNotificationBus.h>


namespace Vegetation
{
    void EditorMeshBlockerComponent::Reflect(AZ::ReflectContext* context)
    {
        BaseClassType::Reflect(context);

        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<EditorMeshBlockerComponent, BaseClassType>()
                ->Version(2, &EditorAreaComponentBaseVersionConverter<typename BaseClassType::WrappedComponentType, typename BaseClassType::WrappedConfigType>)
                ->Field("DrawDebugBounds", &EditorMeshBlockerComponent::m_drawDebugBounds)
                ;

            AZ::EditContext* edit = serialize->GetEditContext();
            if (edit)
            {
                edit->Class<EditorMeshBlockerComponent>(
                    s_componentName, s_componentDescription)
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Icon, s_icon)
                    ->Attribute(AZ::Edit::Attributes::ViewportIcon, s_viewportIcon)
                    ->Attribute(AZ::Edit::Attributes::HelpPageURL, s_helpUrl)
                    ->Attribute(AZ::Edit::Attributes::Category, s_categoryName)
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Game", 0x232b318c))
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(AZ::Edit::UIHandlers::CheckBox, &EditorMeshBlockerComponent::m_drawDebugBounds, "Draw Debug Bounds", "Show the settings to debug the mesh blocker")
                    ;
            }
        }
    }

    void EditorMeshBlockerComponent::Activate()
    {
        BaseClassType::Activate();
        AzFramework::EntityDebugDisplayEventBus::Handler::BusConnect(GetEntityId());
    }

    void EditorMeshBlockerComponent::Deactivate()
    {
        BaseClassType::Deactivate();
        AzFramework::EntityDebugDisplayEventBus::Handler::BusDisconnect();
    }

    void EditorMeshBlockerComponent::DisplayEntityViewport([[maybe_unused]] const AzFramework::ViewportInfo& viewportInfo, AzFramework::DebugDisplayRequests& debugDisplay)
    {
        if (!m_drawDebugBounds)
        {
            return;
        }

        // draw outline of complete mesh bounds
        if (m_component.m_meshBounds.IsValid())
        {
            debugDisplay.SetColor(AZ::Vector4(0.8f, 0.45f, 0.2f, 0.5f));
            debugDisplay.DrawWireBox(
                m_component.m_meshBounds.GetMin(),
                m_component.m_meshBounds.GetMax());
        }

        // draw filled box for bounds where intersections can occur
        if (m_component.m_meshBoundsForIntersection.IsValid())
        {
            debugDisplay.SetColor(AZ::Vector4(1.0f, 0.45f, 0.2f, 0.3f));
            debugDisplay.DrawSolidBox(
                m_component.m_meshBoundsForIntersection.GetMin(),
                m_component.m_meshBoundsForIntersection.GetMax());
        }
    }
}