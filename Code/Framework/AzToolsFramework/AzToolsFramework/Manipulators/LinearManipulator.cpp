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

#include "LinearManipulator.h"

#include <AzFramework/Entity/EntityDebugDisplayBus.h>
#include <AzToolsFramework/Manipulators/ManipulatorDebug.h>
#include <AzToolsFramework/Manipulators/ManipulatorSnapping.h>
#include <AzToolsFramework/Maths/TransformUtils.h>
#include <AzToolsFramework/Viewport/ViewportMessages.h>
#include <AzToolsFramework/ViewportSelection/EditorSelectionUtil.h>

namespace AzToolsFramework
{
    LinearManipulator::Starter CalculateLinearManipulationDataStart(
        const LinearManipulator::Fixed& fixed, const AZ::Transform& worldFromLocal, const AZ::Transform& localTransform,
        const GridSnapAction& gridSnapAction, const ViewportInteraction::MouseInteraction& interaction,
        const float intersectionDistance, const AzFramework::CameraState& cameraState)
    {
        const ManipulatorInteraction manipulatorInteraction =
            BuildManipulatorInteraction(
                worldFromLocal, interaction.m_mousePick.m_rayOrigin, interaction.m_mousePick.m_rayDirection);

        const AZ::Vector3 axis = TransformDirectionNoScaling(localTransform, fixed.m_axis);
        const AZ::Vector3 rayCrossAxis = manipulatorInteraction.m_localRayDirection.Cross(axis);

        LinearManipulator::Start start;
        LinearManipulator::StartTransition startTransition;
        // initialize m_localHitPosition to handle edge case where CalculateRayPlaneIntersectingPoint
        // fails because ray is parallel to the plane
        start.m_localHitPosition = localTransform.GetTranslation();
        startTransition.m_localNormal = rayCrossAxis.Cross(axis).GetNormalizedSafe();

        // initial intersect point
        const AZ::Vector3 localIntersectionPoint =
            manipulatorInteraction.m_localRayOrigin + manipulatorInteraction.m_localRayDirection * intersectionDistance;

        Internal::CalculateRayPlaneIntersectingPoint(
            manipulatorInteraction.m_localRayOrigin, manipulatorInteraction.m_localRayDirection,
            localIntersectionPoint, startTransition.m_localNormal, start.m_localHitPosition);

        const float gridSize = gridSnapAction.m_gridSnapParams.m_gridSize;
        const bool snapping = gridSnapAction.m_gridSnapParams.m_gridSnap;
        const float scaleRecip = manipulatorInteraction.m_scaleReciprocal;

        // calculate position amount to snap, to align with grid
        const AZ::Vector3 positionSnapOffset = snapping && !gridSnapAction.m_localSnapping
            ? CalculateSnappedOffset(localTransform.GetTranslation(), axis, gridSize * scaleRecip)
            : AZ::Vector3::CreateZero();

        const AZ::Vector3 localScale = localTransform.GetScale();
        const AZ::Quaternion localRotation = QuaternionFromTransformNoScaling(localTransform);
        // calculate scale amount to snap, to align to round scale value
        const AZ::Vector3 scaleSnapOffset = snapping && !gridSnapAction.m_localSnapping
            ? localRotation.GetInverseFull().TransformVector(CalculateSnappedOffset(
                localRotation.TransformVector(localScale), axis, gridSize * scaleRecip))
            : AZ::Vector3::CreateZero();

        start.m_screenPosition = interaction.m_mousePick.m_screenCoordinates;
        start.m_positionSnapOffset = positionSnapOffset;
        start.m_scaleSnapOffset = scaleSnapOffset;
        start.m_localPosition = localTransform.GetTranslation() + positionSnapOffset;
        start.m_localScale = localScale + scaleSnapOffset;
        start.m_localAxis = axis;
        // sign to determine which side of the linear axis we pressed
        // (useful to know when the visual axis flips to face the camera)
        start.m_sign =
            AZ::GetSign((start.m_localHitPosition - localTransform.GetTranslation()).Dot(axis));

        startTransition.m_screenToWorldScale =
            1.0f / CalculateScreenToWorldMultiplier((worldFromLocal * localTransform).GetTranslation(), cameraState);

        return {startTransition, start};
    }

