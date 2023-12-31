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

#include <PhysX_precompiled.h>
#include <PhysXCharacters/API/CharacterUtils.h>
#include <PhysXCharacters/API/CharacterController.h>
#include <PhysXCharacters/API/Ragdoll.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzFramework/Physics/SystemBus.h>
#include <World.h>
#include <cfloat>
#include <PhysX/PhysXLocks.h>
#include <Source/RigidBody.h>
#include <Source/Shape.h>
#include <Source/Joint.h>

namespace PhysX
{
    namespace Utils
    {
        namespace Characters
        {
            AZ::Outcome<size_t> GetNodeIndex(const Physics::RagdollConfiguration& configuration, const AZStd::string& nodeName)
            {
                const size_t numNodes = configuration.m_nodes.size();
                for (size_t nodeIndex = 0; nodeIndex < numNodes; nodeIndex++)
                {
                    if (configuration.m_nodes[nodeIndex].m_debugName == nodeName)
                    {
                        return AZ::Success(nodeIndex);
                    }
                }

                return AZ::Failure();
            }

            /// Adds the properties that exist in both the PhysX capsule and box controllers to the controller description.
            /// @param[in,out] controllerDesc The controller description to which the shape independent properties should be added.
            /// @param characterConfig Information about the character required for initialization.
            static void AppendShapeIndependentProperties(physx::PxControllerDesc& controllerDesc,
                const Physics::CharacterConfiguration& characterConfig, CharacterControllerCallbackManager* callbackManager)
            {
                AZStd::vector<AZStd::shared_ptr<Physics::Material> > materials;

                Physics::SystemRequestBus::BroadcastResult(
                    materials,
                    &Physics::SystemRequests::CreateMaterialsFromLibrary,
                    characterConfig.m_materialSelection
                );

                if (materials.empty())
                {
                    AZ_Error("PhysX Character Controller", false, "Could not create character controller, material was invalid.");
                    return;
                }

                physx::PxMaterial* pxMaterial = static_cast<physx::PxMaterial*>(materials.front()->GetNativePointer());

                controllerDesc.material = pxMaterial;
                controllerDesc.slopeLimit = cosf(AZ::DegToRad(characterConfig.m_maximumSlopeAngle));
                controllerDesc.stepOffset = characterConfig.m_stepHeight;
                controllerDesc.upDirection = characterConfig.m_upDirection.IsZero()
                    ? physx::PxVec3(0.0f, 0.0f, 1.0f)
                    : PxMathConvert(characterConfig.m_upDirection).getNormalized();
                controllerDesc.userData = nullptr;
                controllerDesc.behaviorCallback = callbackManager;
                controllerDesc.reportCallback = callbackManager;
            }

            /// Adds the properties which are PhysX specific and not included in the base generic character configuration.
            /// @param[in,out] controllerDesc The controller description to which the PhysX specific properties should be added.
            /// @param characterConfig Information about the character required for initialization.
            void AppendPhysXSpecificProperties(physx::PxControllerDesc& controllerDesc,
                const Physics::CharacterConfiguration& characterConfig)
            {
                if (characterConfig.RTTI_GetType() == CharacterControllerConfiguration::RTTI_Type())
                {
                    const auto& extendedConfig = static_cast<const CharacterControllerConfiguration&>(characterConfig);

                    controllerDesc.scaleCoeff = extendedConfig.m_scaleCoefficient;
                    controllerDesc.contactOffset = extendedConfig.m_contactOffset;
                    controllerDesc.nonWalkableMode = extendedConfig.m_slopeBehaviour == SlopeBehaviour::PreventClimbing
                        ? physx::PxControllerNonWalkableMode::ePREVENT_CLIMBING
                        : physx::PxControllerNonWalkableMode::ePREVENT_CLIMBING_AND_FORCE_SLIDING;
                }
            }

