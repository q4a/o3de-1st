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

#include <Atom/RPI.Public/Culling.h>
#include <Atom/RPI.Public/Model/Model.h>
#include <AzCore/Component/ComponentBus.h>

namespace AZ
{
    namespace Render
    {
        /**
         * MeshComponentRequestBus provides an interface to request operations on a MeshComponent
         */
        class MeshComponentRequests
            : public ComponentBus
        {
        public:
            virtual void SetModelAsset(Data::Asset<RPI::ModelAsset> modelAsset) = 0;
            virtual const Data::Asset<RPI::ModelAsset>& GetModelAsset() const = 0;

            virtual void SetModelAssetId(Data::AssetId modelAssetId) = 0;
            virtual Data::AssetId GetModelAssetId() const = 0;

            virtual void SetModelAssetPath(const AZStd::string& path) = 0;
            virtual AZStd::string GetModelAssetPath() const = 0;

            virtual const Data::Instance<RPI::Model> GetModel() const = 0;

            virtual void SetSortKey(RHI::DrawItemSortKey sortKey) = 0;
            virtual RHI::DrawItemSortKey GetSortKey() const = 0;

            virtual void SetLodOverride(RPI::Cullable::LodOverride lodOverride) = 0;
            virtual RPI::Cullable::LodOverride GetLodOverride() const = 0;

            virtual void SetVisibility(bool visible) = 0;
            virtual bool GetVisibility() const = 0;

            virtual AZ::Aabb GetWorldBounds() = 0;

            virtual AZ::Aabb GetLocalBounds() = 0;
        };
        using MeshComponentRequestBus = EBus<MeshComponentRequests>;

        /**
         * MeshComponent can send out notifications on the MeshComponentNotificationBus
         */
        class MeshComponentNotifications
            : public ComponentBus
        {
        public:
            virtual void OnModelReady(const Data::Asset<RPI::ModelAsset>& modelAsset, const Data::Instance<RPI::Model>& model) = 0;
        };
        using MeshComponentNotificationBus = EBus<MeshComponentNotifications>;

    } // namespace Render
} // namespace AZ
