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

#include <AzFramework/Physics/ClassConverters.h>
#include <AzFramework/Physics/Collision/CollisionGroups.h>
#include <AzFramework/Physics/Collision/CollisionLayers.h>
#include <AzFramework/Physics/Material.h>
#include <AzFramework/Physics/Ragdoll.h>
#include <AzFramework/Physics/Shape.h>
#include <AzFramework/Physics/SystemBus.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzCore/std/utils.h>

namespace Physics
{
    namespace ClassConverters
    {
        bool RagdollNodeConfigConverter(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement)
        {
            if (classElement.GetVersion() < 2)
            {
                // this conversion is to deal with m_shapes in the RagdollNodeConfiguration changing from
                // AZStd::vector<AZStd::shared_ptr<ShapeConfiguration>> to
                // AZStd::vector<AZStd::pair<AZStd::shared_ptr<ColliderConfiguration>, AZStd::shared_ptr<ShapeConfiguration>>>,
                // and the collider related information from ShapeConfiguration moving to ColliderConfiguration
                const int shapesIndex = classElement.FindElement(AZ_CRC("shapes", 0x93dba512));
                if (shapesIndex != -1)
                {
                    AZ::SerializeContext::DataElementNode& shapesElement = classElement.GetSubElement(shapesIndex);

                    // copy the old shape config data before removing the original vector
                    AZStd::vector<AZ::SerializeContext::DataElementNode> shapesCopy;
                    const int numSubElements = shapesElement.GetNumSubElements();
                    shapesCopy.reserve(numSubElements);

                    for (int i = 0; i < numSubElements; i++)
                    {
                        AZ::SerializeContext::DataElementNode& sharedPtrElement = shapesElement.GetSubElement(i);

                        if (sharedPtrElement.GetNumSubElements() > 0)
                        {
                            AZ::SerializeContext::DataElementNode& shape = sharedPtrElement.GetSubElement(0);
                            shapesCopy.push_back(shape);
                        }
                    }

                    // remove the old vector
                    classElement.RemoveElement(shapesIndex);

                    // add a new vector in the new format
                    const int newShapesIndex = classElement.AddElement<ShapeConfigurationList>(context, "shapes");
                    if (newShapesIndex != -1)
                    {
                        AZ::SerializeContext::DataElementNode& newShapesElement = classElement.GetSubElement(newShapesIndex);

                        // convert the old shapes into the new format and add to the vector
                        for (AZ::SerializeContext::DataElementNode shape : shapesCopy)
                        {
                            const int pairIndex = newShapesElement.AddElementWithData<ShapeConfigurationPair>(context, "element", ShapeConfigurationPair());
                            AZ::SerializeContext::DataElementNode& pairElement = newShapesElement.GetSubElement(pairIndex);

                            ColliderConfiguration colliderConfig;
                            if (AZ::SerializeContext::DataElementNode* baseClassNode = shape.FindSubElement(AZ_CRC("BaseClass1", 0xd4925735)))
                            {
                                baseClassNode->FindSubElementAndGetData(AZ_CRC("Trigger", 0x1a6b0f5d), colliderConfig.m_isTrigger);
                                baseClassNode->FindSubElementAndGetData(AZ_CRC("Position", 0x462ce4f5), colliderConfig.m_position);
                                baseClassNode->FindSubElementAndGetData(AZ_CRC("Rotation", 0x297c98f1), colliderConfig.m_rotation);
                                baseClassNode->FindSubElementAndGetData(AZ_CRC("CollisionLayer", 0x39931633), colliderConfig.m_collisionLayer);
                            }

                            shape.RemoveElementByName(AZ_CRC("BaseClass1", 0xd4925735));

                            pairElement.GetSubElement(0).AddElementWithData<ColliderConfiguration>(context, "element", colliderConfig);
                            pairElement.GetSubElement(1).AddElement(shape);
                        }
                    }
                }
            }

            // Don't remove the 'shapes' element here, even though it got removed with v3. The version converter converts elements bottom-up
            // which means the ragdoll node configs get converted before the ragdoll config. If we remove the shapes element here, we would
            // not be able to pass the shapes over to the character collider config. The ragdoll config version converter takes care of this.
            //if (classElement.GetVersion() < 3)
            //{
            //    classElement.RemoveElementByName(AZ_CRC("shapes", 0x93dba512));
            //}

            // Version 4 adds visibility settings to hide rigid body settings that aren't relevant for the animation editor.
            if (classElement.GetVersion() < 4)
            {
                const int rigidBodyConfigIndex = classElement.FindElement(AZ_CRC("RigidBodyConfiguration", 0x152d8d79));
                if (rigidBodyConfigIndex != -1)
                {
                    AZ::SerializeContext::DataElementNode& rigidBodyConfigElement = classElement.GetSubElement(rigidBodyConfigIndex);
                    // in the animation editor we want to show inertia, damping, sleep, interpolation, gravity and CCD properties
                    // so the value should be (1 << 1) | (1 << 2) | (1 << 3) | (1 << 4) | (1 << 5) | (1 << 7) = 190
                    rigidBodyConfigElement.AddElementWithData<AZ::u16>(context, "Property Visibility Flags", 190);
                }
            }

            return true;
        }

