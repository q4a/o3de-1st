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

#include <Atom/RPI.Reflect/Model/ModelLodAsset.h>
#include <Atom/RPI.Reflect/AssetCreator.h>

namespace AZ
{
    namespace RPI
    {
        //! Constructs an instance of an ModelLodAsset.
        class ModelLodAssetCreator
            : public AssetCreator<ModelLodAsset>
        {
        public:
            ModelLodAssetCreator() = default;
            ~ModelLodAssetCreator() = default;

            //! Begins construction of a new ModelLodAsset instance. Resets the creator to a fresh state
            //! @param assetId The unique id to use when creating the asset
            void Begin(const Data::AssetId& assetId);

            //! Sets the lod-wide index buffer that can be referenced by subsequent meshes.
            //! @param bufferAsset The buffer asset to set as the lod's index buffer
            void SetLodIndexBuffer(const Data::Asset<BufferAsset>& bufferAsset);

            //! Adds an lod-wide stream buffer that can be referenced by subsequent meshes.
            //! @param bufferAsset The buffer asset to add as an lod-wide stream buffer
            void AddLodStreamBuffer(const Data::Asset<BufferAsset>& bufferAsset);

            //! Begins the addition of a Mesh to the ModelLodAsset. Begin must be called first.
            void BeginMesh();

            //! Sets the name of the current SubMesh.
            void SetMeshName(const AZ::Name& name);

            //! Sets the Aabb of the current SubMesh.
            //! Begin and BeginMesh must be called first.
            void SetMeshAabb(AZ::Aabb&& aabb);

            //! Sets the material asset for the current SubMesh.
            //! Begin and BeginMesh must be called first
            void SetMeshMaterialAsset(const Data::Asset<MaterialAsset>& materialAsset);

            //! Sets the given BufferAssetView to the current SubMesh as the index buffer.
            //! Begin and BeginMesh must be called first
            void SetMeshIndexBuffer(const BufferAssetView& bufferAssetView);

            //! Adds a BufferAssetView to the current SubMesh as a stream buffer that matches the given semantic name.
            //! Begin and BeginMesh must be called first
            void AddMeshStreamBuffer(
                const RHI::ShaderSemantic& streamSemantic,
                const AZ::Name& customName,
                const BufferAssetView& bufferAssetView);

            //! Adds a StreamBufferInfo struct to the current SubMesh
            //! Begin and BeginMesh must be called first
            void AddMeshStreamBuffer(
                const ModelLodAsset::Mesh::StreamBufferInfo& streamBufferInfo);

            //! Finalizes creation of the current Mesh and adds it to the current ModelLodAsset.
            void EndMesh();

            //! Finalizes the ModelLodAsset and assigns ownership of the asset to result if successful, otherwise returns false and result is left untouched.
            bool End(Data::Asset<ModelLodAsset>& result);

        private:
            bool m_meshBegan = false;

            ModelLodAsset::Mesh m_currentMesh;
            bool ValidateIsMeshReady();
            bool ValidateIsMeshEnded();
            bool ValidateMesh(const ModelLodAsset::Mesh& mesh);
            bool ValidateLod();
        };
    } // namespace RPI
} // namespace AZ
