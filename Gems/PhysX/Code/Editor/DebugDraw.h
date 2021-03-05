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
#include <AzCore/Serialization/SerializeContext.h>
#include <AzFramework/Entity/EntityDebugDisplayBus.h>
#include <AzFramework/Physics/ShapeConfiguration.h>
#include <AzFramework/Physics/Shape.h>
#include <PhysX/MeshAsset.h>
#include <PhysX/Debug/PhysXDebugConfiguration.h>

namespace PhysX
{
    namespace DebugDraw
    {
        //! Open the PhysX Settings Window on the Global Settings tab.
        void OpenPhysXSettingsWindow();

        //! Determine if the global debug draw preference is set to the specified state.
        //! @param requiredState The collider debug state to check against the global state.
        //! @return True if global collider debug state matches the input requiredState.
        bool IsGlobalColliderDebugCheck(Debug::DebugDisplayData::GlobalCollisionDebugState requiredState);

        class DisplayCallback
        {
        public:
            virtual void Display(AzFramework::DebugDisplayRequests& debugDisplayRequests) const = 0;
        protected:
            ~DisplayCallback() = default;
        };

        class Collider
            : protected AzFramework::EntityDebugDisplayEventBus::Handler
        {
        public:
            AZ_CLASS_ALLOCATOR(Collider, AZ::SystemAllocator, 0);
            AZ_RTTI(Collider, "{7DE9CA01-DF1E-4D72-BBF4-76C9136BE6A2}");
            static void Reflect(AZ::ReflectContext* context);

            Collider() = default;

            void Connect(AZ::EntityId entityId);
            void SetDisplayCallback(const DisplayCallback* callback);
            void Disconnect();

            bool HasCachedGeometry() const;
            void ClearCachedGeometry();

            void BuildMeshes(const Physics::ShapeConfiguration& shapeConfig, AZ::u32 geomIndex) const;

            struct ElementDebugInfo
            {
                // Note: this doesn't use member initializer because of a bug in Mac OS compiler
                ElementDebugInfo()
                    : m_materialSlotIndex(0)
                    , m_numTriangles(0)
                {}

                int m_materialSlotIndex;
                AZ::u32 m_numTriangles;
            };

            AZ::Color CalcDebugColor(const Physics::ColliderConfiguration& colliderConfig
                , const ElementDebugInfo& elementToDebugInfo = ElementDebugInfo()) const;

            AZ::Color CalcDebugColorWarning(const AZ::Color& baseColor, AZ::u32 triangleCount) const;

            void DrawSphere(AzFramework::DebugDisplayRequests& debugDisplay,
                const Physics::ColliderConfiguration& colliderConfig,
                const Physics::SphereShapeConfiguration& sphereShapeConfig,
                const AZ::Vector3& colliderScale = AZ::Vector3::CreateOne()) const;

            void DrawBox(AzFramework::DebugDisplayRequests& debugDisplay,
                const Physics::ColliderConfiguration& colliderConfig,
                const Physics::BoxShapeConfiguration& boxShapeConfig,
                const AZ::Vector3& colliderScale = AZ::Vector3::CreateOne(),
                const bool forceUniformScaling = false) const;

            void DrawCapsule(AzFramework::DebugDisplayRequests& debugDisplay,
                const Physics::ColliderConfiguration& colliderConfig,
                const Physics::CapsuleShapeConfiguration& capsuleShapeConfig,
                const AZ::Vector3& colliderScale = AZ::Vector3::CreateOne(),
                const bool forceUniformScaling = false) const;

            void DrawMesh(AzFramework::DebugDisplayRequests& debugDisplay,
                const Physics::ColliderConfiguration& colliderConfig,
                const Physics::CookedMeshShapeConfiguration& assetConfig,
                const AZ::Vector3& meshScale,
                AZ::u32 geomIndex) const;

            void DrawPolygonPrism(AzFramework::DebugDisplayRequests& debugDisplay,
                const Physics::ColliderConfiguration& colliderConfig, const AZStd::vector<AZ::Vector3>& points) const;

            AZ::Transform GetColliderLocalTransform(const Physics::ColliderConfiguration& colliderConfig,
                const AZ::Vector3& colliderScale = AZ::Vector3::CreateOne()) const;

            AZ::u32 GetNumShapes() const;
            const AZStd::vector<AZ::Vector3>& GetVerts(AZ::u32 geomIndex) const;
            const AZStd::vector<AZ::Vector3>& GetPoints(AZ::u32 geomIndex) const;
            const AZStd::vector<AZ::u32>& GetIndices(AZ::u32 geomIndex) const;

        protected:
            // AzFramework::EntityDebugDisplayEventBus
            void DisplayEntityViewport(
                const AzFramework::ViewportInfo& viewportInfo,
                AzFramework::DebugDisplayRequests& debugDisplay) override;

            // Internal mesh drawing subroutines 
            void DrawTriangleMesh(AzFramework::DebugDisplayRequests& debugDisplay,
                const Physics::ColliderConfiguration& colliderConfig, AZ::u32 geomIndex) const;

            void DrawConvexMesh(AzFramework::DebugDisplayRequests& debugDisplay,
                const Physics::ColliderConfiguration& colliderConfig, AZ::u32 geomIndex) const;

            void BuildTriangleMesh(physx::PxBase* meshData, AZ::u32 geomIndex) const;

            void BuildConvexMesh(physx::PxBase* meshData, AZ::u32 geomIndex) const;

            AZStd::string GetEntityName() const;

            bool m_globalButtonState = false; //!< Button linked to the global debug preference.
            bool m_locallyEnabled = true; //!< Local setting to enable displaying the collider in editor view.
            AZ::EntityId m_entityId;
            const DisplayCallback* m_displayCallback = nullptr;

            struct GeometryData
            {
                AZStd::unordered_map<int, AZStd::vector<AZ::u32>> m_triangleIndexesByMaterialSlot;
                AZStd::vector<AZ::Vector3> m_verts;
                AZStd::vector<AZ::Vector3> m_points;
                AZStd::vector<AZ::u32> m_indices;
            };

            mutable AZStd::vector<GeometryData> m_geometry;
        };
    } // namespace DebugDraw
} // namespace PhysX