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

#include <PhysXCharacters/API/Ragdoll.h>
#include <AzFramework/Physics/CharacterPhysicsDataBus.h>
#include <AzFramework/Physics/WorldBodyBus.h>


namespace PhysX
{
    /// Component used to simulate a hierarchy of rigid bodies connected by joints, typically used for characters.
    class RagdollComponent
        : public AZ::Component
        , public AzFramework::RagdollPhysicsRequestBus::Handler
        , public Physics::WorldBodyRequestBus::Handler
        , public AzFramework::CharacterPhysicsDataNotificationBus::Handler
    {
    public:
        AZ_COMPONENT(RagdollComponent, "{B89498F8-4718-42FE-A457-A377DD0D61A0}");

        static void Reflect(AZ::ReflectContext* context);

        RagdollComponent() = default;
        ~RagdollComponent() override = default;

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC("PhysicsWorldBodyService", 0x944da0cc));
            provided.push_back(AZ_CRC("PhysXRagdollService", 0x6d889c70));
        }

        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
        {
            incompatible.push_back(AZ_CRC("PhysXRagdollService", 0x6d889c70));
            incompatible.push_back(AZ_CRC("LegacyCryPhysicsService", 0xbb370351));
        }

        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
        {
            required.push_back(AZ_CRC("TransformService", 0x8ee22c50));
        }

        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
        {
            dependent.push_back(AZ_CRC("PhysXColliderService", 0x4ff43f7c));
            dependent.push_back(AZ_CRC("CharacterPhysicsDataService", 0x34757927));
        }

    protected:
        // AZ::Component
        void Init() override;
        void Activate() override;
        void Deactivate() override;

        // RagdollPhysicsBus
        void EnableSimulation(const Physics::RagdollState& initialState) override;
        void EnableSimulationQueued(const Physics::RagdollState& initialState) override;
        void DisableSimulation() override;
        void DisableSimulationQueued() override;
        Physics::Ragdoll* GetRagdoll() override;
        void GetState(Physics::RagdollState& ragdollState) const override;
        void SetState(const Physics::RagdollState& ragdollState) override;
        void SetStateQueued(const Physics::RagdollState& ragdollState) override;
        void GetNodeState(size_t nodeIndex, Physics::RagdollNodeState& nodeState) const override;
        void SetNodeState(size_t nodeIndex, const Physics::RagdollNodeState& nodeState) override;
        Physics::RagdollNode* GetNode(size_t nodeIndex) const override;

        // WorldBodyRequestBus
        void EnablePhysics() override;
        void DisablePhysics() override;
        bool IsPhysicsEnabled() const override;
        AZ::Aabb GetAabb() const override;
        Physics::WorldBody* GetWorldBody() override;
        Physics::RayCastHit RayCast(const Physics::RayCastRequest& request) override;

        // CharacterPhysicsDataNotificationBus
        void OnRagdollConfigurationReady() override;

        // deprecated Cry functions
        void EnterRagdoll() override;
        void ExitRagdoll() override;

        // version converters
        static bool VersionConverter(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement);

    private:
        bool IsJointProjectionVisible();
        void Reinit();

        AZStd::unique_ptr<Ragdoll> m_ragdoll;
        /// Minimum number of position iterations to perform in the PhysX solver.
        /// Lower iteration counts are less expensive but may behave less realistically.
        AZ::u32 m_positionIterations = 16; 
        /// Minimum number of velocity iterations to perform in the PhysX solver.
        AZ::u32 m_velocityIterations = 8;
        /// Whether to use joint projection to preserve joint constraints in demanding
        /// situations at the expense of potentially reducing physical correctness.
        bool m_enableJointProjection = true; 
        /// Linear joint error above which projection will be applied.
        float m_jointProjectionLinearTolerance = 1e-3f;
        /// Angular joint error (in degrees) above which projection will be applied.
        float m_jointProjectionAngularToleranceDegrees = 1.0f;
    };
} // namespace PhysX
