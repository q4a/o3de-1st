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

#include <AzCore/Math/Matrix3x4.h>
#include <AzCore/std/algorithm.h>
#include <AzCore/std/typetraits/is_base_of.h>
#include <SceneAPI/SceneCore/Containers/Scene.h>
#include <SceneAPI/SceneCore/Containers/SceneGraph.h>
#include <SceneAPI/SceneCore/Containers/Views/FilterIterator.h>
#include <SceneAPI/SceneCore/Containers/Utilities/Filters.h>
#include <SceneAPI/SceneCore/Containers/Utilities/SceneGraphUtilities.h>
#include <SceneAPI/SceneCore/Containers/Views/SceneGraphChildIterator.h>
#include <SceneAPI/SceneCore/DataTypes/GraphData/ITransform.h>
#include <SceneAPI/SceneCore/Events/GraphMetaInfoBus.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace Utilities
        {
            DataTypes::MatrixType BuildWorldTransform(const Containers::SceneGraph& graph, Containers::SceneGraph::NodeIndex nodeIndex)
            {
                DataTypes::MatrixType outTransform = DataTypes::MatrixType::Identity();
                while (nodeIndex.IsValid())
                {
                    auto view = Containers::Views::MakeSceneGraphChildView<Containers::Views::AcceptEndPointsOnly>(graph, nodeIndex,
                        graph.GetContentStorage().begin(), true);
                    auto result = AZStd::find_if(view.begin(), view.end(), Containers::DerivedTypeFilter<DataTypes::ITransform>());
                    if (result != view.end())
                    {
                        // Check if the node has any child transform node
                        const DataTypes::MatrixType& azTransform = azrtti_cast<const DataTypes::ITransform*>(result->get())->GetMatrix();
                        outTransform = azTransform * outTransform;
                    }
                    else
                    {
                        // Check if the node itself is a transform node.
                        AZStd::shared_ptr<const DataTypes::ITransform> transformData = azrtti_cast<const DataTypes::ITransform*>(graph.GetNodeContent(nodeIndex));
                        if (transformData)
                        {
                            outTransform = transformData->GetMatrix() * outTransform;
                        }
                    }

                    if (graph.HasNodeParent(nodeIndex))
                    {
                        nodeIndex = graph.GetNodeParent(nodeIndex);
                    }
                    else
                    {
                        break;
                    }
                }

                return outTransform;
            }
        } // Utilities
    } // SceneAPI
} // AZ