            AZStd::unique_ptr<CharacterController> CreateCharacterController(const Physics::CharacterConfiguration&
                characterConfig, const Physics::ShapeConfiguration& shapeConfig, Physics::World& world)
            {
                physx::PxControllerManager* manager = nullptr;
                if (auto physxWorld = azrtti_cast<PhysX::World*>(&world))
                {
                    manager = physxWorld->GetOrCreateControllerManager();
                }
                if (!manager)
                {
                    AZ_Error("PhysX Character Controller", false, "Could not retrieve character controller manager.");
                    return nullptr;
                }

                auto callbackManager = AZStd::make_unique<CharacterControllerCallbackManager>();

                physx::PxController* pxController = nullptr;

                if (shapeConfig.GetShapeType() == Physics::ShapeType::Capsule)
                {
                    physx::PxCapsuleControllerDesc capsuleDesc;

                    const Physics::CapsuleShapeConfiguration& capsuleConfig = static_cast<const Physics::CapsuleShapeConfiguration&>(shapeConfig);
                    // LY height means total height, PhysX means height of straight section
                    capsuleDesc.height = AZ::GetMax(epsilon, capsuleConfig.m_height - 2.0f * capsuleConfig.m_radius);
                    capsuleDesc.radius = capsuleConfig.m_radius;
                    capsuleDesc.climbingMode = physx::PxCapsuleClimbingMode::eCONSTRAINED;

                    AppendShapeIndependentProperties(capsuleDesc, characterConfig, callbackManager.get());
                    AppendPhysXSpecificProperties(capsuleDesc, characterConfig);
                    PHYSX_SCENE_WRITE_LOCK(static_cast<physx::PxScene*>(world.GetNativePointer()));
                    pxController = manager->createController(capsuleDesc);
                }

                else if (shapeConfig.GetShapeType() == Physics::ShapeType::Box)
                {
                    physx::PxBoxControllerDesc boxDesc;

                    const Physics::BoxShapeConfiguration& boxConfig = static_cast<const Physics::BoxShapeConfiguration&>(shapeConfig);
                    boxDesc.halfHeight = 0.5f * boxConfig.m_dimensions.GetZ();
                    boxDesc.halfSideExtent = 0.5f * boxConfig.m_dimensions.GetY();
                    boxDesc.halfForwardExtent = 0.5f * boxConfig.m_dimensions.GetX();

                    AppendShapeIndependentProperties(boxDesc, characterConfig, callbackManager.get());
                    AppendPhysXSpecificProperties(boxDesc, characterConfig);
                    PHYSX_SCENE_WRITE_LOCK(static_cast<physx::PxScene*>(world.GetNativePointer()));
                    pxController = manager->createController(boxDesc);
                }

                else
                {
                    AZ_Error("PhysX Character Controller", false, "PhysX only supports box and capsule shapes for character controllers.");
                    return nullptr;
                }

                if (!pxController)
                {
                    AZ_Error("PhysX Character Controller", false, "Failed to create character controller.");
                    return nullptr;
                }

                auto controller = AZStd::make_unique<CharacterController>(pxController, AZStd::move(callbackManager));
                controller->SetFilterDataAndShape(characterConfig);
                controller->SetUserData(characterConfig);
                controller->SetActorName(characterConfig.m_debugName);
                controller->SetMinimumMovementDistance(characterConfig.m_minimumMovementDistance);
                controller->SetMaximumSpeed(characterConfig.m_maximumSpeed);
                controller->CreateShadowBody(characterConfig, world);
                controller->SetTag(characterConfig.m_colliderTag);

                return controller;
            }

