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

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Component/Component.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/Interface/Interface.h>

#include <AzFramework/Physics/RagdollPhysicsBus.h>
#include <AzFramework/Physics/CharacterPhysicsDataBus.h>
#include <AzFramework/Physics/World.h>

#include <Integration/Assets/ActorAsset.h>
#include <Integration/ActorComponentBus.h>
#include <Integration/Rendering/RenderActorInstance.h>

#include <LmbrCentral/Animation/AttachmentComponentBus.h>

#include <AzFramework/API/AtomActiveInterface.h>

namespace LmbrCentral
{
    class MaterialOwnerRequestBusHandlerImpl;
}

namespace EMotionFX
{
    namespace Integration
    {
        class ActorComponent
            : public AZ::Component
            , private AZ::Data::AssetBus::Handler
            , private AZ::TransformNotificationBus::MultiHandler
            , private AZ::TickBus::Handler
            , private ActorComponentRequestBus::Handler
            , private ActorComponentNotificationBus::Handler
            , private LmbrCentral::AttachmentComponentNotificationBus::Handler
            , private AzFramework::CharacterPhysicsDataRequestBus::Handler
            , private AzFramework::RagdollPhysicsNotificationBus::Handler
            , protected Physics::WorldNotificationBus::Handler
        {
        public:
            AZ_COMPONENT(ActorComponent, "{BDC97E7F-A054-448B-A26F-EA2B5D78E377}");
            friend class EditorActorComponent;

            /**
            * Configuration struct for procedural configuration of Actor Components.
            */
            struct Configuration
            {
                AZ_TYPE_INFO(Configuration, "{053BFBC0-ABAA-4F4E-911F-5320F941E1A8}")

                AZ::Data::Asset<ActorAsset> m_actorAsset{AZ::Data::AssetLoadBehavior::NoLoad}; ///< Selected actor asset.
                ActorAsset::MaterialList m_materialPerLOD{}; ///< Material assignment per LOD.
                AZ::EntityId m_attachmentTarget{}; ///< Target entity this actor should attach to.
                AZ::u32 m_attachmentJointIndex = MCORE_INVALIDINDEX32; ///< Index of joint on target skeleton for actor attachments.
                AttachmentType m_attachmentType = AttachmentType::None; ///< Type of attachment.
                bool m_renderSkeleton = false; ///< Toggles debug rendering of the skeleton.
                bool m_renderCharacter = true; ///< Toggles rendering of the character.
                bool m_renderBounds = false; ///< Toggles rendering of the character bounds used for visibility testing.
                SkinningMethod m_skinningMethod = SkinningMethod::DualQuat; ///< The skinning method for this actor
                AZ::u32 m_lodLevel = 0;

                // Force updating the joints when it is out of camera view. By
                // default, joints level update (beside the root joint) on
                // actor are disabled when the actor is out of view. 
                bool m_forceUpdateJointsOOV = false;

                static void Reflect(AZ::ReflectContext* context);
            };

            ActorComponent(const Configuration* configuration = nullptr);
            ~ActorComponent() override;

            //////////////////////////////////////////////////////////////////////////
            // AZ::Component interface implementation
            void Activate() override;
            void Deactivate() override;

            //////////////////////////////////////////////////////////////////////////
            // ActorComponentRequestBus::Handler
            size_t GetNumJoints() const override;
            size_t GetJointIndexByName(const char* name) const override;
            AZ::Transform GetJointTransform(size_t jointIndex, Space space) const override;
            void GetJointTransformComponents(size_t jointIndex, Space space, AZ::Vector3& outPosition, AZ::Quaternion& outRotation, AZ::Vector3& outScale) const override;
            Physics::AnimationConfiguration* GetPhysicsConfig() const override;

            ActorInstance* GetActorInstance() override { return m_actorInstance.get(); }
            void AttachToEntity(AZ::EntityId targetEntityId, AttachmentType attachmentType) override;
            void DetachFromEntity() override;
            void DebugDrawRoot(bool enable) override;
            bool GetRenderCharacter() const override;
            void SetRenderCharacter(bool enable) override;
            SkinningMethod GetSkinningMethod() const override;

            //////////////////////////////////////////////////////////////////////////
            // ActorComponentNotificationBus::Handler
            void OnActorInstanceCreated(ActorInstance* actorInstance) override;
            void OnActorInstanceDestroyed(ActorInstance* actorInstance) override;
            //////////////////////////////////////////////////////////////////////////

            // The entity has attached to the target.
            void OnAttached(AZ::EntityId targetId) override;

            // The entity is detaching from the target.
            void OnDetached(AZ::EntityId targetId) override;

            //////////////////////////////////////////////////////////////////////////
            // AzFramework::CharacterPhysicsDataBus::Handler
            bool GetRagdollConfiguration(Physics::RagdollConfiguration& config) const override;
            Physics::RagdollState GetBindPose(const Physics::RagdollConfiguration& config) const override;
            AZStd::string GetParentNodeName(const AZStd::string& childName) const override;

            //////////////////////////////////////////////////////////////////////////
            // AzFramework::RagdollPhysicsNotificationBus::Handler
            void OnRagdollActivated() override;
            void OnRagdollDeactivated() override;

            //////////////////////////////////////////////////////////////////////////
            static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
            {
                provided.push_back(AZ_CRC("EMotionFXActorService", 0xd6e8f48d));
                provided.push_back(AZ_CRC("MeshService", 0x71d8a455));
                provided.push_back(AZ_CRC("CharacterPhysicsDataService", 0x34757927));

                if (AZ::Interface<AzFramework::AtomActiveInterface>::Get())
                {
                    provided.push_back(AZ_CRC("MaterialReceiverService", 0x0d1a6a74));
                }
            }

            static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
            {
                incompatible.push_back(AZ_CRC("EMotionFXActorService", 0xd6e8f48d));
                incompatible.push_back(AZ_CRC("MeshService", 0x71d8a455));
            }

            static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
            {
                dependent.push_back(AZ_CRC("PhysicsService", 0xa7350d22));
            }

            static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
            {
                required.push_back(AZ_CRC("TransformService", 0x8ee22c50));
            }

            static void Reflect(AZ::ReflectContext* context);

            // AZ::Data::AssetBus::Handler
            void OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset) override;
            void OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData> asset) override;

            // Physics::WorldNotificationBus::Handler
            bool IsWorldNotificationBusConnected(AZ::Crc32 worldId) const;

        private:
            // AZ::TransformNotificationBus::MultiHandler
            void OnTransformChanged(const AZ::Transform& local, const AZ::Transform& world) override;

            // AZ::TickBus::Handler
            void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;
            int GetTickOrder() override;

            // Physics::WorldNotifications::Handler
            void OnPostPhysicsSubtick(float fixedDeltaTime) override;
            int GetPhysicsTickOrder() override;

            void CheckActorCreation();
            void DestroyActor();
            void CheckAttachToEntity();

            Configuration                                   m_configuration;            ///< Component configuration.
                                                                                        /// Live state
            ActorAsset::ActorInstancePtr                    m_attachmentTargetActor;    ///< Target actor instance to attach to.
            AZ::EntityId                                    m_attachmentTargetEntityId; ///< Target actor entity ID
            ActorAsset::ActorInstancePtr                    m_actorInstance;            ///< Live actor instance.
            AZStd::vector<AZ::EntityId>                     m_attachments;

            AZStd::unique_ptr<RenderActorInstance>          m_renderActorInstance;
            bool                                            m_debugDrawRoot;            ///< Enables drawing of actor root and facing.
        };
    } //namespace Integration
} // namespace EMotionFX
