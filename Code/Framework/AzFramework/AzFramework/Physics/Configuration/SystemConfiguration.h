/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates, or
* a third party where indicated.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
#pragma once

#include <AzCore/RTTI/RTTI.h>
#include <AzFramework/Physics/Configuration/CollisionConfiguration.h>

namespace AZ
{
    class ReflectContext;
}

namespace AzPhysics
{
    //! Contains global physics settings.
    //! Used to initialize the Physics System.
    struct SystemConfiguration
    {
        AZ_CLASS_ALLOCATOR_DECL;
        AZ_RTTI(SystemConfiguration, "{24697CAF-AC00-443D-9C27-28D58734A84C}");
        static void Reflect(AZ::ReflectContext* context);

        SystemConfiguration() = default;
        virtual ~SystemConfiguration() = default;

        static constexpr float DefaultFixedTimestep = 0.0166667f; //! Value represents 1/60th or 60 FPS.

        float m_maxTimestep = 1.f / 20.f; //!< Maximum fixed timestep in seconds to run the physics update.
        float m_fixedTimestep = DefaultFixedTimestep; //!< Timestep in seconds to run the physics update. See DefaultFixedTimestep.

        AZ::u64 m_raycastBufferSize = 32; //!< Maximum number of hits that will be returned from a raycast.
        AZ::u64 m_shapecastBufferSize = 32; //!< Maximum number of hits that can be returned from a shapecast.
        AZ::u64 m_overlapBufferSize = 32; //!< Maximum number of overlaps that can be returned from an overlap query.

        //! Contains the default global collision layers and groups.
        //! Each Physics Scene uses this as a base and will override as needed.
        CollisionConfiguration m_collisionConfig;

        //! Controls whether the Physics System will self register to the TickBus and call StartSimulation / FinishSimulation on each Scene.
        //! Disable this to manually control Physics Scene simulation logic.
        bool m_autoManageSimulationUpdate = true;

        bool operator==(const SystemConfiguration& other) const;
        bool operator!=(const SystemConfiguration& other) const;
    };
}
