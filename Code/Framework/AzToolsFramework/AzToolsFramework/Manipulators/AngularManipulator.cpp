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

#include "AngularManipulator.h"

#include <AzFramework/Entity/EntityDebugDisplayBus.h>
#include <AzToolsFramework/Manipulators/ManipulatorView.h>
#include <AzToolsFramework/Manipulators/ManipulatorSnapping.h>

namespace AzToolsFramework
{
    static const float s_circularRotateThresholdDegrees = 80.0f;

    AngularManipulator::ActionInternal AngularManipulator::CalculateManipulationDataStart(
        const Fixed& fixed, const AZ::Transform& worldFromLocal, const AZ::Transform& localTransform,
        const AZ::Vector3& rayOrigin, const AZ::Vector3& rayDirection, const float rayDistance)
    {
        const AZ::Transform worldFromLocalWithTransform = worldFromLocal * localTransform;
        const AZ::Vector3 worldAxis = TransformDirectionNoScaling(worldFromLocalWithTransform, fixed.m_axis);

        ActionInternal actionInternal;
        // default plane normal and point used for rotation
        actionInternal.m_start.m_planeNormal = worldAxis;
        actionInternal.m_start.m_planePoint = worldFromLocalWithTransform.GetTranslation();

        // if angular manipulator axis is at right angles to us, use initial ray direction
        // as plane normal and use hit position on manipulator as plane point
        const float pickAngle = AZ::RadToDeg(AZ::Acos(AZ::Abs(rayDirection.Dot(worldAxis))));
        if (pickAngle > s_circularRotateThresholdDegrees)
        {
            actionInternal.m_start.m_planeNormal = -rayDirection;
            actionInternal.m_start.m_planePoint = rayOrigin + rayDirection * rayDistance;
        }

        // store initial world hit position
        Internal::CalculateRayPlaneIntersectingPoint(
            rayOrigin, rayDirection, actionInternal.m_start.m_planePoint,
            actionInternal.m_start.m_planeNormal, actionInternal.m_current.m_worldHitPosition);

        // store entity transform (to go from local to world space)
        // and store our own starting local transform
        actionInternal.m_start.m_worldFromLocal = worldFromLocal;
        actionInternal.m_start.m_localTransform = localTransform;
        actionInternal.m_current.m_radians = 0.0f;

        return actionInternal;
    }

    AngularManipulator::Action AngularManipulator::CalculateManipulationDataAction(
        const Fixed& fixed, ActionInternal& actionInternal, const AZ::Transform& worldFromLocal,
        const AZ::Transform& localTransform, const bool snapping, const float angleStepDegrees,
        const AZ::Vector3& rayOrigin, const AZ::Vector3& rayDirection,
        const ViewportInteraction::KeyboardModifiers keyboardModifiers)
    {
        const AZ::Transform worldFromLocalWithTransform = worldFromLocal * localTransform;
        const AZ::Vector3 worldAxis = TransformDirectionNoScaling(worldFromLocalWithTransform, fixed.m_axis);

        AZ::Vector3 worldHitPosition = AZ::Vector3::CreateZero();
        Internal::CalculateRayPlaneIntersectingPoint(rayOrigin, rayDirection,
            actionInternal.m_start.m_planePoint, actionInternal.m_start.m_planeNormal,
            worldHitPosition);

        // get vector from center of rotation for current and previous frame
        const AZ::Vector3 center = worldFromLocalWithTransform.GetTranslation();
        const AZ::Vector3 currentWorldHitVector = (worldHitPosition - center).GetNormalizedSafe();
        const AZ::Vector3 previousWorldHitVector =
            (actionInternal.m_current.m_worldHitPosition - center).GetNormalizedSafe();

        // calculate which direction we rotated
        const AZ::Vector3 worldAxisRight = worldAxis.Cross(previousWorldHitVector);
        const float rotateSign = Sign(currentWorldHitVector.Dot(worldAxisRight));
        // how far did we rotate this frame
        const float rotationAngleRad = AZ::Acos(AZ::GetMin<float>(
            1.0f, currentWorldHitVector.Dot(previousWorldHitVector)));
        actionInternal.m_current.m_worldHitPosition = worldHitPosition;

        // if we're snapping, only increment current radians when we know
        // preSnapRadians is greater than the angleStep
        if (snapping)
        {
            actionInternal.m_current.m_preSnapRadians += rotationAngleRad * rotateSign;

            const float angleStepRad = AZ::DegToRad(angleStepDegrees);
            const float preSnapRotateSign = Sign(actionInternal.m_current.m_preSnapRadians);
            // if we move more than angleStep in a frame, make sure we catch up
            while (fabsf(actionInternal.m_current.m_preSnapRadians) >= angleStepRad)
            {
                actionInternal.m_current.m_radians += angleStepRad * preSnapRotateSign;
                actionInternal.m_current.m_preSnapRadians -= angleStepRad * preSnapRotateSign;
            }
        }
        else
        {
            // no snapping, just update current radius immediately
            actionInternal.m_current.m_radians += rotationAngleRad * rotateSign;
        }

        Action action;
        action.m_start.m_space = actionInternal.m_start.m_worldFromLocal.GetRotation().GetNormalized();
        action.m_start.m_rotation = actionInternal.m_start.m_localTransform.GetRotation().GetNormalized();
        action.m_current.m_delta = AZ::Quaternion::CreateFromAxisAngle(fixed.m_axis, actionInternal.m_current.m_radians).GetNormalized();
        action.m_modifiers = keyboardModifiers;

        return action;
    }

