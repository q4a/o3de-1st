/*
 * All or portions of this file Copyright(c) Amazon.com, Inc.or its affiliates or
 * its licensors.
 *
 * For complete copyright and license terms please see the LICENSE at the root of this
 * distribution(the "License").All use of this software is governed by the License,
 *or, if provided, by the license below or the license accompanying this file.Do not
 * remove or modify any license notices.This file is distributed on an "AS IS" BASIS,
 *WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *
 */

#pragma once

#include <Atom/RPI.Reflect/Model/ModelAsset.h>
#include <Atom/RPI.Reflect/Model/ModelLodAsset.h>
#include <Atom/RPI.Reflect/Material/MaterialAsset.h>
#include <Atom/RPI.Reflect/Buffer/BufferAsset.h>

#include <AzCore/Asset/AssetCommon.h>

#include <SceneAPI/SceneCore/Events/ExportEventContext.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace DataTypes
        {
            class IMeshGroup;
        }
        namespace Containers
        {
            class Scene;
        }
    }

    namespace RPI
    {
        struct NamedMaterialAsset
        {
            Data::Asset<MaterialAsset> m_asset;
            AZStd::string m_name;
        };
        using MaterialAssetsByUid = AZStd::unordered_map<uint64_t, NamedMaterialAsset>;

        struct ModelAssetBuilderContext : public AZ::SceneAPI::Events::ICallContext
        {
            AZ_RTTI(
                ModelAssetBuilderContext,
                "{63FEFB4B-25DC-48DD-AC72-D27DA9A6D94A}",
                AZ::SceneAPI::Events::ICallContext);

            ModelAssetBuilderContext(
                const AZ::SceneAPI::Containers::Scene& scene,
                const AZ::SceneAPI::DataTypes::IMeshGroup& group,
                const MaterialAssetsByUid& materialsByUid,
                Data::Asset<ModelAsset>& outputModelAsset);
            ~ModelAssetBuilderContext() override = default;

            ModelAssetBuilderContext& operator=(const ModelAssetBuilderContext& other) = delete;

            const AZ::SceneAPI::Containers::Scene& m_scene;
            const AZ::SceneAPI::DataTypes::IMeshGroup& m_group;
            const MaterialAssetsByUid& m_materialsByUid;
            Data::Asset<ModelAsset>& m_outputModelAsset;
        };

        struct MaterialAssetBuilderContext : public AZ::SceneAPI::Events::ICallContext
        {
            AZ_RTTI(
                MaterialAssetBuilderContext,
                "{6451418A-453B-4646-A5B2-A5687FA2E97F}",
                AZ::SceneAPI::Events::ICallContext);

            MaterialAssetBuilderContext(const AZ::SceneAPI::Containers::Scene& scene, MaterialAssetsByUid& outputMaterialsByUid);
            ~MaterialAssetBuilderContext() override = default;

            MaterialAssetBuilderContext& operator=(const MaterialAssetBuilderContext& other) = delete;

            const AZ::SceneAPI::Containers::Scene& m_scene;
            MaterialAssetsByUid& m_outputMaterialsByUid;
        };
    } // namespace RPI
} // namespace AZ
