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

#include <AzCore/Component/ComponentBus.h>
#include <AzCore/Component/TransformBus.h>
#include <AzFramework/Render/GeometryIntersectionStructures.h>
// required by components implementing BoundsRequestBus to notify the
// Entity that their bounds has changed (or to access the cached entity bound union)
#include <AzFramework/Visibility/EntityBoundsUnionBus.h>

namespace AZ
{
    class ReflectContext;
}

namespace AzFramework
{
    //! Implemented by components that provide bounds for use with various systems.
    class BoundsRequests : public AZ::ComponentBus
    {
    public:
        static void Reflect(AZ::ReflectContext* context);

        //! Returns an axis aligned bounding box in world space.
        //! @note It is preferred to use CalculateEntityWorldBoundsUnion in the general case as
        //! more than one component may be providing a bound. It isn't guaranteed which bound
        //! will be returned by a single call to GetWorldBounds.
        virtual AZ::Aabb GetWorldBounds() = 0;
        //! Returns an axis aligned bounding box in local space.
        //! @note It is preferred to use CalculateEntityLocalBoundsUnion in the general case as
        //! more than one component may be providing a bound. It isn't guaranteed which bound
        //! will be returned by a single call to GetLocalBounds.
        virtual AZ::Aabb GetLocalBounds() = 0;

    protected:
        ~BoundsRequests() = default;
    };

    using BoundsRequestBus = AZ::EBus<BoundsRequests>;

    //! Returns a union of all local Aabbs provided by components implementing the BoundsRequestBus.
    //! @note It is preferred to call this function as opposed to GetLocalBounds directly as more than one
    //! component may be implementing this bus on an Entity and so only the first result (Aabb) will be returned.
    inline AZ::Aabb CalculateEntityLocalBoundsUnion(const AZ::EntityId entityId)
    {
        AZ::EBusReduceResult<AZ::Aabb, AabbUnionAggregator> aabbResult(AZ::Aabb::CreateNull());
        BoundsRequestBus::EventResult(
            aabbResult, entityId, &BoundsRequestBus::Events::GetLocalBounds);

        if (aabbResult.value.IsValid())
        {
            return aabbResult.value;
        }

        return AZ::Aabb::CreateFromMinMax(-AZ::Vector3(0.5f), AZ::Vector3(0.5f));
    }

    //! Returns a union of all world Aabbs provided by components implementing the BoundsRequestBus.
    //! @note It is preferred to call this function as opposed to GetWorldBounds directly as more than one
    //! component may be implementing this bus on an Entity and so only the first result (Aabb) will be returned.
    inline AZ::Aabb CalculateEntityWorldBoundsUnion(const AZ::EntityId entityId)
    {
        AZ::EBusReduceResult<AZ::Aabb, AabbUnionAggregator> aabbResult(AZ::Aabb::CreateNull());
        BoundsRequestBus::EventResult(aabbResult, entityId, &BoundsRequestBus::Events::GetWorldBounds);

        if (aabbResult.value.IsValid())
        {
            return aabbResult.value;
        }

        AZ::Vector3 worldTranslation = AZ::Vector3::CreateZero();
        AZ::TransformBus::EventResult(worldTranslation, entityId, &AZ::TransformBus::Events::GetWorldTranslation);
        return AZ::Aabb::CreateCenterHalfExtents(worldTranslation, AZ::Vector3(0.5f));
    }
} // namespace AzFramework