        bool RagdollConfigConverter(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement)
        {
            if (classElement.GetVersion() < 3)
            {
                CharacterColliderConfiguration newColliderConfig;

                AZ::SerializeContext::DataElementNode* ragdollNodeConfig = classElement.FindSubElement(AZ_CRC("nodes", 0x1d3d05fc));
                if (ragdollNodeConfig)
                {
                    int numNodes = ragdollNodeConfig->GetNumSubElements();
                    for (int i = 0; i < numNodes; ++i)
                    {
                        AZ::SerializeContext::DataElementNode& nodeElement = ragdollNodeConfig->GetSubElement(i);

                        AZStd::string name;
                        AZ::SerializeContext::DataElementNode* baseClass1 = nodeElement.FindSubElement(AZ_CRC("BaseClass1", 0xd4925735));
                        if (baseClass1)
                        {
                            AZ::SerializeContext::DataElementNode* baseBaseClass1 = baseClass1->FindSubElement(AZ_CRC("BaseClass1", 0xd4925735));
                            if (baseBaseClass1 && baseBaseClass1->FindSubElementAndGetData<AZStd::string>(AZ_CRC("name", 0x5e237e06), name))
                            {
                                ShapeConfigurationList shapes;
                                if (nodeElement.FindSubElementAndGetData<ShapeConfigurationList>(AZ_CRC("shapes", 0x93dba512), shapes))
                                {
                                    CharacterColliderNodeConfiguration newColliderNodeConfig;
                                    newColliderNodeConfig.m_name = name;
                                    newColliderNodeConfig.m_shapes = shapes;
                                    newColliderConfig.m_nodes.push_back(newColliderNodeConfig);
                                }
                            }
                        }

                        nodeElement.RemoveElementByName(AZ_CRC("shapes", 0x93dba512));
                    }
                }

                classElement.AddElementWithData(context, "colliders", newColliderConfig);
            }

            return true;
        }

        bool MaterialLibraryAssetConverter(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement)
        {
            if (classElement.GetVersion() <= 1)
            {
                AZStd::vector<MaterialFromAssetConfiguration> newConfiguration;

                auto oldConfigurationsListDataNode = AZ::Utils::FindDescendantElements(context, classElement, { AZ_CRC("Properties", 0x87c331c7) });
                for (auto dataElement : oldConfigurationsListDataNode)
                {
                    int elementsCount = dataElement->GetNumSubElements();
                    for (int i = 0; i < elementsCount; ++i)
                    {
                        MaterialConfiguration oldConfiguration;

                        auto oldConfigurationDataNode = dataElement->GetSubElement(i);
                        if (!oldConfigurationDataNode.GetDataHierarchy(context, oldConfiguration))
                        {
                            return false;
                        }

                        MaterialId oldId;
                        if (auto oldIdNode = oldConfigurationDataNode.FindSubElement(AZ_CRC("UID", 0x539b0606)))
                        {
                            oldIdNode->GetData<MaterialId>(oldId);
                        }

                        MaterialFromAssetConfiguration configuration;
                        configuration.m_configuration = oldConfiguration;
                        configuration.m_id = oldId;

                        newConfiguration.push_back(configuration);
                    }
                }

                classElement.RemoveElementByName(AZ_CRC("Properties", 0x87c331c7));
                if (classElement.AddElementWithData(context, "Properties", newConfiguration) == -1)
                {
                    return false;
                }
            }

            return true;
        }

