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
#include "LmbrCentral_precompiled.h"
#include "NavigationSystemComponent.h"

#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Memory/SystemAllocator.h>

// AZ to LY conversion
#include <MathConversion.h>

// for INavigationSystem access
#include <ISystem.h>
#include <functor.h> // needed in <INavigationSystem.h>
#include <INavigationSystem.h>


namespace LmbrCentral
{
    AZ_CLASS_ALLOCATOR_IMPL(NavRayCastResult, AZ::SystemAllocator, 0)

    void NavigationSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<NavigationSystemComponent, Component>()
                ->Version(1)
                ;

            serializeContext->Class<NavRayCastResult>()
                ->Version(1)
                ->Field("collision", &NavRayCastResult::m_collision)
                ->Field("position", &NavRayCastResult::m_position)
                ->Field("meshId", &NavRayCastResult::m_meshId)
                ;
        }

        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            // RayCastWorld return type
            behaviorContext->Class<NavRayCastResult>()
                ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
                ->Property("collision", BehaviorValueGetter(&NavRayCastResult::m_collision), nullptr)
                ->Property("position", BehaviorValueGetter(&NavRayCastResult::m_position), nullptr)
                ->Property("meshId", BehaviorValueGetter(&NavRayCastResult::m_meshId), nullptr)
                ;

            behaviorContext->EBus<NavigationSystemRequestBus>("NavigationSystemRequestBus")
                ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
                ->Event("RayCast", &NavigationSystemRequestBus::Events::RayCast)
                ;
        }
    }

    void NavigationSystemComponent::Activate()
    {
        // start listening for for OnCrySystemInitialized call
        CrySystemEventBus::Handler::BusConnect();
    }

    void NavigationSystemComponent::Deactivate()
    {
        // disconnect the bus here, in case it hasn't already been disconnected in OnCrySystemShutdown
        NavigationSystemRequestBus::Handler::BusDisconnect();

        // disconnect here even if we haven't received OnCrySystemShutdown
        CrySystemEventBus::Handler::BusDisconnect();
    }

    NavRayCastResult NavigationSystemComponent::RayCast(const AZ::Vector3& begin, const AZ::Vector3& direction, float maxDistance)
    {
        NavRayCastResult result;

        // Convert to Cry
        const Vec3 LYstart = AZVec3ToLYVec3(begin);
        const Vec3 LYend = AZVec3ToLYVec3(begin + (direction * maxDistance));

        const INavigationSystem* navigationSystem = nullptr; // INavigationSystem will be converted to an AZInterface (LY-111343)
        if (navigationSystem)
        {
            // perform raycast
            const std::tuple<bool, NavigationMeshID, Vec3> LYResult = navigationSystem->RaycastWorld(LYstart, LYend);

            // translate result
            result.m_collision = std::get<0>(LYResult);
            result.m_meshId = std::get<1>(LYResult);
            result.m_position = LYVec3ToAZVec3(std::get<2>(LYResult));
        }

        return result;
    }

    void NavigationSystemComponent::OnCrySystemInitialized([[maybe_unused]] ISystem& system, const SSystemInitParams&)
    {
        NavigationSystemRequestBus::Handler::BusConnect();
    }

    void NavigationSystemComponent::OnCrySystemShutdown([[maybe_unused]] ISystem& system)
    {
        // disconnect the bus here, in case it hasn't already been disconnected from Deactivate
        NavigationSystemRequestBus::Handler::BusDisconnect();
    }
} // namespace LmbrCentral
