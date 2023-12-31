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

#include <AzCore/std/containers/vector.h>
#include <EMotionFX/Source/MeshBuilderVertexAttributeLayers.h>

namespace EMotionFX
{
    // forward declarations
    class MeshBuilder;

    class MeshBuilderSubMesh
    {
    public:
        AZ_CLASS_ALLOCATOR_DECL
        MeshBuilderSubMesh(size_t materialIndex, MeshBuilder* mesh);

        size_t GetNumIndices() const { return m_indices.size(); }
        size_t GetNumPolygons() const { return m_polyVertexCounts.size(); }
        size_t GetNumJoints() const { return m_jointList.size(); }
        size_t GetMaterialIndex() const { return m_materialIndex; }
        size_t GetNumVertices() const { return m_numVertices; }
        size_t GetJoint(size_t index) const { return m_jointList[index]; }
        uint8 GetPolygonVertexCount(size_t polyIndex) const { return m_polyVertexCounts[polyIndex]; }
        AZ_FORCE_INLINE MeshBuilderVertexLookup& GetVertex(size_t index)
        {
            if (m_vertexOrder.size() != m_numVertices)
            {
                GenerateVertexOrder();
            }
            return m_vertexOrder[index];
        }
        MeshBuilder* GetMesh() const { return m_mesh; }
        size_t GetIndex(size_t index);

        void GenerateVertexOrder();

        void SetJoints(const AZStd::vector<size_t>& jointList) { m_jointList = jointList; }
        const AZStd::vector<size_t>& GetJoints() const { return m_jointList; }

        void Optimize();
        void AddPolygon(const AZStd::vector<MeshBuilderVertexLookup>& indices, const AZStd::vector<size_t>& jointList);
        bool CanHandlePolygon(const AZStd::vector<size_t>& orgVertexNumbers, size_t materialIndex, AZStd::vector<size_t>& outJointList) const;

        size_t CalcNumSimilarJoints(const AZStd::vector<size_t>& jointList) const;

    private:
        AZStd::vector<MeshBuilderVertexLookup> m_indices;
        AZStd::vector<MeshBuilderVertexLookup> m_vertexOrder;
        AZStd::vector<size_t> m_jointList;
        AZStd::vector<AZ::u8> m_polyVertexCounts;
        size_t m_materialIndex = InvalidIndex32;
        size_t m_numVertices = 0;
        MeshBuilder* m_mesh = nullptr;

        bool CheckIfHasVertex(const MeshBuilderVertexLookup& vertex);
    };
} // namespace EMotionFX