    LinearManipulator::Action CalculateLinearManipulationDataAction(
        const LinearManipulator::Fixed& fixed, const LinearManipulator::Starter& starter,
        const AZ::Transform& worldFromLocal, const AZ::Transform& localTransform, const GridSnapAction& gridSnapAction,
        const ViewportInteraction::MouseInteraction& interaction)
    {
        const ManipulatorInteraction manipulatorInteraction =
            BuildManipulatorInteraction(
                worldFromLocal, interaction.m_mousePick.m_rayOrigin, interaction.m_mousePick.m_rayDirection);

        const auto& [startTransition, start] = starter;

        // as CalculateRayPlaneIntersectingPoint may fail, ensure localHitPosition is initialized with
        // the starting hit position so the manipulator returns to the original location it was pressed
        // if an invalid ray intersection is attempted
        AZ::Vector3 localHitPosition = start.m_localHitPosition;
        Internal::CalculateRayPlaneIntersectingPoint(
            manipulatorInteraction.m_localRayOrigin, manipulatorInteraction.m_localRayDirection,
            start.m_localHitPosition, startTransition.m_localNormal, localHitPosition);

        localHitPosition = Internal::TryConstrainHitPositionToView(
            localHitPosition, start.m_localHitPosition, worldFromLocal.GetInverse(),
            GetCameraState(interaction.m_interactionId.m_viewportId));

        const AZ::Vector3 axis = TransformDirectionNoScaling(localTransform, fixed.m_axis);
        const AZ::Vector3 hitDelta = (localHitPosition - start.m_localHitPosition);
        const AZ::Vector3 unsnappedOffset = axis * axis.Dot(hitDelta);

        const float scaleRecip = manipulatorInteraction.m_scaleReciprocal;
        const float gridSize = gridSnapAction.m_gridSnapParams.m_gridSize;
        const bool snapping = gridSnapAction.m_gridSnapParams.m_gridSnap;

        LinearManipulator::Action action;
        action.m_fixed = fixed;
        action.m_start = start;
        action.m_current.m_localPositionOffset = snapping
            ? unsnappedOffset + CalculateSnappedOffset(unsnappedOffset, axis, gridSize * scaleRecip)
            : unsnappedOffset;
        action.m_current.m_screenPosition = interaction.m_mousePick.m_screenCoordinates;
        action.m_viewportId = interaction.m_interactionId.m_viewportId;

        const AZ::Quaternion localRotation = QuaternionFromTransformNoScaling(localTransform);
        const AZ::Vector3 scaledUnsnappedOffset = unsnappedOffset * startTransition.m_screenToWorldScale;
        // how much to adjust the scale based on movement
        const AZ::Quaternion invLocalRotation = localRotation.GetInverseFull();
        action.m_current.m_localScaleOffset = snapping
            ? invLocalRotation.TransformVector((scaledUnsnappedOffset + CalculateSnappedOffset(scaledUnsnappedOffset, axis, gridSize * scaleRecip)))
            : invLocalRotation.TransformVector(scaledUnsnappedOffset);

        // record what modifier keys are held during this action
        action.m_modifiers = interaction.m_keyboardModifiers;

        return action;
    }

    AZStd::shared_ptr<LinearManipulator> LinearManipulator::MakeShared(const AZ::Transform& worldFromLocal)
    {
        return AZStd::shared_ptr<LinearManipulator>(aznew LinearManipulator(worldFromLocal));
    }

    LinearManipulator::LinearManipulator(const AZ::Transform& worldFromLocal)
        : m_worldFromLocal(worldFromLocal)
    {
        AttachLeftMouseDownImpl();
    }

    void LinearManipulator::InstallLeftMouseDownCallback(const MouseActionCallback& onMouseDownCallback)
    {
        m_onLeftMouseDownCallback = onMouseDownCallback;
    }

    void LinearManipulator::InstallLeftMouseUpCallback(const MouseActionCallback& onMouseUpCallback)
    {
        m_onLeftMouseUpCallback = onMouseUpCallback;
    }

    void LinearManipulator::InstallMouseMoveCallback(const MouseActionCallback& onMouseMoveCallback)
    {
        m_onMouseMoveCallback = onMouseMoveCallback;
    }

    void LinearManipulator::OnLeftMouseDownImpl(
        const ViewportInteraction::MouseInteraction& interaction, const float rayIntersectionDistance)
    {
        const AZ::Transform worldFromLocalUniformScale = TransformUniformScale(m_worldFromLocal);
        const GridSnapParameters gridSnapParams = GridSnapSettings(interaction.m_interactionId.m_viewportId);

        // note: m_localTransform must not be made uniform as it may contain a local scale we want to snap
        m_starter = CalculateLinearManipulationDataStart(
            m_fixed, worldFromLocalUniformScale, m_localTransform,
            GridSnapAction(gridSnapParams, interaction.m_keyboardModifiers.Alt()), interaction, rayIntersectionDistance,
            GetCameraState(interaction.m_interactionId.m_viewportId));

        if (m_onLeftMouseDownCallback)
        {
            m_onLeftMouseDownCallback(CalculateLinearManipulationDataAction(
                m_fixed, m_starter, worldFromLocalUniformScale, m_localTransform,
                GridSnapAction(gridSnapParams, interaction.m_keyboardModifiers.Alt()), interaction));
        }
    }

