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

#include <AzCore/Math/Vector3.h>
#include <AzCore/std/containers/vector.h>

#include <SceneAPI/SceneData/SceneDataConfiguration.h>
#include <SceneAPI/SceneCore/DataTypes/GraphData/IMeshVertexBitangentData.h>


namespace AZ
{
    namespace SceneData
    {
        namespace GraphData
        {

            class SCENE_DATA_CLASS MeshVertexBitangentData
                : public AZ::SceneAPI::DataTypes::IMeshVertexBitangentData
            {
            public:                
                AZ_RTTI(MeshVertexBitangentData, "{F56FB088-4C92-4453-AFE9-4E820F03FA90}", AZ::SceneAPI::DataTypes::IMeshVertexBitangentData);

                SCENE_DATA_API ~MeshVertexBitangentData() override = default;

                SCENE_DATA_API size_t GetCount() const override;
                SCENE_DATA_API const AZ::Vector3& GetBitangent(size_t index) const override;
                SCENE_DATA_API void SetBitangent(size_t vertexIndex, const AZ::Vector3& bitangent) override;

                SCENE_DATA_API void SetBitangentSetIndex(size_t setIndex) override;
                SCENE_DATA_API size_t GetBitangentSetIndex() const override;

                SCENE_DATA_API void Resize(size_t numVerts);
                SCENE_DATA_API void ReserveContainerSpace(size_t numVerts);
                SCENE_DATA_API void AppendBitangent(const AZ::Vector3& bitangent);

                SCENE_DATA_API AZ::SceneAPI::DataTypes::TangentSpace GetTangentSpace() const override;
                SCENE_DATA_API void SetTangentSpace(AZ::SceneAPI::DataTypes::TangentSpace space) override;

                SCENE_DATA_API void GetDebugOutput(AZ::SceneAPI::Utilities::DebugOutput& output) const override;
            protected:
                AZStd::vector<AZ::Vector3>              m_bitangents;
                AZ::SceneAPI::DataTypes::TangentSpace   m_tangentSpace = AZ::SceneAPI::DataTypes::TangentSpace::FromFbx;
                size_t                                  m_setIndex = 0;
            };

        } // GraphData
    } // SceneData
} // AZ
