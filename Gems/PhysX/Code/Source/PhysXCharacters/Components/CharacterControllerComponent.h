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
#include <AzFramework/Physics/CharacterBus.h>
#include <AzFramework/Physics/SystemBus.h>
#include <AzFramework/Physics/CollisionBus.h>
#include <AzFramework/Physics/WorldBodyBus.h>
#include <AzFramework/Physics/World.h>
#include <AzFramework/Physics/Common/PhysicsEvents.h>
#include <PhysXCharacters/API/CharacterController.h>
#include <AzCore/Component/TransformBus.h>
#include <PhysX/CharacterControllerBus.h>

namespace PhysX
{
    class CharacterControllerConfiguration;

    /// Component used to physically represent characters for basic interactions with the physical world, for example to
    /// prevent walking through walls or falling through terrain.
    class CharacterControllerComponent
        : public AZ::Component
        , public Physics::CharacterRequestBus::Handler
        , public Physics::WorldBodyRequestBus::Handler
        , public AZ::TransformNotificationBus::Handler
        , public CharacterControllerRequestBus::Handler
        , public Physics::CollisionFilteringRequestBus::Handler
    {
    public:
        AZ_COMPONENT(CharacterControllerComponent, "{BCBD8448-2FFC-450D-B82F-7C297D2F0C8C}");

        static void Reflect(AZ::ReflectContext* context);

        CharacterControllerComponent();
        CharacterControllerComponent(AZStd::unique_ptr<Physics::CharacterConfiguration> characterConfig,
            AZStd::unique_ptr<Physics::ShapeConfiguration> shapeConfig);
        ~CharacterControllerComponent();

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC("PhysicsWorldBodyService", 0x944da0cc));
            provided.push_back(AZ_CRC("PhysXCharacterControllerService", 0x428de4fa));
        }

        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
        {
            incompatible.push_back(AZ_CRC("PhysXCharacterControllerService", 0x428de4fa));
        }

        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
        {
            required.push_back(AZ_CRC("TransformService", 0x8ee22c50));
        }

        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
        {
            dependent.push_back(AZ_CRC("PhysXColliderService", 0x4ff43f7c));
        }

        Physics::CharacterConfiguration& GetCharacterConfiguration()
        {
            return *m_characterConfig;
        }

    protected:
        // AZ::Component
        void Init() override;
        void Activate() override;
        void Deactivate() override;

        // Physics::CharacterRequestBus
        AZ::Vector3 GetBasePosition() const override;
        void SetBasePosition(const AZ::Vector3& position) override;
        AZ::Vector3 GetCenterPosition() const override;
        float GetStepHeight() const override;
        void SetStepHeight(float stepHeight) override;
        AZ::Vector3 GetUpDirection() const override;
        void SetUpDirection(const AZ::Vector3& upDirection) override;
        float GetSlopeLimitDegrees() const override;
        void SetSlopeLimitDegrees(float slopeLimitDegrees) override;
        float GetMaximumSpeed() const override;
        void SetMaximumSpeed(float maximumSpeed) override;
        AZ::Vector3 GetVelocity() const override;
        void AddVelocity(const AZ::Vector3& velocity) override;
        bool IsPresent() const override { return IsPhysicsEnabled(); }
        Physics::Character* GetCharacter() override;

        // WorldBodyRequestBus
        void EnablePhysics() override;
        void DisablePhysics() override;
        bool IsPhysicsEnabled() const override;
        AZ::Aabb GetAabb() const override;
        Physics::WorldBody* GetWorldBody() override;
        Physics::RayCastHit RayCast(const Physics::RayCastRequest& request) override;

        // CharacterControllerRequestBus
        void Resize(float height) override;
        float GetHeight() override;
        void SetHeight(float height);
        float GetRadius() override;
        void SetRadius(float radius) override;
        float GetHalfSideExtent() override;
        void SetHalfSideExtent(float halfSideExtent) override;
        float GetHalfForwardExtent() override;
        void SetHalfForwardExtent(float halfForwardExtent) override;

        // TransformNotificationBus
        void OnTransformChanged(const AZ::Transform& local, const AZ::Transform& world) override;

        // CollisionFilteringRequestBus
        void SetCollisionLayer(const AZStd::string& layerName, AZ::Crc32 colliderTag) override;
        AZStd::string GetCollisionLayerName() override;
        void SetCollisionGroup(const AZStd::string& groupName, AZ::Crc32 colliderTag) override;
        AZStd::string GetCollisionGroupName() override;
        void ToggleCollisionLayer(const AZStd::string& layerName, AZ::Crc32 colliderTag, bool enabled) override;

    private:
        bool CreateController();
        void AttachColliders(Physics::Character& character);
        void OnPreSimulate(float deltaTime);

        AZStd::unique_ptr<Physics::CharacterConfiguration> m_characterConfig;
        AZStd::unique_ptr<Physics::ShapeConfiguration> m_shapeConfig;
        AZStd::unique_ptr<Physics::Character> m_controller;
        AzPhysics::SystemEvents::OnPresimulateEvent::Handler m_preSimulateHandler;
    };
} // namespace PhysX
