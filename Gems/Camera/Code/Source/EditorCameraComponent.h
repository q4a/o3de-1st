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

#include <AzCore/Component/Component.h>
#include <AzToolsFramework/ToolsComponents/EditorComponentAdapter.h>
#include <AzCore/Math/Color.h>

#include <AzFramework/Entity/EntityDebugDisplayBus.h>
#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/API/EditorCameraBus.h>
#include <AzFramework/Viewport/ViewportColors.h>
#include <AzFramework/Components/CameraBus.h>

#include <AzFramework/Components/EditorEntityEvents.h>
#include "CameraComponent.h"
#include "CameraComponentController.h"
#include <IViewSystem.h>
#include <Cry_Camera.h>

#include <Atom/RPI.Public/Base.h>

namespace Camera
{
    //////////////////////////////////////////////////////////////////////////
    /// The CameraComponent holds all of the data necessary for a camera.
    /// Get and set data through the CameraRequestBus or TransformBus
    //////////////////////////////////////////////////////////////////////////
    using EditorCameraComponentBase = AzToolsFramework::Components::EditorComponentAdapter<CameraComponentController, CameraComponent, CameraComponentConfig>;
    class EditorCameraComponent
        : public EditorCameraComponentBase
        , public EditorCameraViewRequestBus::Handler
        , private AzFramework::EntityDebugDisplayEventBus::Handler
        , private EditorCameraNotificationBus::Handler
    {
    public:
        AZ_EDITOR_COMPONENT(EditorCameraComponent, EditorCameraComponentTypeId, AzToolsFramework::Components::EditorComponentBase);
        virtual ~EditorCameraComponent() = default;

        static void Reflect(AZ::ReflectContext* reflection);

        // AZ::Component interface
        void Activate() override;
        void Deactivate() override;
        AZ::u32 OnConfigurationChanged() override;

        // AzFramework::DebugDisplayRequestBus::Handler interface
        void DisplayEntityViewport(
            const AzFramework::ViewportInfo& viewportInfo,
            AzFramework::DebugDisplayRequests& debugDisplay) override;

        /// EditorCameraNotificationBus::Handler interface
        void OnViewportViewEntityChanged(const AZ::EntityId& newViewId) override;

        void ToggleCameraAsActiveView() override { OnPossessCameraButtonClicked(); }

    protected:
        void EditorDisplay(AzFramework::DebugDisplayRequests& displayInterface, const AZ::Transform& world);
        AZ::Crc32 OnPossessCameraButtonClicked();
        AZStd::string GetCameraViewButtonText() const;

        bool m_isActiveEditorCamera = false;
        float m_frustumViewPercentLength = 1.f;
        AZ::Color m_frustumDrawColor = AzFramework::ViewportColors::HoverColor;
    };
} // Camera