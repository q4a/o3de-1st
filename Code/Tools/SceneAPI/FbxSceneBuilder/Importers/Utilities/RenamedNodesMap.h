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

#include <memory>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/string/string.h>
#include <SceneAPI/SceneCore/Containers/SceneGraph.h>
#include <SceneAPI/SDKWrapper/NodeWrapper.h>

namespace AZ
{
    namespace FbxSDKWrapper
    {
        class FbxNodeWrapper;
    }

    namespace SceneAPI
    {
        namespace FbxSceneBuilder
        {
            class RenamedNodesMap
            {
            public:
                //! Checks if the provided name is valid for the position in the graph and makes corrections if
                //! problems are found.
                //! @param name The name of the node in the scene graph.
                //! @param graph The scene graph the node will be added to.
                //! @param parentNode The node that will be the intended parent for the the node who's name is being checked.
                //! @param defaultName If the provided name is empty, the defaultName will be used.
                //! @return True if the name was updated otherwise false.
                static bool SanitizeNodeName(AZStd::string& name, const Containers::SceneGraph& graph,
                    Containers::SceneGraph::NodeIndex parentNode, const char* defaultName = "unnamed");
                
                //! Register the name for later reference. If the name needs to be sanitized, the sanitized name will be stored.
                //! @param node The node that's to be registered.
                //! @param graph The scene graph the node will be added to.
                //! @param parentNode The node that will be the intended parent for the the node who's name is being checked.
                //! @param defaultName If the provided name is empty, the defaultName will be used.
                //! @return True if the node was successfully registered.
                bool RegisterNode(const std::shared_ptr<SDKNode::NodeWrapper>& node, const Containers::SceneGraph& graph,
                    Containers::SceneGraph::NodeIndex parentNode, const char* defaultName = "unnamed");
                bool RegisterNode(const std::shared_ptr<const SDKNode::NodeWrapper>& node, const Containers::SceneGraph& graph,
                    Containers::SceneGraph::NodeIndex parentNode, const char* defaultName = "unnamed");
                bool RegisterNode(const SDKNode::NodeWrapper& node, const Containers::SceneGraph& graph,
                    Containers::SceneGraph::NodeIndex parentNode, const char* defaultName = "unnamed");

                //! Returns the name of the given node, which may be sanitized if this was needed.
                const char* GetNodeName(const std::shared_ptr<SDKNode::NodeWrapper>& node) const;
                const char* GetNodeName(const std::shared_ptr<const SDKNode::NodeWrapper>& node) const;
                const char* GetNodeName(const AZStd::shared_ptr<SDKNode::NodeWrapper>& node) const;
                const char* GetNodeName(const AZStd::shared_ptr<const SDKNode::NodeWrapper>& node) const;
                const char* GetNodeName(const SDKNode::NodeWrapper& node) const;
            private:

                AZStd::unordered_map<u64, AZStd::string> m_idToName;
            };
        } // namespace FbxSceneBuilder
    } // namespace SceneAPI
} // namespace AZ
