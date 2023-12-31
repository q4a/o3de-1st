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

#include <AzCore/Math/Sphere.h>
#include <AzCore/Math/Frustum.h>
#include <AzCore/base.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/Interface/Interface.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/parallel/mutex.h>

#include <AzCore/Console/IConsole.h>
#include <AzCore/Console/Console.h>
#include <AzCore/Math/Obb.h>
#include <AzCore/std/limits.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/std/containers/vector.h>

#include <AzFramework/Visibility/IVisibilitySystem.h>

#include <Atom/RPI.Public/View.h>

#include <Atom/RHI/DrawList.h>

#include <AtomCore/std/parallel/concurrency_checker.h>

#ifdef AZ_DEBUG_BUILD
#define AZ_CULL_DEBUG_ENABLED
#endif

namespace AZ
{
    class Job;

    namespace RHI
    {
        class DrawPacket;
    }

    namespace RPI
    {
        class Scene;

        struct Cullable
        {
            struct CullData
            {
                AzFramework::VisibilityEntry m_visibilityEntry;

                //! World-space bounding sphere
                AZ::Sphere m_boundingSphere;
                //! World-space bouding oriented-bounding-box
                AZ::Obb m_boundingObb;

                //! Will only pass visibity if at least one of the drawListMask bits matches the view's drawListMask.
                //! Set to all 1's if the object type doesn't have a drawListMask
                RHI::DrawListMask m_drawListMask;
                //! Will hide this object if any of the hideFlags match the View's usage flags. Useful to hide objects from certain Views.
                //! Set to all 0's if you don't want to hide the object from any Views.
                RPI::View::UsageFlags m_hideFlags = RPI::View::UsageNone;

                class RPI::Scene* m_scene = nullptr;  //[GFX_TODO][ATOM-13796] once the IVisibilitySystem supports multiple octree scenes, remove this
            };
            CullData m_cullData;

            using LodOverride = uint8_t;
            static constexpr uint8_t NoLodOverride = AZStd::numeric_limits<LodOverride>::max();

            struct LodData
            {
                struct Lod
                {
                    float m_screenCoverageMin;
                    float m_screenCoverageMax;
                    AZStd::vector<const RHI::DrawPacket*> m_drawPackets;
                };

                AZStd::vector<Lod> m_lods;

                //! Used for determining which lod(s) to select (usually is smaller than the bounding sphere radius)
                //! Suggest setting to: 0.5f*localAabb.GetExtents().GetMaxElement()
                float m_lodSelectionRadius = 1.0f;

                LodOverride m_lodOverride = NoLodOverride;
            };
            LodData m_lodData;

            void SetDebugName([[maybe_unused]] const AZ::Name& debugName)
            {
#ifdef AZ_CULL_DEBUG_ENABLED
                m_debugName = debugName;
#endif
            }
            const AZ::Name GetDebugName() const
            {
#ifdef AZ_CULL_DEBUG_ENABLED
                return m_debugName;
#else
                return AZ::Name();
#endif
            }

#ifdef AZ_CULL_DEBUG_ENABLED
            AZ::Name m_debugName;
#endif
        };

        class CullingDebugContext
        {
        public:
            AZStd::mutex m_frozenFrustumsMutex;
            AZStd::unordered_map<View*, Frustum> m_frozenFrustums;

            // UI Options
            bool m_enableStats = false;
            bool m_enableFrustumCulling = true;
            bool m_parallelOctreeTraversal = true;
            bool m_freezeFrustums = false;
            bool m_debugDraw = false;
            bool m_drawViewFrustum = false;
            bool m_drawFullyVisibleNodes = false;
            bool m_drawPartiallyVisibleNodes = false;
            bool m_drawBoundingBoxes = true;
            bool m_drawBoundingSpheres = false;
            bool m_drawLodRadii = false;
            bool m_drawWorldCoordinateAxes = false;
            int m_currentViewSelection = 0;
            AZ::Name m_currentViewSelectionName;

            // global metrics
            uint32_t m_numCullablesInScene = 0;

            //! Per-view culling metrics
            class CullStats
            {
            public:
                AZ_TYPE_INFO(AZ::RPI::CullingDebugContext::CullStats, "{3B70C5D3-54F8-4160-8324-DFC71EB47412}");
                AZ_CLASS_ALLOCATOR(CullStats, AZ::SystemAllocator, 0);