    void LinearManipulator::OnMouseMoveImpl(const ViewportInteraction::MouseInteraction& interaction)
    {
        if (m_onMouseMoveCallback)
        {
            const GridSnapParameters gridSnapParams = GridSnapSettings(interaction.m_interactionId.m_viewportId);

            // note: m_localTransform must not be made uniform as it may contain a local scale we want to snap
            m_onMouseMoveCallback(CalculateLinearManipulationDataAction(
                m_fixed, m_starter, TransformUniformScale(m_worldFromLocal), m_localTransform,
                GridSnapAction(gridSnapParams, interaction.m_keyboardModifiers.Alt()), interaction));
        }
    }

    void LinearManipulator::OnLeftMouseUpImpl(const ViewportInteraction::MouseInteraction& interaction)
    {
        if (m_onLeftMouseUpCallback)
        {
            const GridSnapParameters gridSnapParams = GridSnapSettings(interaction.m_interactionId.m_viewportId);

            // note: m_localTransform must not be made uniform as it may contain a local scale we want to snap
            m_onLeftMouseUpCallback(CalculateLinearManipulationDataAction(
                m_fixed, m_starter, TransformUniformScale(m_worldFromLocal), m_localTransform,
                GridSnapAction(gridSnapParams, interaction.m_keyboardModifiers.Alt()), interaction));
        }
    }

    void LinearManipulator::Draw(
        const ManipulatorManagerState& managerState,
        AzFramework::DebugDisplayRequests& debugDisplay,
        const AzFramework::CameraState& cameraState,
        const ViewportInteraction::MouseInteraction& mouseInteraction)
    {
        const AZ::Transform localTransform = m_useVisualsOverride
            ? AZ::Transform::CreateFromQuaternionAndTranslation(
                m_visualOrientationOverride, m_localTransform.GetTranslation())
            : m_localTransform;

        if (cl_manipulatorDrawDebug)
        {
            if (PerformingAction())
            {
                const GridSnapParameters gridSnapParams =
                    GridSnapSettings(mouseInteraction.m_interactionId.m_viewportId);

                const auto action = CalculateLinearManipulationDataAction(
                    m_fixed, m_starter, TransformUniformScale(m_worldFromLocal), m_localTransform,
                    GridSnapAction(gridSnapParams, mouseInteraction.m_keyboardModifiers.Alt()), mouseInteraction);

                // display the exact hit (ray intersection) of the mouse pick on the manipulator
                DrawTransformAxes(
                    debugDisplay, TransformUniformScale(m_worldFromLocal) *
                    AZ::Transform::CreateTranslation(
                        action.m_start.m_localHitPosition + action.m_current.m_localPositionOffset));
            }

            const AZ::Transform combined = TransformUniformScale(m_worldFromLocal) * localTransform;

            DrawTransformAxes(debugDisplay, combined);
            DrawAxis(
                debugDisplay, combined.GetTranslation(), TransformDirectionNoScaling(combined, m_fixed.m_axis));
        }

        for (auto& view : m_manipulatorViews)
        {
            view->Draw(
                GetManipulatorManagerId(), managerState,
                GetManipulatorId(), {
                    m_worldFromLocal * localTransform,
                    AZ::Vector3::CreateZero(), MouseOver()
                },
                debugDisplay, cameraState, mouseInteraction);
        }
    }

    void LinearManipulator::SetAxis(const AZ::Vector3& axis)
    {
        m_fixed.m_axis = axis;
    }

    void LinearManipulator::SetSpace(const AZ::Transform& worldFromLocal)
    {
        m_worldFromLocal = worldFromLocal;
    }

    void LinearManipulator::SetLocalTransform(const AZ::Transform& localTransform)
    {
        m_localTransform = localTransform;
    }

    void LinearManipulator::SetLocalPosition(const AZ::Vector3& localPosition)
    {
        m_localTransform.SetTranslation(localPosition);
    }

    void LinearManipulator::SetLocalOrientation(const AZ::Quaternion& localOrientation)
    {
        m_localTransform = AZ::Transform::CreateFromQuaternionAndTranslation(
            localOrientation, m_localTransform.GetTranslation());
    }

    void LinearManipulator::InvalidateImpl()
    {
        for (auto& view : m_manipulatorViews)
        {
            view->Invalidate(GetManipulatorManagerId());
        }
    }

    void LinearManipulator::SetBoundsDirtyImpl()
    {
        for (auto& view : m_manipulatorViews)
        {
            view->SetBoundDirty(GetManipulatorManagerId());
        }
    }
} // namespace AzToolsFramework
