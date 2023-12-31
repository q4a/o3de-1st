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

#include "PlanarManipulator.h"

#include <AzFramework/Entity/EntityDebugDisplayBus.h>
#include <AzToolsFramework/Manipulators/ManipulatorDebug.h>
#include <AzToolsFramework/Manipulators/ManipulatorSnapping.h>
#include <AzToolsFramework/Manipulators/ManipulatorView.h>
#include <AzToolsFramework/Maths/TransformUtils.h>
#include <AzToolsFramework/ViewportSelection/EditorSelectionUtil.h>

namespace AzToolsFramework
{
    PlanarManipulator::StartInternal PlanarManipulator::CalculateManipulationDataStart(
        const Fixed& fixed, const AZ::Transform& worldFromLocal, const AZ::Transform& localTransform,
        const GridSnapAction& gridSnapAction, const ViewportInteraction::MouseInteraction& interaction,
        const float intersectionDistance)
    {
        const ManipulatorInteraction manipulatorInteraction =
            BuildManipulatorInteraction(
                worldFromLocal, interaction.m_mousePick.m_rayOrigin, interaction.m_mousePick.m_rayDirection);

        const AZ::Vector3 normal = TransformDirectionNoScaling(localTransform, fixed.m_normal);
        const AZ::Vector3 axis1 = TransformDirectionNoScaling(localTransform, fixed.m_axis1);
        const AZ::Vector3 axis2 = TransformDirectionNoScaling(localTransform, fixed.m_axis2);

        // initial intersect point
        const AZ::Vector3 localIntersectionPoint =
            manipulatorInteraction.m_localRayOrigin + manipulatorInteraction.m_localRayDirection * intersectionDistance;

        StartInternal startInternal;
        Internal::CalculateRayPlaneIntersectingPoint(
            manipulatorInteraction.m_localRayOrigin, manipulatorInteraction.m_localRayDirection,
            localIntersectionPoint, normal, startInternal.m_localHitPosition);

        const float scaleRecip = manipulatorInteraction.m_scaleReciprocal;
        const float gridSize = gridSnapAction.m_gridSnapParams.m_gridSize;
        const bool snapping = gridSnapAction.m_gridSnapParams.m_gridSnap;

        // calculate amount to snap to align with grid
        const AZ::Vector3 snapOffset = snapping && !gridSnapAction.m_localSnapping
            ? CalculateSnappedOffset(localTransform.GetTranslation(), axis1, gridSize * scaleRecip) +
              CalculateSnappedOffset(localTransform.GetTranslation(), axis2, gridSize * scaleRecip)
            : AZ::Vector3::CreateZero();

        startInternal.m_snapOffset = snapOffset;
        startInternal.m_localPosition = localTransform.GetTranslation() + snapOffset;

        return startInternal;
    }