    AZStd::shared_ptr<AngularManipulator> AngularManipulator::MakeShared(const AZ::Transform& worldFromLocal)
    {
        return AZStd::shared_ptr<AngularManipulator>(aznew AngularManipulator(worldFromLocal));
    }

    AngularManipulator::AngularManipulator(const AZ::Transform& worldFromLocal)
        : m_worldFromLocal(worldFromLocal)
    {
        AttachLeftMouseDownImpl();
    }

    void AngularManipulator::InstallLeftMouseDownCallback(const MouseActionCallback& onMouseDownCallback)
    {
        m_onLeftMouseDownCallback = onMouseDownCallback;
    }

    void AngularManipulator::InstallLeftMouseUpCallback(const MouseActionCallback& onMouseUpCallback)
    {
        m_onLeftMouseUpCallback = onMouseUpCallback;
    }

    void AngularManipulator::InstallMouseMoveCallback(const MouseActionCallback& onMouseMoveCallback)
    {
        m_onMouseMoveCallback = onMouseMoveCallback;
    }

    void AngularManipulator::OnLeftMouseDownImpl(
        const ViewportInteraction::MouseInteraction& interaction, const float rayIntersectionDistance)
    {
        const bool snapping = AngleSnapping(interaction.m_interactionId.m_viewportId);
        const float angleStep = AngleStep(interaction.m_interactionId.m_viewportId);

        // calculate initial state when mouse press first happens
        m_actionInternal = CalculateManipulationDataStart(
            m_fixed, TransformNormalizedScale(m_worldFromLocal), TransformNormalizedScale(m_localTransform),
            interaction.m_mousePick.m_rayOrigin, interaction.m_mousePick.m_rayDirection,
            rayIntersectionDistance);

        if (m_onLeftMouseDownCallback)
        {
            m_onLeftMouseDownCallback(CalculateManipulationDataAction(
                m_fixed, m_actionInternal, m_actionInternal.m_start.m_worldFromLocal,
                m_actionInternal.m_start.m_localTransform, snapping, angleStep,
                interaction.m_mousePick.m_rayOrigin, interaction.m_mousePick.m_rayDirection,
                interaction.m_keyboardModifiers));
        }
    }

    void AngularManipulator::OnMouseMoveImpl(const ViewportInteraction::MouseInteraction& interaction)
    {
        if (m_onMouseMoveCallback)
        {
            // calculate delta rotation
            m_onMouseMoveCallback(CalculateManipulationDataAction(
                m_fixed, m_actionInternal, m_actionInternal.m_start.m_worldFromLocal,
                m_actionInternal.m_start.m_localTransform,
                AngleSnapping(interaction.m_interactionId.m_viewportId),
                AngleStep(interaction.m_interactionId.m_viewportId),
                interaction.m_mousePick.m_rayOrigin, interaction.m_mousePick.m_rayDirection,
                interaction.m_keyboardModifiers));
        }
    }

    void AngularManipulator::OnLeftMouseUpImpl(const ViewportInteraction::MouseInteraction& interaction)
    {
        if (m_onLeftMouseUpCallback)
        {
            m_onLeftMouseUpCallback(CalculateManipulationDataAction(
                m_fixed, m_actionInternal, m_actionInternal.m_start.m_worldFromLocal,
                m_actionInternal.m_start.m_localTransform,
                AngleSnapping(interaction.m_interactionId.m_viewportId),
                AngleStep(interaction.m_interactionId.m_viewportId),
                interaction.m_mousePick.m_rayOrigin, interaction.m_mousePick.m_rayDirection,
                interaction.m_keyboardModifiers));
        }
    }

    void AngularManipulator::Draw(
        const ManipulatorManagerState& managerState,
        AzFramework::DebugDisplayRequests& debugDisplay,
        const AzFramework::CameraState& cameraState,
        const ViewportInteraction::MouseInteraction& mouseInteraction)
    {
        m_manipulatorView->Draw(
            GetManipulatorManagerId(), managerState,
            GetManipulatorId(), {
                m_worldFromLocal * m_localTransform,
                AZ::Vector3::CreateZero(), MouseOver()
            },
            debugDisplay, cameraState, mouseInteraction);
    }

    void AngularManipulator::SetAxis(const AZ::Vector3& axis)
    {
        m_fixed.m_axis = axis;
    }

    void AngularManipulator::SetSpace(const AZ::Transform& worldFromLocal)
    {
        m_worldFromLocal = worldFromLocal;
    }

    void AngularManipulator::SetLocalTransform(const AZ::Transform& localTransform)
    {
        m_localTransform = localTransform;
    }

    void AngularManipulator::SetLocalPosition(const AZ::Vector3& localPosition)
    {
        m_localTransform.SetTranslation(localPosition);
    }

    void AngularManipulator::SetLocalOrientation(const AZ::Quaternion& localOrientation)
    {
        m_localTransform = AZ::Transform::CreateFromQuaternionAndTranslation(
            localOrientation, m_localTransform.GetTranslation());
    }

    void AngularManipulator::SetView(AZStd::unique_ptr<ManipulatorView>&& view)
    {
        m_manipulatorView = AZStd::move(view);
    }

    void AngularManipulator::SetBoundsDirtyImpl()
    {
        m_manipulatorView->SetBoundDirty(GetManipulatorManagerId());
    }

    void AngularManipulator::InvalidateImpl()
    {
        m_manipulatorView->Invalidate(GetManipulatorManagerId());
    }

} // namespace AzToolsFramework