            AZStd::unique_ptr<Ragdoll> CreateRagdoll(const Physics::RagdollConfiguration& configuration,
                const Physics::RagdollState& initialState, const ParentIndices& parentIndices)
            {
                const size_t numNodes = configuration.m_nodes.size();
                if (numNodes != initialState.size())
                {
                    AZ_Error("PhysX Ragdoll", false, "Mismatch between number of nodes in ragdoll configuration (%i) "
                        "and number of nodes in the initial ragdoll state (%i)", numNodes, initialState.size());
                    return nullptr;
                }

                AZStd::unique_ptr<Ragdoll> ragdoll = AZStd::make_unique<Ragdoll>();
                ragdoll->SetParentIndices(parentIndices);

                // Set up rigid bodies
                for (size_t nodeIndex = 0; nodeIndex < numNodes; nodeIndex++)
                {
                    const Physics::RagdollNodeConfiguration& nodeConfig = configuration.m_nodes[nodeIndex];
                    const Physics::RagdollNodeState& nodeState = initialState[nodeIndex];
                    physx::PxTransform transform(PxMathConvert(nodeState.m_position), PxMathConvert(nodeState.m_orientation));

                    AZStd::unique_ptr<Physics::RigidBody> rigidBody = AZStd::make_unique<RigidBody>(nodeConfig);

                    physx::PxRigidDynamic* pxRigidDynamic = static_cast<physx::PxRigidDynamic*>(rigidBody->GetNativePointer());
                    pxRigidDynamic->setGlobalPose(transform);

                    Physics::CharacterColliderNodeConfiguration* colliderNodeConfig = configuration.m_colliders.FindNodeConfigByName(nodeConfig.m_debugName);
                    if (colliderNodeConfig)
                    {
                        for (const auto& shapeConfig : colliderNodeConfig->m_shapes)
                        {
                            AZStd::shared_ptr<Physics::Shape> shape = AZStd::make_shared<Shape>(*shapeConfig.first, *shapeConfig.second);
                            if (shape)
                            {
                                rigidBody->AddShape(shape);
                            }
                            else
                            {
                                AZ_Error("PhysX Ragdoll", false, "Failed to create collider shape for ragdoll node %s", nodeConfig.m_debugName.c_str());
                            }
                        }
                    }

                    AZStd::unique_ptr<RagdollNode> node = AZStd::make_unique<RagdollNode>(AZStd::move(rigidBody));
                    ragdoll->AddNode(AZStd::move(node));
                }

                // Set up joints.  Needs a second pass because child nodes in the ragdoll config aren't guaranteed to have
                // larger indices than their parents.
                size_t rootIndex = SIZE_MAX;
                for (size_t nodeIndex = 0; nodeIndex < numNodes; nodeIndex++)
                {
                    size_t parentIndex = parentIndices[nodeIndex];
                    if (parentIndex < numNodes)
                    {
                        physx::PxRigidDynamic* parentActor = ragdoll->GetPxRigidDynamic(parentIndex);
                        physx::PxRigidDynamic* childActor = ragdoll->GetPxRigidDynamic(nodeIndex);
                        if (parentActor && childActor)
                        {
                            physx::PxVec3 parentOffset = parentActor->getGlobalPose().q.rotateInv(
                                childActor->getGlobalPose().p - parentActor->getGlobalPose().p);
                            physx::PxTransform parentTM(parentOffset);
                            physx::PxTransform childTM(physx::PxIdentity);

                            AZStd::shared_ptr<Physics::JointLimitConfiguration> jointLimitConfig = configuration.m_nodes[nodeIndex].m_jointLimit;
                            if (!jointLimitConfig)
                            {
                                AZStd::vector<AZ::TypeId> supportedJointLimitTypes = JointUtils::GetSupportedJointTypes();

                                if (!supportedJointLimitTypes.empty())
                                {
                                    jointLimitConfig = JointUtils::CreateJointLimitConfiguration(supportedJointLimitTypes[0]);
                                }
                            }

                            AZStd::shared_ptr<Physics::Joint> joint = JointUtils::CreateJoint(
                                jointLimitConfig,
                                &ragdoll->GetNode(parentIndex)->GetRigidBody(),
                                &ragdoll->GetNode(nodeIndex)->GetRigidBody());

                            // Moving from PhysX 3.4 to 4.1, the allowed range of the twist angle was expanded from -pi..pi
                            // to -2*pi..2*pi.
                            // In 3.4, twist angles which were outside the range were wrapped into it, which means that it
                            // would be possible for a joint to have been authored under 3.4 which would be inside its twist
                            // limit in 3.4 but violating the limit by up to 2*pi in 4.1.
                            // If this case is detected, flipping the sign of one of the joint local pose quaternions will
                            // ensure that the twist angle will have a value which would not lead to wrapping.
                            auto* jointNativePointer = static_cast<physx::PxJoint*>(joint->GetNativePointer());
                            if (jointNativePointer && jointNativePointer->getConcreteType() == physx::PxJointConcreteType::eD6)
                            {
                                auto* d6Joint = static_cast<physx::PxD6Joint*>(jointNativePointer);
                                const float twist = d6Joint->getTwistAngle();
                                const physx::PxJointAngularLimitPair twistLimit = d6Joint->getTwistLimit();
                                if (twist < twistLimit.lower || twist > twistLimit.upper)
                                {
                                    physx::PxTransform childLocalTransform = d6Joint->getLocalPose(physx::PxJointActorIndex::eACTOR1);
                                    childLocalTransform.q = -childLocalTransform.q;
                                    d6Joint->setLocalPose(physx::PxJointActorIndex::eACTOR1, childLocalTransform);
                                }
                            }

                            Physics::RagdollNode* childNode = ragdoll->GetNode(nodeIndex);
                            static_cast<RagdollNode*>(childNode)->SetJoint(joint);
                        }
                        else
                        {
                            AZ_Error("PhysX Ragdoll", false, "Failed to create joint for node index %i.", nodeIndex);
                        }
                    }
                    else
                    {
                        // If the configuration only has one root and is valid, the node without a parent must be the root.
                        rootIndex = nodeIndex;
                    }
                }

                ragdoll->SetRootIndex(rootIndex);

                return ragdoll;
            }

            physx::PxD6JointDrive CreateD6JointDrive(float stiffness, float dampingRatio, float forceLimit)
            {
                if (!(std::isfinite)(stiffness) || stiffness < 0.0f)
                {
                    AZ_Warning("PhysX Character Utils", false, "Invalid joint stiffness, using 0.0f instead.");
                    stiffness = 0.0f;
                }

                if (!(std::isfinite)(dampingRatio) || dampingRatio < 0.0f)
                {
                    AZ_Warning("PhysX Character Utils", false, "Invalid joint damping ratio, using 1.0f instead.");
                    dampingRatio = 1.0f;
                }

                if (!(std::isfinite)(forceLimit))
                {
                    AZ_Warning("PhysX Character Utils", false, "Invalid joint force limit, ignoring.");
                    forceLimit = std::numeric_limits<float>::max();
                }

                float damping = dampingRatio * 2.0f * sqrtf(stiffness);
                bool isAcceleration = true;
                return physx::PxD6JointDrive(stiffness, damping, forceLimit, isAcceleration);
            }
        } // namespace Characters
    } // namespace Utils
} // namespace PhysX
