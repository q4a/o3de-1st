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
#include <Editor/DebugDraw.h>
#include <Editor/ConfigurationWindowBus.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/Interface/Interface.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <LyViewPaneNames.h>
#include <LmbrCentral/Geometry/GeometrySystemComponentBus.h>
#include <Source/Utils.h>

#include <PhysX/Configuration/PhysXConfiguration.h>
#include <System/PhysXSystem.h>

namespace PhysX
{
    namespace DebugDraw
    {
        const AZ::u32 TrianglesWarningThreshold = 16384 * 3;
        const AZ::u32 MaxTrianglesRange = 800;
        const AZ::Color WarningColor(1.0f, 0.0f, 0.0f, 1.0f);
        const float WarningFrequency = 1.0f; // the number of times per second to flash

        const AZ::Color WireframeColor(0.0f, 0.0f, 0.0f, 0.7f);
        const float ColliderLineWidth = 2.0f;

        void OpenPhysXSettingsWindow()
        {
            // Open configuration window
            AzToolsFramework::EditorRequestBus::Broadcast(&AzToolsFramework::EditorRequests::OpenViewPane,
                LyViewPane::PhysXConfigurationEditor);

            // Set to Global Settings configuration tab
            Editor::ConfigurationWindowRequestBus::Broadcast(&Editor::ConfigurationWindowRequests::ShowGlobalSettingsTab);
        }

        bool IsGlobalColliderDebugCheck(Debug::DebugDisplayData::GlobalCollisionDebugState requiredState)
        {
            if (auto* physXDebug = AZ::Interface<Debug::PhysXDebugInterface>::Get())
            {
                return physXDebug->GetDebugDisplayData().m_globalCollisionDebugDraw == requiredState;
            }
            return false;
        }

        static void BuildAABBVerts(const AZ::Aabb& aabb,
            AZStd::vector<AZ::Vector3>& verts,
            AZStd::vector<AZ::Vector3>& points,
            AZStd::vector<AZ::u32>& indices)
        {
            struct Triangle
            {
                AZ::Vector3 m_points[3];

                Triangle(const AZ::Vector3& point0, const AZ::Vector3& point1, const AZ::Vector3& point2)
                    : m_points{point0, point1, point2}
                {}
            };

            const AZ::Vector3 aabbMin = aabb.GetMin();
            const AZ::Vector3 aabbMax = aabb.GetMax();

            const float x[2] = { aabbMin.GetX(), aabbMax.GetX() };
            const float y[2] = { aabbMin.GetY(), aabbMax.GetY() };
            const float z[2] = { aabbMin.GetZ(), aabbMax.GetZ() };

            // There are 12 triangles in an AABB
            const AZStd::vector<Triangle> triangles =
            {
                // Bottom
                {AZ::Vector3(x[0], y[0], z[0]), AZ::Vector3(x[1], y[1], z[0]), AZ::Vector3(x[1], y[0], z[0])},
                {AZ::Vector3(x[0], y[0], z[0]), AZ::Vector3(x[0], y[1], z[0]), AZ::Vector3(x[1], y[1], z[0])},
                // Top
                {AZ::Vector3(x[0], y[0], z[1]), AZ::Vector3(x[1], y[0], z[1]), AZ::Vector3(x[1], y[1], z[1])},
                {AZ::Vector3(x[0], y[0], z[1]), AZ::Vector3(x[1], y[1], z[1]), AZ::Vector3(x[0], y[1], z[1])},
                // Near
                {AZ::Vector3(x[0], y[0], z[0]), AZ::Vector3(x[1], y[0], z[1]), AZ::Vector3(x[1], y[0], z[1])},
                {AZ::Vector3(x[0], y[0], z[0]), AZ::Vector3(x[1], y[0], z[1]), AZ::Vector3(x[0], y[0], z[1])},
                // Far
                {AZ::Vector3(x[0], y[1], z[0]), AZ::Vector3(x[1], y[1], z[1]), AZ::Vector3(x[0], y[1], z[1])},
                {AZ::Vector3(x[0], y[1], z[0]), AZ::Vector3(x[1], y[1], z[0]), AZ::Vector3(x[1], y[1], z[1])},
                // Left
                {AZ::Vector3(x[0], y[1], z[0]), AZ::Vector3(x[0], y[0], z[1]), AZ::Vector3(x[0], y[1], z[1])},
                {AZ::Vector3(x[0], y[1], z[0]), AZ::Vector3(x[0], y[0], z[0]), AZ::Vector3(x[0], y[0], z[1])},
                // Right
                {AZ::Vector3(x[1], y[0], z[0]), AZ::Vector3(x[1], y[1], z[0]), AZ::Vector3(x[1], y[1], z[1])},
                {AZ::Vector3(x[1], y[0], z[0]), AZ::Vector3(x[1], y[1], z[1]), AZ::Vector3(x[1], y[0], z[1])}
            };

            int index = static_cast<int>(verts.size());

            verts.reserve(verts.size() + triangles.size() * 3);
            indices.reserve(indices.size() + triangles.size() * 3);
            points.reserve(points.size() + triangles.size() * 6);

            for (const auto& triangle : triangles)
            {
                verts.emplace_back(triangle.m_points[0]);
                verts.emplace_back(triangle.m_points[1]);
                verts.emplace_back(triangle.m_points[2]);

                indices.emplace_back(index++);
                indices.emplace_back(index++);
                indices.emplace_back(index++);

                points.emplace_back(triangle.m_points[0]);
                points.emplace_back(triangle.m_points[1]);
                points.emplace_back(triangle.m_points[1]);
                points.emplace_back(triangle.m_points[2]);
                points.emplace_back(triangle.m_points[2]);
                points.emplace_back(triangle.m_points[0]);
            }
        }

