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

#include <AzCore/Casting/numeric_cast.h>
#include <SceneAPI/SceneData/GraphData/SkinWeightData.h>

namespace AZ
{
    namespace SceneData
    {
        namespace GraphData
        {
            size_t SkinWeightData::GetVertexCount() const
            {
                return m_vertexLinks.size();
            }

            size_t SkinWeightData::GetLinkCount(size_t vertexIndex) const
            {
                AZ_Assert(vertexIndex < m_vertexLinks.size(), "Invalid vertex index %i for skin weight data links.", vertexIndex);
                return m_vertexLinks[vertexIndex].size();
            }

            const SceneAPI::DataTypes::ISkinWeightData::Link& SkinWeightData::GetLink(size_t vertexIndex, size_t linkIndex) const
            {
                AZ_Assert(vertexIndex < m_vertexLinks.size(), "Invalid vertex index %i for skin weight data links.", vertexIndex);
                AZ_Assert(linkIndex < m_vertexLinks[vertexIndex].size(), "Invalid link index %i for skin weight data %i.", linkIndex, vertexIndex);
                return m_vertexLinks[vertexIndex][linkIndex];
            }

            SceneAPI::DataTypes::ISkinWeightData::Link& SkinWeightData::GetLink(size_t vertexIndex, size_t linkIndex)
            {
                AZ_Assert(vertexIndex < m_vertexLinks.size(), "Invalid vertex index %i for skin weight data links.", vertexIndex);
                AZ_Assert(linkIndex < m_vertexLinks[vertexIndex].size(), "Invalid link index %i for skin weight data %i.", linkIndex, vertexIndex);
                return m_vertexLinks[vertexIndex][linkIndex];
            }

            size_t SkinWeightData::GetBoneCount() const
            {
                return m_boneIdNameMap.size();
            }

            const AZStd::string& SkinWeightData::GetBoneName(int boneId) const
            {
                AZ_Assert(m_boneIdNameMap.find(boneId) != m_boneIdNameMap.end(), "Invalid bone id %i to look up bone name.", boneId);
                return m_boneIdNameMap.at(boneId);
            }

            void SkinWeightData::ResizeContainerSpace(size_t size)
            {
                m_vertexLinks.resize(size);
            }

            void SkinWeightData::AppendLink(size_t vertexIndex, const SceneAPI::DataTypes::ISkinWeightData::Link& link)
            {
                AZ_Assert(vertexIndex < m_vertexLinks.size(), "Invalid vertex index %i for skin weight data links.", vertexIndex);
                m_vertexLinks[vertexIndex].push_back(link);
            }

            int SkinWeightData::GetBoneId(const AZStd::string& boneName)
            {
                if (m_boneNameIdMap.find(boneName) == m_boneNameIdMap.end())
                {
                    m_boneNameIdMap[boneName] = aznumeric_caster(m_boneNameIdMap.size());
                    m_boneIdNameMap[m_boneNameIdMap[boneName]] = boneName;
                }
                return m_boneNameIdMap[boneName];
            }

            void SkinWeightData::GetDebugOutput(AZ::SceneAPI::Utilities::DebugOutput& output) const
            {
                output.Write("VertexLinks", m_vertexLinks);
            }
        } // GraphData
    } // SceneData
} // AZ
