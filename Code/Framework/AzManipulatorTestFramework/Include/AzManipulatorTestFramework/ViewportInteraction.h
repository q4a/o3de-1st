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

#include <AzManipulatorTestFramework/AzManipulatorTestFramework.h>

namespace AzManipulatorTestFramework
{
    class NullDebugDisplayRequests;

    //! Implementation of the viewport interaction model to handle viewport interaction requests.
    class ViewportInteraction
        : public ViewportInteractionInterface
        , private AzToolsFramework::ViewportInteraction::ViewportInteractionRequestBus::Handler
    {
    public:
        ViewportInteraction();
        ~ViewportInteraction();

        // ViewportInteractionInterface ...
        AzFramework::CameraState GetCameraState() override;
        void SetCameraState(const AzFramework::CameraState& cameraState) override;
        AzFramework::DebugDisplayRequests& GetDebugDisplay() override;
        void EnableGridSnaping() override;
        void DisableGridSnaping() override;
        void EnableAngularSnaping() override;
        void DisableAngularSnaping() override;
        void SetGridSize(float size) override;
        void SetAngularStep(float step) override;
        int GetViewportId() const override;
        AZStd::optional<AZ::Vector3> ViewportScreenToWorld(const QPoint& screenPosition, float depth) override;
        AZStd::optional<AzToolsFramework::ViewportInteraction::ProjectedViewportRay> ViewportScreenToWorldRay(const QPoint& screenPosition) override;
        QPoint ViewportCursorScreenPosition() override;
    private:
        // ViewportInteractionRequestBus ...
        bool GridSnappingEnabled();
        float GridSize();
        bool ShowGrid();
        bool AngleSnappingEnabled();
        float AngleStep();
        QPoint ViewportWorldToScreen(const AZ::Vector3& worldPosition);
    private:
        AZStd::unique_ptr<NullDebugDisplayRequests> m_nullDebugDisplayRequests;
        const int m_viewportId = 1234; // Arbitrary viewport id for manipulator tests
        AzFramework::CameraState m_cameraState;
        bool m_gridSnapping = false;
        bool m_angularSnapping = false;
        float m_gridSize = 1.0f;
        float m_angularStep = 0.0f;
    };
} // namespace AzManipulatorTestFramework
