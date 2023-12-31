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

namespace AZ
{
    namespace Render
    {
        // AsyncLoadTracker is for use by Feature Processors to track in-flight loading of assets that their sub-objects need.
        // For instance, the individual decals that are controlled by the DecalFeatureProcessor will need materials to be loaded in asynchronously before use.
        template<typename FeatureProcessorHandle>
        class AsyncLoadTracker
        {
        public:

            void TrackAssetLoad(const FeatureProcessorHandle handle, const AZ::Data::AssetId asset)
            {
                if (IsAssetLoading(handle))
                {
                    // We might have a case where a decal was told to load an asset, then while the load was being fulfilled, it was told to
                    // switch to a different asset. That is why we are removing the existing handle rather than just returning.
                    RemoveHandle(handle);
                }
                Add(handle, asset);
            }

            bool IsAssetLoading(const AZ::Data::AssetId asset) const
            {
                return m_inFlightHandlesByAsset.count(asset) > 0;
            }

            bool IsAssetLoading(const FeatureProcessorHandle handle) const
            {
                return m_inFlightHandles.count(handle) > 0;
            }

            AZStd::vector<FeatureProcessorHandle> GetHandlesByAsset(const AZ::Data::AssetId asset) const
            {
                const auto iter = m_inFlightHandlesByAsset.find(asset);
                if (iter != m_inFlightHandlesByAsset.end())
                {
                    return iter->second;
                }
                else
                {
                    return {};
                }
            }

            void RemoveAllHandlesWithAsset(const AZ::Data::AssetId asset)
            {
                const auto iter = m_inFlightHandlesByAsset.find(asset);
                if (iter == m_inFlightHandlesByAsset.end())
                {
                    return;
                }

                auto& handleList = iter->second;
                for (auto& handle : handleList)
                {
                    EraseFromInFlightHandles(handle);
                }
                m_inFlightHandlesByAsset.erase(iter);
            }

            void RemoveHandle(const FeatureProcessorHandle handle)
            {
                const auto asset = EraseFromInFlightHandles(handle);

                AZ_Assert(m_inFlightHandlesByAsset.count(asset) > 0, "AsyncLoadTracker in a bad state");
                auto& handleList = m_inFlightHandlesByAsset[asset];
                EraseFromVector(handleList, handle);
                if (handleList.empty())
                {
                    m_inFlightHandlesByAsset.erase(asset);
                }
            }

            bool AreAnyLoadsInFlight() const
            {
                return !m_inFlightHandles.empty();
            }

        private:

            // Helper function that erases from an AZStd::vector via swap() and pop_back()
            template<typename T, typename U>
            static void EraseFromVector(AZStd::vector<T>& vec, const U elementToErase)
            {
                const auto iter = AZStd::find(vec.begin(), vec.end(), elementToErase);
                AZ_Assert(iter != vec.end(), "EraseFromVector failed to find the given object");
                const auto indexToRemove = AZStd::distance(vec.begin(), iter);
                AZStd::swap(*iter, vec.back());
                vec.pop_back();
            }

            void Add(const FeatureProcessorHandle handle, const AZ::Data::AssetId asset)
            {
                AZ_Assert(m_inFlightHandles.count(handle) == 0, "AsyncLoadTracker::Add() - told to add a handle that was already being tracked.");
                m_inFlightHandlesByAsset[asset].push_back(handle);
                m_inFlightHandles[handle] = asset;
            }

            AZ::Data::AssetId EraseFromInFlightHandles(const FeatureProcessorHandle handle)
            {
                const auto iter = m_inFlightHandles.find(handle);
                AZ_Assert(iter != m_inFlightHandles.end(), "Told to remove handle that was not present");
                const auto asset = iter->second;
                m_inFlightHandles.erase(iter);
                return asset;
            }

            // Tracks all objects that need a particular asset
            AZStd::unordered_map<AZ::Data::AssetId, AZStd::vector<FeatureProcessorHandle>> m_inFlightHandlesByAsset;

            // Hash table that tracks the reverse of the m_inFlightHandlesByAsset hash table.
            // i.e. for each object, it stores what asset that it needs.
            AZStd::unordered_map<FeatureProcessorHandle, AZ::Data::AssetId> m_inFlightHandles;
        };
    }
}
