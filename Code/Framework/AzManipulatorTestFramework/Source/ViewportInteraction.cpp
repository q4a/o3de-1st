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

#include <AzManipulatorTestFramework/ViewportInteraction.h>
#include <AzFramework/Viewport/ViewportScreen.h>
#include <AzFramework/Viewport/CameraState.h>
#include <AzToolsFramework/Manipulators/ManipulatorBus.h>

namespace AzManipulatorTestFramework
{
    // Null debug display for dummy draw calls
    class NullDebugDisplayRequests
        : public AzFramework::DebugDisplayRequests
    {
    public:
        virtual ~NullDebugDisplayRequests() = default;
    };

    ViewportInteraction::ViewportInteraction()
        : m_nullDebugDisplayRequests(AZStd::make_unique<NullDebugDisplayRequests>())
    {
        AzToolsFramework::ViewportInteraction::ViewportInteractionRequestBus::Handler::BusConnect(m_viewportId);
    }

    ViewportInteraction::~ViewportInteraction()
    {
        AzToolsFramework::ViewportInteraction::ViewportInteractionRequestBus::Handler::BusDisconnect();
    }

    AzFramework::CameraState ViewportInteraction::GetCameraState()
    {
        return m_cameraState;
    }

    bool ViewportInteraction::GridSnappingEnabled()
    {
        return m_gridSnapping;
    }

    float ViewportInteraction::GridSize()
    {
        return m_gridSize;
    }

    bool ViewportInteraction::ShowGrid()
    {
        return false;
    }

    bool ViewportInteraction::AngleSnappingEnabled()
    {
        return m_angularSnapping;
    }

    float ViewportInteraction::AngleStep()
    {
        return m_angularStep;
    }

    QPoint ViewportInteraction::ViewportWorldToScreen(const AZ::Vector3& worldPosition)
    {
        auto pos = AzFramework::WorldToScreen(worldPosition, m_cameraState);
        return QPoint(pos.m_x, pos.m_y);
    }

    void ViewportInteraction::SetCameraState(const AzFramework::CameraState& cameraState)
    {
        m_cameraState = cameraState;
    }

    AzFramework::DebugDisplayRequests& ViewportInteraction::GetDebugDisplay()
    {
        return *m_nullDebugDisplayRequests;
    }

    void ViewportInteraction::EnableGridSnaping()
    {
        m_gridSnapping = true;
    }

    void ViewportInteraction::DisableGridSnaping()
    {
        m_gridSnapping = false;
    }

    void ViewportInteraction::EnableAngularSnaping()
    {
        m_angularSnapping = true;
    }

    void ViewportInteraction::DisableAngularSnaping()
    {
        m_angularSnapping = false;
    }

    void ViewportInteraction::SetGridSize(float size)
    {
        m_gridSize = size;
    }

    void ViewportInteraction::SetAngularStep(float step)
    {
        m_angularStep = step;
    }

    int ViewportInteraction::GetViewportId() const
    {
        return m_viewportId;
    }

    AZStd::optional<AZ::Vector3> ViewportInteraction::ViewportScreenToWorld([[maybe_unused]]const QPoint& screenPosition, [[maybe_unused]]float depth)
    {
        return {};
    }

    AZStd::optional<AzToolsFramework::ViewportInteraction::ProjectedViewportRay> ViewportInteraction::ViewportScreenToWorldRay([[maybe_unused]]const QPoint& screenPosition)
    {
        return {};
    }

    QPoint ViewportInteraction::ViewportCursorScreenPosition()
    {
        return QPoint();
    }
} // namespace AzManipulatorTestFramework