        bool ColliderConfigurationConverter(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& dataElement)
        {
            // version 1->2
            if (dataElement.GetVersion() <= 1)
            {
                // Convert collision group to group id
                dataElement.RemoveElementByName(AZ_CRC("CollisionGroup", 0xb08873ec));
                dataElement.AddElement<AzPhysics::CollisionGroups::Id>(context, "CollisionGroupId");
            }

            // version 2->3
            if (dataElement.GetVersion() <= 2)
            {
                // Force all new colliders to have exclusive shapes
                dataElement.RemoveElementByName(AZ_CRC("Exclusive", 0x012318fc));
                dataElement.AddElementWithData<bool>(context, "Exclusive", true);
            }

            // version 3->4
            if (dataElement.GetVersion() <= 3)
            {
                const int elementIndex = dataElement.FindElement(AZ_CRC("Trigger", 0x1a6b0f5d));

                if (elementIndex >= 0)
                {
                    bool isTrigger = false;
                    AZ::SerializeContext::DataElementNode& triggerElement = dataElement.GetSubElement(elementIndex);
                    const bool found = triggerElement.GetData<bool>(isTrigger);

                    if (found && isTrigger)
                    {
                        // Version 4 added "InSceneQueries" field set to true by default.
                        // The field is applicable to both trigger and simulated shapes.
                        // However before all trigger shapes were always invisible to scene queries.
                        // Setting "In Scene Queries" to false for all existing triggers to avoid breaking the existing content.
                        const int idx = dataElement.AddElement<bool>(context, "InSceneQueries");
                        if (idx != -1)
                        {
                            if (!dataElement.GetSubElement(idx).SetData<bool>(context, false))
                            {
                                return false;
                            }
                        }
                    }
                }
            }

            return true;
        }

        bool MaterialSelectionConverter(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& dataElement)
        {
            bool success = true;

            if (dataElement.GetVersion() <= 1)
            {
                Physics::MaterialId materialId;
                success = dataElement.FindSubElementAndGetData(AZ_CRC("MaterialId", 0x9360e002), materialId);

                if (success)
                {
                    success = success && dataElement.RemoveElementByName(AZ_CRC("MaterialId", 0x9360e002));
                    success = success && dataElement.AddElementWithData(context, "MaterialIds", AZStd::vector<Physics::MaterialId> { materialId });
                }
            }

            return success;
        }

        bool RigidBodyVersionConverter(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement)
        {
            if (classElement.GetVersion() <= 1)
            {
                const int elementIndex = classElement.FindElement(AZ_CRC("Centre of mass offset", 0x1e569a45));

                if (elementIndex >= 0)
                {
                    AZ::Vector3 existingCenterOfMassOffset;
                    AZ::SerializeContext::DataElementNode& centerOfMassElement = classElement.GetSubElement(elementIndex);
                    const bool found = centerOfMassElement.GetData<AZ::Vector3>(existingCenterOfMassOffset);

                    if (found && !existingCenterOfMassOffset.IsZero())
                    {
                        // An existing center of mass (COM) offset value was specified for this rigid body.
                        // Version 2 includes a new m_computeCenterOfMass boolean flag to specify the automatic calculation of COM.
                        // In this case set m_computeCenterOfMass to false so that the existing center of mass offset value is utilized correctly.
                        const int idx = classElement.AddElement<bool>(context, "Compute COM");
                        if (idx != -1)
                        {
                            if (!classElement.GetSubElement(idx).SetData<bool>(context, false))
                            {
                                return false;
                            }
                        }
                    }
                }
            }

            if (classElement.GetVersion() <= 2)
            {
                const int elementIndex = classElement.FindElement(AZ_CRC("Mass", 0x6c035b66));

                if (elementIndex >= 0)
                {
                    float existingMass = 0;
                    AZ::SerializeContext::DataElementNode& massElement = classElement.GetSubElement(elementIndex);
                    const bool found = massElement.GetData<float>(existingMass);

                    if (found && existingMass > 0)
                    {
                        // Keeping the existing mass and disabling auto-compute of the mass for this rigid body.
                        // Version 3 includes a new m_computeMass boolean flag to specify the automatic calculation of mass.
                        const int idx = classElement.AddElement<bool>(context, "Compute Mass");
                        if (idx != -1)
                        {
                            if (!classElement.GetSubElement(idx).SetData<bool>(context, false))
                            {
                                return false;
                            }
                        }
                    }
                }
            }

            return true;
        }
    } // namespace ClassConverters
} // namespace Physics
