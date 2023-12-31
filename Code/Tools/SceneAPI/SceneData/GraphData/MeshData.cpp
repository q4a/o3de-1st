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

#include <SceneAPI/SceneData/GraphData/MeshData.h>
#include <AzCore/Casting/numeric_cast.h>

namespace AZ
{
    namespace SceneData
    {
        namespace GraphData
        {
            namespace DataTypes = AZ::SceneAPI::DataTypes;


            MeshData::~MeshData() = default;

            void MeshData::AddPosition(const AZ::Vector3& position)
            {
                m_positions.push_back(position);
            }

            void MeshData::AddNormal(const AZ::Vector3& normal)
            {
                m_normals.push_back(normal);
            }

            //assume consistent winding - no stripping or fanning expected (3 index per face)
            //indices can be used for position and normal
            void MeshData::AddFace(unsigned int index1, unsigned int index2, unsigned int index3, unsigned int faceMaterialId)
            {
                IMeshData::Face face;
                face.vertexIndex[0] = index1;
                face.vertexIndex[1] = index2;
                face.vertexIndex[2] = index3;
                m_faceList.push_back(face);
                m_faceMaterialIds.push_back(faceMaterialId);
            }

            void MeshData::AddFace(const DataTypes::IMeshData::Face& face, unsigned int faceMaterialId)
            {
                m_faceList.push_back(face);
                m_faceMaterialIds.push_back(faceMaterialId);
            }

            void MeshData::SetSdkMeshIndex(int sdkMeshIndex)
            {
                m_sdkMeshIndex = sdkMeshIndex;
            }
            int MeshData::GetSdkMeshIndex() const
            {
                return m_sdkMeshIndex;
            }

            void MeshData::SetVertexIndexToControlPointIndexMap(int vertexIndex, int controlPointIndex)
            {
                m_vertexIndexToControlPointIndexMap[vertexIndex] = controlPointIndex;

                // The above hashmap stores the control point index (value) per vertex (key).
                // We construct an unordered set and fill in the control point indices in order to get access to the number of unique control points indices.
                if (m_controlPointToUsedVertexIndexMap.find(controlPointIndex) == m_controlPointToUsedVertexIndexMap.end())
                {
                    m_controlPointToUsedVertexIndexMap[controlPointIndex] = aznumeric_cast<unsigned int>(m_controlPointToUsedVertexIndexMap.size());
                }
            }

            int MeshData::GetControlPointIndex(int vertexIndex) const
            {
                AZ_Assert(m_vertexIndexToControlPointIndexMap.find(vertexIndex) != m_vertexIndexToControlPointIndexMap.end(), "Vertex index %i doesn't exist", vertexIndex);
                // Note: AZStd::unordered_map's operator [] doesn't have const version... 
                return m_vertexIndexToControlPointIndexMap.find(vertexIndex)->second;
            }

            size_t MeshData::GetUsedControlPointCount() const
            {
                return m_controlPointToUsedVertexIndexMap.size();
            }

            int MeshData::GetUsedPointIndexForControlPoint(int controlPointIndex) const
            {
                auto iter = m_controlPointToUsedVertexIndexMap.find(controlPointIndex);
                if (iter != m_controlPointToUsedVertexIndexMap.end())
                {
                    return iter->second;
                }
                else
                {
                    return -1; // That control point is not used in this mesh
                }
            }

            unsigned int MeshData::GetVertexCount() const
            {
                return static_cast<unsigned int>(m_positions.size());
            }

            bool MeshData::HasNormalData() const
            {
                return m_normals.size() > 0;
            }

            const AZ::Vector3& MeshData::GetPosition(unsigned int index) const
            {
                AZ_Assert(index < m_positions.size(), "GetPosition index not in range");
                return m_positions[index];
            }

            const AZ::Vector3& MeshData::GetNormal(unsigned int index) const
            {
                AZ_Assert(index < m_normals.size(), "GetNormal index not in range");
                return m_normals[index];
            }

            unsigned int MeshData::GetFaceCount() const
            {
                return static_cast<unsigned int>(m_faceList.size());
            }

            const  DataTypes::IMeshData::Face& MeshData::GetFaceInfo(unsigned int index) const
            {
                AZ_Assert(index < m_faceList.size(), "GetFaceInfo index not in range");
                return m_faceList[index];
            }

            unsigned int MeshData::GetFaceMaterialId(unsigned int index) const
            {
                AZ_Assert(index < m_faceMaterialIds.size(), "GetFaceMaterialIds index not in range");
                return m_faceMaterialIds[index];
            }

            unsigned int MeshData::GetVertexIndex(int faceIndex, int vertexIndexInFace) const
            {
                AZ_Assert(faceIndex < m_faceList.size(), "GetFaceInfo index not in range");
                AZ_Assert(vertexIndexInFace < 3, "vertexIndexInFace index not in range");
                return m_faceList[faceIndex].vertexIndex[vertexIndexInFace];
            }

            void MeshData::GetDebugOutput(SceneAPI::Utilities::DebugOutput& output) const
            {
                output.Write("Positions", m_positions);
                output.Write("Normals", m_normals);
                output.Write("FaceList", m_faceList);
                output.Write("FaceMaterialIds", m_faceMaterialIds);
            }
        }
    }
}