    PlanarManipulator::Action PlanarManipulator::CalculateManipulationDataAction(
        const Fixed& fixed, const StartInternal& startInternal, const AZ::Transform& worldFromLocal,
        const AZ::Transform& localTransform, const GridSnapAction& gridSnapAction,
        const ViewportInteraction::MouseInteraction& interaction)
    {
        const ManipulatorInteraction manipulatorInteraction =
            BuildManipulatorInteraction(
                worldFromLocal, interaction.m_mousePick.m_rayOrigin, interaction.m_mousePick.m_rayDirection);

        const AZ::Vector3 normal = TransformDirectionNoScaling(localTransform, fixed.m_normal);

        // as CalculateRayPlaneIntersectingPoint may fail, ensure localHitPosition is initialized with
        // the starting hit position so the manipulator returns to the original location it was pressed
        // if an invalid ray intersection is attempted
        AZ::Vector3 localHitPosition = startInternal.m_localHitPosition;
        Internal::CalculateRayPlaneIntersectingPoint(
            manipulatorInteraction.m_localRayOrigin, manipulatorInteraction.m_localRayDirection,
            startInternal.m_localHitPosition, normal, localHitPosition);

        localHitPosition = Internal::TryConstrainHitPositionToView(
            localHitPosition, startInternal.m_localHitPosition, worldFromLocal.GetInverse(),
            GetCameraState(interaction.m_interactionId.m_viewportId));

        const AZ::Vector3 axis1 = TransformDirectionNoScaling(localTransform, fixed.m_axis1);
        const AZ::Vector3 axis2 = TransformDirectionNoScaling(localTransform, fixed.m_axis2);

        const AZ::Vector3 hitDelta = (localHitPosition - startInternal.m_localHitPosition);
        const AZ::Vector3 unsnappedOffset = axis1.Dot(hitDelta) * axis1 + axis2.Dot(hitDelta) * axis2;

        const float scaleRecip = manipulatorInteraction.m_scaleReciprocal;
        const float gridSize = gridSnapAction.m_gridSnapParams.m_gridSize;
        const bool snapping = gridSnapAction.m_gridSnapParams.m_gridSnap;

        Action action;
        action.m_fixed = fixed;
        action.m_start.m_localPosition = startInternal.m_localPosition;
        action.m_start.m_snapOffset = startInternal.m_snapOffset;
        action.m_start.m_localHitPosition = startInternal.m_localHitPosition;
        action.m_current.m_localOffset = snapping
            ? unsnappedOffset +
                CalculateSnappedOffset(unsnappedOffset, axis1, gridSize * scaleRecip) +
                CalculateSnappedOffset(unsnappedOffset, axis2, gridSize * scaleRecip)
            : unsnappedOffset;

        // record what modifier keys are held during this action
        action.m_modifiers = interaction.m_keyboardModifiers;

        return action;
    }

    AZStd::shared_ptr<PlanarManipulator> PlanarManipulator::MakeShared(const AZ::Transform& worldFromLocal)
    {
        return AZStd::shared_ptr<PlanarManipulator>(aznew PlanarManipulator(worldFromLocal));
    }

    PlanarManipulator::PlanarManipulator(const AZ::Transform& worldFromLocal)
        : m_worldFromLocal(worldFromLocal)
    {
        AttachLeftMouseDownImpl();
    }

    void PlanarManipulator::InstallLeftMouseDownCallback(const MouseActionCallback& onMouseDownCallback)
    {
        m_onLeftMouseDownCallback = onMouseDownCallback;
    }

    void PlanarManipulator::InstallLeftMouseUpCallback(const MouseActionCallback& onMouseUpCallback)
    {
        m_onLeftMouseUpCallback = onMouseUpCallback;
    }

    void PlanarManipulator::InstallMouseMoveCallback(const MouseActionCallback& onMouseMoveCallback)
    {
        m_onMouseMoveCallback = onMouseMoveCallback;
    }

    void PlanarManipulator::OnLeftMouseDownImpl(
        const ViewportInteraction::MouseInteraction& interaction, const float rayIntersectionDistance)
    {
        const AZ::Transform worldFromLocalUniformScale = TransformUniformScale(m_worldFromLocal);

        const GridSnapParameters gridSnapParams = GridSnapSettings(interaction.m_interactionId.m_viewportId);

        m_startInternal = CalculateManipulationDataStart(
            m_fixed, worldFromLocalUniformScale, TransformNormalizedScale(m_localTransform),
            GridSnapAction(gridSnapParams, interaction.m_keyboardModifiers.Alt()),
            interaction, rayIntersectionDistance);

        if (m_onLeftMouseDownCallback)
        {
            m_onLeftMouseDownCallback(CalculateManipulationDataAction(
                m_fixed, m_startInternal, worldFromLocalUniformScale, TransformNormalizedScale(m_localTransform),
                GridSnapAction(gridSnapParams, interaction.m_keyboardModifiers.Alt()), interaction));
        }
    }

    void PlanarManipulator::OnMouseMoveImpl(const ViewportInteraction::MouseInteraction& interaction)
    {
        if (m_onMouseMoveCallback)
        {
            const GridSnapParameters gridSnapParams = GridSnapSettings(interaction.m_interactionId.m_viewportId);

            m_onMouseMoveCallback(CalculateManipulationDataAction(
                m_fixed, m_startInternal, TransformUniformScale(m_worldFromLocal),
                TransformNormalizedScale(m_localTransform),
                GridSnapAction(gridSnapParams, interaction.m_keyboardModifiers.Alt()), interaction));
        }
    }

