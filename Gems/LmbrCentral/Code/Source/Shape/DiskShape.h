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
#include <LmbrCentral/Shape/ShapeComponentBus.h>
#include <LmbrCentral/Shape/DiskShapeComponentBus.h>

namespace AzFramework
{
    class DebugDisplayRequests;
}

namespace LmbrCentral
{
    struct ShapeDrawParams;

    //! Provide DiskShape functionality.
    class DiskShape
        : public ShapeComponentRequestsBus::Handler
        , public DiskShapeComponentRequestBus::Handler
        , public AZ::TransformNotificationBus::Handler
    {
    public:
        AZ_CLASS_ALLOCATOR(DiskShape, AZ::SystemAllocator, 0)
        AZ_RTTI(DiskShape, "{21E75068-3E05-4DD2-981A-DAEB0B1A9BC4}")

        static void Reflect(AZ::ReflectContext* context);

        void Activate(AZ::EntityId entityId);
        void Deactivate();
        void InvalidateCache(InvalidateShapeCacheReason reason);

        // ShapeComponentRequestsBus
        AZ::Crc32 GetShapeType() override { return AZ_CRC("DiskShape", 0x8c83427c); }
        AZ::Aabb GetEncompassingAabb() override;
        void GetTransformAndLocalBounds(AZ::Transform& transform, AZ::Aabb& bounds) override;
        bool IsPointInside(const AZ::Vector3& point)  override;
        float DistanceSquaredFromPoint(const AZ::Vector3& point) override;
        bool IntersectRay(const AZ::Vector3& src, const AZ::Vector3& dir, float& distance) override;

        // DiskShapeComponentRequestBus
        DiskShapeConfig GetDiskConfiguration() override;
        void SetRadius(float radius) override;
        float GetRadius() override;
        const AZ::Vector3& GetNormal() override;

        // AZ::TransformNotificationBus
        void OnTransformChanged(const AZ::Transform& local, const AZ::Transform& world) override;

        const DiskShapeConfig& GetDiskConfiguration() const;
        void SetDiskConfiguration(const DiskShapeConfig& diskShapeConfig);
        const AZ::Transform& GetCurrentTransform() const;

    protected:

        friend class EditorDiskShapeComponent;
        ShapeComponentConfig& ModifyShapeComponent();

    private:
        //! Runtime data - cache potentially expensive operations.
        class DiskIntersectionDataCache
            : public IntersectionTestDataCache<DiskShapeConfig>
        {
            void UpdateIntersectionParamsImpl(
                const AZ::Transform& currentTransform, const DiskShapeConfig& configuration,
                const AZ::Vector3& currentNonUniformScale = AZ::Vector3::CreateOne()) override;

            friend class DiskShape;

            AZ::Vector3 m_position; ///< Position of the center of the disk.
            AZ::Vector3 m_normal; ///< normal of the disk.
            float m_radius = 0.0f; ///< Radius of the disk (including entity scale).
        };

        DiskShapeConfig m_diskShapeConfig; ///< Underlying disk configuration.
        DiskIntersectionDataCache m_intersectionDataCache; ///< Caches transient intersection data.
        AZ::Transform m_currentTransform; ///< Caches the current world transform.
        AZ::EntityId m_entityId; ///< The Id of the entity the shape is attached to.
    };

    void DrawDiskShape(
        const ShapeDrawParams& shapeDrawParams, const DiskShapeConfig& diskShapeConfig,
        AzFramework::DebugDisplayRequests& debugDisplay);

} // namespace LmbrCentral