        // Collider
        void Collider::Reflect(AZ::ReflectContext* context)
        {
            if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<Collider>()
                    ->Version(1)
                    ->Field("LocallyEnabled", &Collider::m_locallyEnabled)
                    ->Field("GlobalButtonState", &Collider::m_globalButtonState)
                    ;

                if (auto editContext = serializeContext->GetEditContext())
                {
                    using GlobalCollisionDebugState = Debug::DebugDisplayData::GlobalCollisionDebugState;
                    using VisibilityFunc = bool(*)();

                    editContext->Class<Collider>(
                        "PhysX Collider Debug Draw", "Manages global and per-collider debug draw settings and logic")
                        ->DataElement(AZ::Edit::UIHandlers::CheckBox, &Collider::m_locallyEnabled, "Draw collider",
                            "Shows the geometry for the collider in the viewport")
                        ->Attribute(AZ::Edit::Attributes::CheckboxTooltip,
                            "If set, the geometry of this collider is visible in the viewport")
                        ->Attribute(AZ::Edit::Attributes::Visibility,
                            VisibilityFunc{ []() { return IsGlobalColliderDebugCheck(GlobalCollisionDebugState::Manual); } })
                        ->DataElement(AZ::Edit::UIHandlers::Button, &Collider::m_globalButtonState, "Draw collider",
                            "Shows the geometry for the collider in the viewport")
                        ->Attribute(AZ::Edit::Attributes::ButtonText, "Global override")
                        ->Attribute(AZ::Edit::Attributes::ButtonTooltip,
                            "A global setting is overriding this property (to disable the override, "
                            "set the Global Collision Debug setting to \"Set manually\" in the PhysX Configuration)")
                        ->Attribute(AZ::Edit::Attributes::Visibility,
                            VisibilityFunc{ []() { return !IsGlobalColliderDebugCheck(GlobalCollisionDebugState::Manual); } })
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &OpenPhysXSettingsWindow)
                        ;
                }
            }
        }

        void Collider::Connect(AZ::EntityId entityId)
        {
            m_entityId = entityId;
            AzFramework::EntityDebugDisplayEventBus::Handler::BusConnect(m_entityId);
        }

        void Collider::SetDisplayCallback(const DisplayCallback* callback)
        {
            m_displayCallback = callback;
        }

        void Collider::Disconnect()
        {
            AzFramework::EntityDebugDisplayEventBus::Handler::BusDisconnect();
            m_displayCallback = nullptr;
            m_entityId = AZ::EntityId();

            ClearCachedGeometry();
        }

        bool Collider::HasCachedGeometry() const
        {
            return !m_geometry.empty();
        }

        void Collider::ClearCachedGeometry()
        {
            m_geometry.clear();
        }

        void Collider::BuildMeshes(const Physics::ShapeConfiguration& shapeConfig, AZ::u32 geomIndex) const
        {
            if (m_geometry.size() <= geomIndex)
            {
                m_geometry.resize(geomIndex + 1);
            }

            GeometryData& geom = m_geometry[geomIndex];

            AZStd::vector<AZ::Vector3>& verts = geom.m_verts;
            AZStd::vector<AZ::Vector3>& points = geom.m_points;
            AZStd::vector<AZ::u32>& indices = geom.m_indices;

            verts.clear();
            indices.clear();
            points.clear();

            switch (shapeConfig.GetShapeType())
            {
            case Physics::ShapeType::Sphere:
            {
                const auto& sphereConfig = static_cast<const Physics::SphereShapeConfiguration&>(shapeConfig);
                AZ::Vector3 boxMax = AZ::Vector3(sphereConfig.m_scale * sphereConfig.m_radius);
                AZ::Aabb aabb = AZ::Aabb::CreateFromMinMax(-boxMax, boxMax);
                BuildAABBVerts(aabb, verts, points, indices);
            }
            break;
            case Physics::ShapeType::Box:
            {
                const auto& boxConfig = static_cast<const Physics::BoxShapeConfiguration&>(shapeConfig);
                AZ::Vector3 boxMax = boxConfig.m_scale * 0.5f * boxConfig.m_dimensions;
                AZ::Aabb aabb = AZ::Aabb::CreateFromMinMax(-boxMax, boxMax);
                BuildAABBVerts(aabb, verts, points, indices);
            }
            break;
            case Physics::ShapeType::Capsule:
            {
                const auto& capsuleConfig = static_cast<const Physics::CapsuleShapeConfiguration&>(shapeConfig);
                LmbrCentral::CapsuleGeometrySystemRequestBus::Broadcast(
                    &LmbrCentral::CapsuleGeometrySystemRequestBus::Events::GenerateCapsuleMesh,
                    capsuleConfig.m_radius * capsuleConfig.m_scale.GetX(),
                    capsuleConfig.m_height * capsuleConfig.m_scale.GetZ(),
                    16, 8, verts, indices, points);
            }
            break;
            case Physics::ShapeType::CookedMesh:
            {
                const auto& cookedMeshConfig = static_cast<const Physics::CookedMeshShapeConfiguration&>(shapeConfig);
                physx::PxBase* meshData = static_cast<physx::PxBase*>(cookedMeshConfig.GetCachedNativeMesh());
                if (meshData)
                {
                    if (meshData->is<physx::PxTriangleMesh>())
                    {
                        BuildTriangleMesh(meshData, geomIndex);
                    }
                    else
                    {
                        BuildConvexMesh(meshData, geomIndex);
                    }
                }
                break;
            }
            case Physics::ShapeType::PhysicsAsset:
            {
                AZ_Error("PhysX", false,
                    "DebugDraw::Collider::BuildMeshes: Cannot pass PhysicsAsset configuration since it is a collection of shapes. "
                    "Please iterate over m_colliderShapes in the asset and call this function for each of them. "
                    "Entity %s, ID: %llu", GetEntityName().c_str(), m_entityId);
                break;
            }

            default:
            {
                AZ_Error("PhysX", false, "DebugDraw::Collider::BuildMeshes: Unsupported ShapeType %d. Entity %s, ID: %llu",
                    static_cast<AZ::u32>(shapeConfig.GetShapeType()), GetEntityName().c_str(), m_entityId);
                break;
            }
            }

            if ((indices.size() / 3) >= TrianglesWarningThreshold)
            {
                AZ_Warning("PhysX Debug Draw", false, "Mesh has too many triangles! Entity: '%s', ID: %llu",
                    GetEntityName().c_str(), m_entityId);
            }
        }

        void Collider::BuildTriangleMesh(physx::PxBase* meshData, AZ::u32 geomIndex) const
        {
            GeometryData& geom = m_geometry[geomIndex];

            AZStd::unordered_map<int, AZStd::vector<AZ::u32>>& triangleIndexesByMaterialSlot = geom.m_triangleIndexesByMaterialSlot;
            AZStd::vector<AZ::Vector3>& verts = geom.m_verts;
            AZStd::vector<AZ::Vector3>& points = geom.m_points;
            AZStd::vector<AZ::u32>& indices = geom.m_indices;

            physx::PxTriangleMeshGeometry mesh = physx::PxTriangleMeshGeometry(reinterpret_cast<physx::PxTriangleMesh*>(meshData));

            const physx::PxTriangleMesh* triangleMesh = mesh.triangleMesh;
            const physx::PxVec3* vertices = triangleMesh->getVertices();
            const AZ::u32 vertCount = triangleMesh->getNbVertices();
            const AZ::u32 triangleCount = triangleMesh->getNbTriangles();
            const void* triangles = triangleMesh->getTriangles();

            verts.reserve(vertCount);
            indices.reserve(triangleCount * 3);
            points.reserve(triangleCount * 3 * 2);
            triangleIndexesByMaterialSlot.clear();

            physx::PxTriangleMeshFlags triangleMeshFlags = triangleMesh->getTriangleMeshFlags();
            const bool mesh16BitVertexIndices = triangleMeshFlags.isSet(physx::PxTriangleMeshFlag::Enum::e16_BIT_INDICES);

            auto GetVertIndex = [=](AZ::u32 index) -> AZ::u32
            {
                if (mesh16BitVertexIndices)
                {
                    return reinterpret_cast<const physx::PxU16*>(triangles)[index];
                }
                else
                {
                    return reinterpret_cast<const physx::PxU32*>(triangles)[index];
                }
            };

            for (AZ::u32 vertIndex = 0; vertIndex < vertCount; ++vertIndex)
            {
                AZ::Vector3 vert = PxMathConvert(vertices[vertIndex]);
                verts.push_back(vert);
            }

            for (AZ::u32 triangleIndex = 0; triangleIndex < triangleCount * 3; triangleIndex += 3)
            {
                AZ::u32 index1 = GetVertIndex(triangleIndex);
                AZ::u32 index2 = GetVertIndex(triangleIndex + 1);
                AZ::u32 index3 = GetVertIndex(triangleIndex + 2);

                AZ::Vector3 a = verts[index1];
                AZ::Vector3 b = verts[index2];
                AZ::Vector3 c = verts[index3];
                indices.push_back(index1);
                indices.push_back(index2);
                indices.push_back(index3);

                points.push_back(a);
                points.push_back(b);
                points.push_back(b);
                points.push_back(c);
                points.push_back(c);
                points.push_back(a);

                const physx::PxMaterialTableIndex materialIndex = triangleMesh->getTriangleMaterialIndex(triangleIndex / 3);
                const int slotIndex = static_cast<const int>(materialIndex);
                triangleIndexesByMaterialSlot[slotIndex].push_back(index1);
                triangleIndexesByMaterialSlot[slotIndex].push_back(index2);
                triangleIndexesByMaterialSlot[slotIndex].push_back(index3);
            }
        }

        void Collider::BuildConvexMesh(physx::PxBase* meshData, AZ::u32 geomIndex) const
        {
            GeometryData& geom = m_geometry[geomIndex];

            AZStd::vector<AZ::Vector3>& verts = geom.m_verts;
            AZStd::vector<AZ::Vector3>& points = geom.m_points;

            physx::PxConvexMeshGeometry mesh = physx::PxConvexMeshGeometry(reinterpret_cast<physx::PxConvexMesh*>(meshData));
            const physx::PxConvexMesh* convexMesh = mesh.convexMesh;
            const physx::PxU8* pxIndices = convexMesh->getIndexBuffer();
            const physx::PxVec3* pxVertices = convexMesh->getVertices();
            const AZ::u32 numPolys = convexMesh->getNbPolygons();

            for (AZ::u32 polygonIndex = 0; polygonIndex < numPolys; ++polygonIndex)
            {
                physx::PxHullPolygon poly;
                convexMesh->getPolygonData(polygonIndex, poly);

                AZ::u32 index1 = 0;
                AZ::u32 index2 = 1;
                AZ::u32 index3 = 2;

                const AZ::Vector3 a = PxMathConvert(pxVertices[pxIndices[poly.mIndexBase + index1]]);
                const AZ::u32 triangleCount = poly.mNbVerts - 2;

                for (AZ::u32 triangleIndex = 0; triangleIndex < triangleCount; ++triangleIndex)
                {
                    AZ_Assert(index3 < poly.mNbVerts, "Implementation error: attempted to index outside range of polygon vertices.");

                    const AZ::Vector3 b = PxMathConvert(pxVertices[pxIndices[poly.mIndexBase + index2]]);
                    const AZ::Vector3 c = PxMathConvert(pxVertices[pxIndices[poly.mIndexBase + index3]]);

                    verts.push_back(a);
                    verts.push_back(b);
                    verts.push_back(c);

                    points.push_back(a);
                    points.push_back(b);
                    points.push_back(b);
                    points.push_back(c);
                    points.push_back(c);
                    points.push_back(a);

                    index2 = index3++;
                }
            }
        }

        AZ::Color Collider::CalcDebugColor(const Physics::ColliderConfiguration& colliderConfig
            , const ElementDebugInfo& elementDebugInfo) const
        {
            using GlobalCollisionDebugColorMode = Debug::DebugDisplayData::GlobalCollisionDebugColorMode;

            AZ::Color debugColor = AZ::Colors::White;

            GlobalCollisionDebugColorMode debugDrawColorMode = GlobalCollisionDebugColorMode::MaterialColor;
            if (auto* physXDebug = AZ::Interface<Debug::PhysXDebugInterface>::Get())
            {
                debugDrawColorMode = physXDebug->GetDebugDisplayData().m_globalCollisionDebugDrawColorMode;
            }
                        
            switch (debugDrawColorMode)
            {
            case GlobalCollisionDebugColorMode::MaterialColor:
            {
                Physics::MaterialFromAssetConfiguration materialConfiguration;
                const Physics::MaterialId materialId = colliderConfig.m_materialSelection.GetMaterialId(elementDebugInfo.m_materialSlotIndex);
                if (colliderConfig.m_materialSelection.GetMaterialConfiguration(materialConfiguration, materialId))
                {
                    debugColor = materialConfiguration.m_configuration.m_debugColor;
                }
                break;
            }
            case GlobalCollisionDebugColorMode::ErrorColor:
            {
                debugColor = CalcDebugColorWarning(debugColor, elementDebugInfo.m_numTriangles);
                break;
            }
            // Don't add default case, compilation warning C4062 will happen if user adds a new ColorMode to the enum and not handles the case
            }
            debugColor.SetA(0.5f);
            return debugColor;
        }

        AZ::Color Collider::CalcDebugColorWarning(const AZ::Color& currentColor, AZ::u32 triangleCount) const
        {
            // Show glowing warning color when the triangle count exceeds the maximum allowed triangles
            if (triangleCount > TrianglesWarningThreshold)
            {
                AZ::ScriptTimePoint currentTimePoint;
                AZ::TickRequestBus::BroadcastResult(currentTimePoint, &AZ::TickRequests::GetTimeAtCurrentTick);
                float currentTime = static_cast<float>(currentTimePoint.GetSeconds());
                float alpha = fabsf(sinf(currentTime * AZ::Constants::HalfPi * WarningFrequency));
                alpha *= static_cast<float>(AZStd::GetMin(MaxTrianglesRange, triangleCount - TrianglesWarningThreshold)) /
                    static_cast<float>(TrianglesWarningThreshold);
                return currentColor * (1.0f - alpha) + WarningColor * alpha;
            }
            return currentColor;
        }

        void Collider::DrawSphere(AzFramework::DebugDisplayRequests& debugDisplay,
            const Physics::ColliderConfiguration& colliderConfig,
            const Physics::SphereShapeConfiguration& sphereShapeConfig,
            const AZ::Vector3& colliderScale) const
        {
            const float scaledSphereRadius =
                (Utils::GetNonUniformScale(m_entityId) * colliderScale).GetMaxElement() * sphereShapeConfig.m_radius;

            debugDisplay.PushMatrix(GetColliderLocalTransform(colliderConfig, colliderScale));
            debugDisplay.SetColor(CalcDebugColor(colliderConfig));
            debugDisplay.DrawBall(AZ::Vector3::CreateZero(), scaledSphereRadius);
            debugDisplay.SetColor(WireframeColor);
            debugDisplay.DrawWireSphere(AZ::Vector3::CreateZero(), scaledSphereRadius);
            debugDisplay.PopMatrix();
        }

        void Collider::DrawBox(AzFramework::DebugDisplayRequests& debugDisplay,
            const Physics::ColliderConfiguration& colliderConfig,
            const Physics::BoxShapeConfiguration& boxShapeConfig,
            const AZ::Vector3& colliderScale,
            const bool forceUniformScaling) const
        {
            // The resulting scale is the product of the scale in the entity's transform and the collider scale.
            const AZ::Vector3 resultantScale = Utils::GetNonUniformScale(m_entityId) * colliderScale;

            // Scale the box parameters using the desired method (uniform or non-uniform).
            AZ::Vector3 scaledBoxParameters = boxShapeConfig.m_dimensions * 0.5f;

            if (forceUniformScaling)
            {
                scaledBoxParameters *= resultantScale.GetMaxElement();
            }
            else
            {
                scaledBoxParameters *= resultantScale;
            }

            const AZ::Color& faceColor = CalcDebugColor(colliderConfig);

            debugDisplay.PushMatrix(GetColliderLocalTransform(colliderConfig, colliderScale));
            debugDisplay.SetColor(faceColor);
            debugDisplay.DrawSolidBox(-scaledBoxParameters, scaledBoxParameters);
            debugDisplay.SetColor(WireframeColor);
            debugDisplay.DrawWireBox(-scaledBoxParameters, scaledBoxParameters);
            debugDisplay.PopMatrix();
        }

        void Collider::DrawCapsule(AzFramework::DebugDisplayRequests& debugDisplay,
            const Physics::ColliderConfiguration& colliderConfig,
            const Physics::CapsuleShapeConfiguration& capsuleShapeConfig,
            const AZ::Vector3& colliderScale,
            const bool forceUniformScaling) const
        {
            AZStd::vector<AZ::Vector3> verts;
            AZStd::vector<AZ::Vector3> points;
            AZStd::vector<AZ::u32> indices;

            // The resulting scale is the product of the scale in the entity's transform and the collider scale.
            const AZ::Vector3 resultantScale = Utils::GetNonUniformScale(m_entityId) * colliderScale;

            // Scale the capsule parameters using the desired method (uniform or non-uniform).
            AZ::Vector2 scaledCapsuleParameters = AZ::Vector2(capsuleShapeConfig.m_radius, capsuleShapeConfig.m_height);

            if (forceUniformScaling)
            {
                scaledCapsuleParameters *= resultantScale.GetMaxElement();
            }
            else
            {
                scaledCapsuleParameters *= AZ::Vector2(
                    AZ::GetMax(resultantScale.GetX(), resultantScale.GetY()),
                    resultantScale.GetZ()
                );
            }

            debugDisplay.PushMatrix(GetColliderLocalTransform(colliderConfig, colliderScale));

            LmbrCentral::CapsuleGeometrySystemRequestBus::Broadcast(
                &LmbrCentral::CapsuleGeometrySystemRequestBus::Events::GenerateCapsuleMesh,
                scaledCapsuleParameters.GetX(),
                scaledCapsuleParameters.GetY(),
                16, 8, verts, indices, points);

            const AZ::Color& faceColor = CalcDebugColor(colliderConfig);
            debugDisplay.DrawTrianglesIndexed(verts, indices, faceColor);
            debugDisplay.DrawLines(points, WireframeColor);
            debugDisplay.SetLineWidth(ColliderLineWidth);
            debugDisplay.PopMatrix();
        }

        void Collider::DrawMesh(AzFramework::DebugDisplayRequests& debugDisplay,
            const Physics::ColliderConfiguration& colliderConfig,
            const Physics::CookedMeshShapeConfiguration& meshConfig,
            const AZ::Vector3& meshScale, 
            AZ::u32 geomIndex) const
        {
            if (geomIndex >= m_geometry.size())
            {
                AZ_Error("PhysX", false, "DrawMesh: geomIndex %d is out of range for %s. Size: %d",
                    geomIndex, GetEntityName().c_str(), m_geometry.size());
                return;
            }

            if (meshConfig.GetCachedNativeMesh())
            {
                const AZ::Transform scaleMatrix = AZ::Transform::CreateScale(meshScale);
                debugDisplay.PushMatrix(GetColliderLocalTransform(colliderConfig) * scaleMatrix);

                if (meshConfig.GetMeshType() == Physics::CookedMeshShapeConfiguration::MeshType::TriangleMesh)
                {
                    DrawTriangleMesh(debugDisplay, colliderConfig, geomIndex);
                }
                else
                {
                    DrawConvexMesh(debugDisplay, colliderConfig, geomIndex);
                }

                debugDisplay.PopMatrix();
            }
        }

        void Collider::DrawTriangleMesh(AzFramework::DebugDisplayRequests& debugDisplay,
            const Physics::ColliderConfiguration& colliderConfig,
            AZ::u32 geomIndex) const
        {
            AZ_Assert(geomIndex < m_geometry.size(), "DrawTriangleMesh: geomIndex is out of range");

            const GeometryData& geom = m_geometry[geomIndex];

            const AZStd::unordered_map<int, AZStd::vector<AZ::u32>>& triangleIndexesByMaterialSlot
                = geom.m_triangleIndexesByMaterialSlot;
            const AZStd::vector<AZ::Vector3>& verts = geom.m_verts;
            const AZStd::vector<AZ::Vector3>& points = geom.m_points;

            if (!verts.empty())
            {
                for (const auto& element : triangleIndexesByMaterialSlot)
                {
                    const int materialSlot = element.first;
                    const AZStd::vector<AZ::u32>& triangleIndexes = element.second;
                    const AZ::u32 triangleCount = static_cast<AZ::u32>(triangleIndexes.size() / 3);

                    ElementDebugInfo triangleMeshInfo;
                    triangleMeshInfo.m_numTriangles = triangleCount;
                    triangleMeshInfo.m_materialSlotIndex = materialSlot;

                    debugDisplay.DrawTrianglesIndexed(verts, triangleIndexes
                        , CalcDebugColor(colliderConfig, triangleMeshInfo));
                }
                debugDisplay.DrawLines(points, WireframeColor);
            }
        }

        void Collider::DrawConvexMesh(AzFramework::DebugDisplayRequests& debugDisplay,
            const Physics::ColliderConfiguration& colliderConfig, AZ::u32 geomIndex) const
        {
            AZ_Assert(geomIndex < m_geometry.size(), "DrawConvexMesh: geomIndex is out of range");

            const GeometryData& geom = m_geometry[geomIndex];
            const AZStd::vector<AZ::Vector3>& verts = geom.m_verts;
            const AZStd::vector<AZ::Vector3>& points = geom.m_points;

            if (!verts.empty())
            {
                const AZ::u32 triangleCount = static_cast<AZ::u32>(verts.size() / 3);
                ElementDebugInfo convexMeshInfo;
                convexMeshInfo.m_numTriangles = triangleCount;

                debugDisplay.DrawTriangles(verts, CalcDebugColor(colliderConfig, convexMeshInfo));
                debugDisplay.DrawLines(points, WireframeColor);
            }
        }

        void Collider::DrawPolygonPrism(AzFramework::DebugDisplayRequests& debugDisplay,
            const Physics::ColliderConfiguration& colliderConfig, const AZStd::vector<AZ::Vector3>& points) const
        {
            if (!points.empty())
            {
                debugDisplay.PushMatrix(GetColliderLocalTransform(colliderConfig));
                debugDisplay.SetLineWidth(ColliderLineWidth);
                debugDisplay.DrawLines(points, WireframeColor);
                debugDisplay.PopMatrix();
            }
        }

        AZ::Transform Collider::GetColliderLocalTransform(const Physics::ColliderConfiguration& colliderConfig,
            const AZ::Vector3& colliderScale) const
        {
            // Apply entity world transform scale to collider offset
            const AZ::Vector3 translation =
                colliderConfig.m_position * Utils::GetNonUniformScale(m_entityId) * colliderScale;

            return AZ::Transform::CreateFromQuaternionAndTranslation(
                colliderConfig.m_rotation, translation);
        }

        const AZStd::vector<AZ::Vector3>& Collider::GetVerts(AZ::u32 geomIndex) const
        {
            AZ_Assert(geomIndex < m_geometry.size(), "GetVerts: geomIndex %d is out of range for %s. Size: %d",
                geomIndex, GetEntityName().c_str(), m_geometry.size());
            return m_geometry[geomIndex].m_verts;
        }

        const AZStd::vector<AZ::Vector3>& Collider::GetPoints(AZ::u32 geomIndex) const
        {
            AZ_Assert(geomIndex < m_geometry.size(), "GetPoints: geomIndex %d is out of range for %s. Size: %d",
                geomIndex, GetEntityName().c_str(), m_geometry.size());
            return m_geometry[geomIndex].m_points;
        }

        const AZStd::vector<AZ::u32>& Collider::GetIndices(AZ::u32 geomIndex) const
        {
            AZ_Assert(geomIndex < m_geometry.size(), "GetIndices: geomIndex %d is out of range for %s. Size: %d",
                geomIndex, GetEntityName().c_str(), m_geometry.size());
            return m_geometry[geomIndex].m_indices;
        }

        AZ::u32 Collider::GetNumShapes() const
        {
            return static_cast<AZ::u32>(m_geometry.size());
        }

        // AzFramework::EntityDebugDisplayEventBus
        void Collider::DisplayEntityViewport(
            [[maybe_unused]] const AzFramework::ViewportInfo& viewportInfo,
            AzFramework::DebugDisplayRequests& debugDisplay)
        {
            if (!m_displayCallback)
            {
                return;
            }

            // Let each collider decide how to scale itself, so extract the scale here.
            AZ::Transform entityWorldTransformWithoutScale = AZ::Transform::CreateIdentity();
            AZ::TransformBus::EventResult(entityWorldTransformWithoutScale, m_entityId, &AZ::TransformInterface::GetWorldTM);
            entityWorldTransformWithoutScale.ExtractScale();

            auto* physXDebug = AZ::Interface<Debug::PhysXDebugInterface>::Get();
            if (physXDebug == nullptr)
            {
                return;
            }
            const PhysX::Debug::DebugDisplayData& displayData = physXDebug->GetDebugDisplayData();

            const PhysX::Debug::ColliderProximityVisualization& proximityVisualization = displayData.m_colliderProximityVisualization;
            const bool colliderIsInRange =
                proximityVisualization.m_cameraPosition.GetDistanceSq(entityWorldTransformWithoutScale.GetTranslation()) <
                proximityVisualization.m_radius * proximityVisualization.m_radius;

            const PhysX::Debug::DebugDisplayData::GlobalCollisionDebugState& globalCollisionDebugDraw = displayData.m_globalCollisionDebugDraw;
            if (globalCollisionDebugDraw != PhysX::Debug::DebugDisplayData::GlobalCollisionDebugState::AlwaysOff)
            {
                if (globalCollisionDebugDraw == PhysX::Debug::DebugDisplayData::GlobalCollisionDebugState::AlwaysOn
                    || m_locallyEnabled
                    || (proximityVisualization.m_enabled && colliderIsInRange))
                {
                    debugDisplay.PushMatrix(entityWorldTransformWithoutScale);
                    m_displayCallback->Display(debugDisplay);
                    debugDisplay.PopMatrix();
                }
            }
        }

        AZStd::string Collider::GetEntityName() const 
        {
            AZStd::string entityName;
            AZ::ComponentApplicationBus::BroadcastResult(entityName, 
                &AZ::ComponentApplicationRequests::GetEntityName, m_entityId);

            return entityName;
        }

    } // namespace DebugDraw
} // namespace PhysX