    void PlanarManipulator::OnLeftMouseUpImpl(const ViewportInteraction::MouseInteraction& interaction)
    {
        if (m_onLeftMouseUpCallback)
        {
            const GridSnapParameters gridSnapParams = GridSnapSettings(interaction.m_interactionId.m_viewportId);

            m_onLeftMouseUpCallback(CalculateManipulationDataAction(
                m_fixed, m_startInternal, TransformUniformScale(m_worldFromLocal),
                TransformNormalizedScale(m_localTransform),
                GridSnapAction(gridSnapParams, interaction.m_keyboardModifiers.Alt()), interaction));
        }
    }

    void PlanarManipulator::Draw(
        const ManipulatorManagerState& managerState,
        AzFramework::DebugDisplayRequests& debugDisplay,
        const AzFramework::CameraState& cameraState,
        const ViewportInteraction::MouseInteraction& mouseInteraction)
    {
        if (cl_manipulatorDrawDebug)
        {
            if (PerformingAction())
            {
                const GridSnapParameters gridSnapParams = GridSnapSettings(mouseInteraction.m_interactionId.m_viewportId);
                const auto action = CalculateManipulationDataAction(
                    m_fixed, m_startInternal, TransformUniformScale(m_worldFromLocal),
                    TransformNormalizedScale(m_localTransform),
                    GridSnapAction(gridSnapParams, mouseInteraction.m_keyboardModifiers.Alt()), mouseInteraction);

                // display the exact hit (ray intersection) of the mouse pick on the manipulator
                DrawTransformAxes(
                    debugDisplay, TransformUniformScale(m_worldFromLocal) *
                    AZ::Transform::CreateTranslation(
                        action.m_start.m_localHitPosition + action.m_current.m_localOffset));
            }

            const AZ::Transform combined = m_worldFromLocal * m_localTransform;

            DrawTransformAxes(debugDisplay, combined);

            DrawAxis(
                debugDisplay, combined.GetTranslation(),
                TransformDirectionNoScaling(m_localTransform, m_fixed.m_axis1));
            DrawAxis(
                debugDisplay, combined.GetTranslation(),
                TransformDirectionNoScaling(m_localTransform, m_fixed.m_axis2));
        }

        for (auto& view : m_manipulatorViews)
        {
            view->Draw(
                GetManipulatorManagerId(), managerState,
                GetManipulatorId(), {
                    m_worldFromLocal * m_localTransform,
                    AZ::Vector3::CreateZero(), MouseOver()
                },
                debugDisplay, cameraState, mouseInteraction);
        }
    }

    void PlanarManipulator::SetAxes(const AZ::Vector3& axis1, const AZ::Vector3& axis2)
    {
        m_fixed.m_axis1 = axis1;
        m_fixed.m_axis2 = axis2;
        m_fixed.m_normal = axis1.Cross(axis2);
    }

    void PlanarManipulator::SetSpace(const AZ::Transform& worldFromLocal)
    {
        m_worldFromLocal = worldFromLocal;
    }

    void PlanarManipulator::SetLocalTransform(const AZ::Transform& localTransform)
    {
        m_localTransform = localTransform;
    }

    void PlanarManipulator::SetLocalPosition(const AZ::Vector3& localPosition)
    {
        m_localTransform.SetTranslation(localPosition);
    }

    void PlanarManipulator::SetLocalOrientation(const AZ::Quaternion& localOrientation)
    {
        m_localTransform = AZ::Transform::CreateFromQuaternionAndTranslation(
            localOrientation, m_localTransform.GetTranslation());
    }

    void PlanarManipulator::InvalidateImpl()
    {
        for (auto& view : m_manipulatorViews)
        {
            view->Invalidate(GetManipulatorManagerId());
        }
    }

    void PlanarManipulator::SetBoundsDirtyImpl()
    {
        for (auto& view : m_manipulatorViews)
        {
            view->SetBoundDirty(GetManipulatorManagerId());
        }
    }
} // namespace AzToolsFramework
