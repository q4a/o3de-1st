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

#include <SceneAPI/FbxSceneBuilder/ImportContexts/AssImpImportContexts.h>
#include <SceneAPI/SDKWrapper/AssImpNodeWrapper.h>
#include <SceneAPI/SceneCore/Events/ImportEventContext.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace FbxSceneBuilder
        {
            AssImpImportContext::AssImpImportContext(const AssImpSDKWrapper::AssImpSceneWrapper& sourceScene,
                const FbxSceneSystem& sourceSceneSystem,
                AssImpSDKWrapper::AssImpNodeWrapper& sourceNode)
                : m_sourceScene(sourceScene)
                , m_sourceSceneSystem(sourceSceneSystem)
                , m_sourceNode(sourceNode)
            {
            }

            AssImpNodeEncounteredContext::AssImpNodeEncounteredContext(Containers::Scene& scene,
                Containers::SceneGraph::NodeIndex currentGraphPosition,
                const AssImpSDKWrapper::AssImpSceneWrapper& sourceScene,
                const FbxSceneSystem& sourceSceneSystem,
                RenamedNodesMap& nodeNameMap,
                AssImpSDKWrapper::AssImpNodeWrapper& sourceNode)
                : AssImpImportContext(sourceScene, sourceSceneSystem, sourceNode)
                , NodeEncounteredContext(scene, currentGraphPosition, nodeNameMap)
            {
            }

            AssImpNodeEncounteredContext::AssImpNodeEncounteredContext(
                Events::ImportEventContext& parent,
                Containers::SceneGraph::NodeIndex currentGraphPosition,
                const AssImpSDKWrapper::AssImpSceneWrapper& sourceScene,
                const FbxSceneSystem& sourceSceneSystem,
                RenamedNodesMap& nodeNameMap,
                AssImpSDKWrapper::AssImpNodeWrapper& sourceNode)
                : AssImpImportContext(sourceScene, sourceSceneSystem, sourceNode)
                , NodeEncounteredContext(parent.GetScene(), currentGraphPosition, nodeNameMap)
            {
            }

            AssImpSceneDataPopulatedContext::AssImpSceneDataPopulatedContext(AssImpNodeEncounteredContext& parent,
                const AZStd::shared_ptr<DataTypes::IGraphObject>& graphData, const AZStd::string& dataName)
                : AssImpImportContext(parent.m_sourceScene, parent.m_sourceSceneSystem, parent.m_sourceNode)
                , SceneDataPopulatedContextBase(parent, graphData, dataName)
            {
            }

            AssImpSceneDataPopulatedContext::AssImpSceneDataPopulatedContext(Containers::Scene& scene,
                Containers::SceneGraph::NodeIndex currentGraphPosition,
                const AssImpSDKWrapper::AssImpSceneWrapper& sourceScene,
                const FbxSceneSystem& sourceSceneSystem,
                RenamedNodesMap& nodeNameMap,
                AssImpSDKWrapper::AssImpNodeWrapper& sourceNode,
                const AZStd::shared_ptr<DataTypes::IGraphObject>& nodeData, const AZStd::string& dataName)
                : AssImpImportContext(sourceScene, sourceSceneSystem, sourceNode)
                , SceneDataPopulatedContextBase(scene, currentGraphPosition, nodeNameMap, nodeData, dataName)
            {
            }

            AssImpSceneNodeAppendedContext::AssImpSceneNodeAppendedContext(AssImpSceneDataPopulatedContext& parent,
                Containers::SceneGraph::NodeIndex newIndex)
                : AssImpImportContext(parent.m_sourceScene, parent.m_sourceSceneSystem, parent.m_sourceNode)
                , SceneNodeAppendedContextBase(parent.m_scene, newIndex, parent.m_nodeNameMap)
            {
            }

            AssImpSceneNodeAppendedContext::AssImpSceneNodeAppendedContext(Containers::Scene& scene,
                Containers::SceneGraph::NodeIndex currentGraphPosition,
                const AssImpSDKWrapper::AssImpSceneWrapper& sourceScene,
                const FbxSceneSystem& sourceSceneSystem,
                RenamedNodesMap& nodeNameMap, AssImpSDKWrapper::AssImpNodeWrapper& sourceNode)
                : AssImpImportContext(sourceScene, sourceSceneSystem, sourceNode)
                , SceneNodeAppendedContextBase(scene, currentGraphPosition, nodeNameMap)
            {
            }

            AssImpSceneAttributeDataPopulatedContext::AssImpSceneAttributeDataPopulatedContext(AssImpSceneNodeAppendedContext& parent, const AZStd::shared_ptr<DataTypes::IGraphObject>& nodeData, const Containers::SceneGraph::NodeIndex attributeNodeIndex, const AZStd::string& dataName)
                : AssImpImportContext(parent.m_sourceScene, parent.m_sourceSceneSystem, parent.m_sourceNode)
                , SceneAttributeDataPopulatedContextBase(parent, nodeData, attributeNodeIndex, dataName)
            {
            }

            AssImpSceneAttributeNodeAppendedContext::AssImpSceneAttributeNodeAppendedContext(AssImpSceneAttributeDataPopulatedContext& parent, Containers::SceneGraph::NodeIndex newIndex)
                : AssImpImportContext(parent.m_sourceScene, parent.m_sourceSceneSystem, parent.m_sourceNode)
                , SceneAttributeNodeAppendedContextBase(parent, newIndex)
            {
            }

            AssImpSceneNodeAddedAttributesContext::AssImpSceneNodeAddedAttributesContext(AssImpSceneNodeAppendedContext& parent)
                : AssImpImportContext(parent.m_sourceScene, parent.m_sourceSceneSystem, parent.m_sourceNode)
                , SceneNodeAddedAttributesContextBase(parent)
            {
            }

            AssImpSceneNodeFinalizeContext::AssImpSceneNodeFinalizeContext(AssImpSceneNodeAddedAttributesContext& parent)
                : AssImpImportContext(parent.m_sourceScene, parent.m_sourceSceneSystem, parent.m_sourceNode)
                , SceneNodeFinalizeContextBase(parent)
            {
            }

            

            AssImpFinalizeSceneContext::AssImpFinalizeSceneContext(Containers::Scene& scene,
                const AssImpSDKWrapper::AssImpSceneWrapper& sourceScene,
                const FbxSceneSystem& sourceSceneSystem,
                RenamedNodesMap& nodeNameMap)
                : FinalizeSceneContextBase(scene, nodeNameMap)
                , m_sourceScene(sourceScene)
                , m_sourceSceneSystem(sourceSceneSystem)
            {
            }

        } // namespace SceneAPI
    } // namespace FbxSceneBuilder
} // namespace AZ
