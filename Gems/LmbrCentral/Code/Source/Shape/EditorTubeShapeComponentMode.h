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

#include <AzCore/Component/TransformBus.h>
#include <AzToolsFramework/ComponentMode/EditorBaseComponentMode.h>
#include <LmbrCentral/Shape/ShapeComponentBus.h>
#include <LmbrCentral/Shape/EditorSplineComponentBus.h>
#include <LmbrCentral/Shape/SplineComponentBus.h>

namespace AzToolsFramework
{
    class LinearManipulator;
}

namespace LmbrCentral
{
    /// ComponentMode designed for providing Viewport Editing of TubeShape.
    class EditorTubeShapeComponentMode
        : public AzToolsFramework::ComponentModeFramework::EditorBaseComponentMode
        , private AZ::TransformNotificationBus::Handler
        , private ShapeComponentNotificationsBus::Handler
        , private SplineComponentNotificationBus::Handler
        , private EditorSplineComponentNotificationBus::Handler
    {
    public:
        AZ_CLASS_ALLOCATOR_DECL

        EditorTubeShapeComponentMode(
            const AZ::EntityComponentIdPair& entityComponentIdPair, AZ::Uuid componentType);
        ~EditorTubeShapeComponentMode();

    private:
        // EditorBaseComponentMode
        void Refresh() override;
        AZStd::vector<AzToolsFramework::ActionOverride> PopulateActionsImpl() override;

        // Manipulator handling
        void CreateManipulators();
        void DestroyManipulators();
        void ContainerChanged();

        // SplineComponentNotificationBus
        void OnSplineChanged() override;
        void OnVertexAdded(size_t index) override;
        void OnVertexRemoved(size_t index) override;
        void OnVerticesSet(const AZStd::vector<AZ::Vector3>& vertices) override;
        void OnVerticesCleared() override;
        void OnOpenCloseChanged(bool closed) override;

        // ShapeComponentNotificationsBus
        void OnShapeChanged(ShapeChangeReasons changeReason) override;

        // TransformNotificationBus
        void OnTransformChanged(const AZ::Transform& local, const AZ::Transform& world) override;

        // EditorSplineComponentNotificationBus
        void OnSplineTypeChanged() override;

        void RefreshManipulatorsLocal(AZ::EntityId entityId);

        /// Data required per TubeShape manipulator.
        struct TubeManipulatorState
        {
            AZ::SplineAddress m_splineAddress;
            AZ::u64 m_vertIndex;
        };

        /// For a given Tube + Spline combo, generate data required for each manipulator at
        /// each vertex required for modifying the tube.
        AZStd::vector<TubeManipulatorState> GenerateTubeManipulatorStates(const AZ::Spline& spline);

        AZ::Transform m_currentTransform; ///< The current localToWorld transform of the TubeShape.
        AZStd::vector<AZStd::shared_ptr<AzToolsFramework::LinearManipulator>> m_radiusManipulators; ///< Manipulators to control the radius (volume) of the tube at each vertex.
    };
} // namespace LmbrCentral