                CullStats(AZ::Name name) : m_name(name) {}
                CullStats() = default;
                ~CullStats() = default;

                void Reset()
                {
                    m_numJobs = 0;
                    m_numVisibleCullables = 0;
                    m_numVisibleDrawPackets = 0;
                }

                AZ::Name m_name;
                AZ::Matrix4x4 m_cameraViewToWorld;
                AZStd::atomic_uint32_t m_numJobs = 0;
                AZStd::atomic_uint32_t m_numVisibleCullables = 0;
                AZStd::atomic_uint32_t m_numVisibleDrawPackets = 0;
            };

            CullingDebugContext() = default;
            ~CullingDebugContext();

            //! Finds or creates a new CullStats struct for a given view.
            //! Once accessed, use the CullStats to accumulate metrics for a frame.
            //! Is designed to be threadsafe
            CullStats& GetCullStatsForView(View* view);

            void ResetCullStats();

            //! For internal use only. Use GetCullStatsForView() instead
            AZStd::unordered_map<View*, CullStats*>& LockAndGetAllCullStats()
            {
                m_frozenFrustumsMutex.lock();
                return m_perViewCullStats;
            }
            //! For internal use only.
            void UnlockAllCullStats()
            {
                m_frozenFrustumsMutex.unlock();
            }

        private:
            AZStd::unordered_map<View*, CullStats*> m_perViewCullStats;
            AZStd::mutex m_perViewCullStatsMutex;
        };

        //! Selects an lod (based on size-in-screnspace) and adds the appropriate DrawPackets to the view.
        uint32_t AddLodDataToView(const Vector3& pos, const Cullable::LodData& lodData, RPI::View& view);

        //! Centralized manager for culling-related processing.
        //! There is one CullingSystem owned by each Scene, so external systems (such as FeatureProcessors) should
        //! access the CullingSystem via their parent Scene.
        class CullingSystem
        {
        public:
            AZ_RTTI(CullingSystem, "{5B23B55B-8A1D-4B0D-9760-15E87FC8518A}");
            AZ_CLASS_ALLOCATOR(CullingSystem, AZ::SystemAllocator, 0);
            AZ_DISABLE_COPY_MOVE(CullingSystem);

            CullingSystem() = default;
            virtual ~CullingSystem() = default;

            void Activate(const class Scene* parentScene);
            void Deactivate();

            //! Notifies the CullingSystem that culling will begin for this frame.
            void BeginCulling(const AZStd::vector<ViewPtr>& views);

            //! Notifies the CullingSystem that the culling is done for this frame.
            void EndCulling();

            //! Performs render culling and lod selection for a View, then adds the visible renderpackets to that View.
            //! Must be called between BeginCulling() and EndCulling(), once for each active scene/view pair.
            //! Will create child jobs under the parentJob to do the processing in parallel.
            //! Can be called in parallel (i.e. to perform culling on multiple views at the same time).
            void ProcessCullables(const Scene& scene, View& view, AZ::Job& parentJob);

            //! Adds a Cullable to the underlying visibility system(s).
            //! Must be called at least once on initialization and whenever a Cullable's position or bounds is changed.
            //! Is not threadsafe, so call this from the main thread outside of Begin/EndCulling()
            void RegisterOrUpdateCullable(Cullable& cullable);

            //! Removes a Cullable from the underlying visibility system(s).
            //! Must be called once for each cullable object on de-initialization.
            //! Is not threadsafe, so call this from the main thread outside of Begin/EndCulling()
            void UnregisterCullable(Cullable& cullable);

            //! Returns the number of cullables that have been added to the CullingSystem
            uint32_t GetNumCullables() const;

            CullingDebugContext& GetDebugContext()
            {
                return m_debugCtx;
            }

            static const size_t WorkListCapacity = 5;
            using WorkListType = AZStd::fixed_vector<AzFramework::IVisibilitySystem::NodeData, WorkListCapacity>;

        protected:
            size_t CountObjectsInScene();

            const Scene* m_parentScene = nullptr;

            CullingDebugContext m_debugCtx;

            AZStd::concurrency_checker m_cullDataConcurrencyCheck;

        };
        

        
    } // namespace RPI
} // namespace AZ
