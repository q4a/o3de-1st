#
# All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
# its licensors.
#
# For complete copyright and license terms please see the LICENSE at the root of this
# distribution (the "License"). All use of this software is governed by the License,
# or, if provided, by the license below or the license accompanying this file. Do not
# remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#

set(FILES
    Include/GraphModel/GraphModelBus.h
    Include/GraphModel/Model/Common.h
    Include/GraphModel/Model/Connection.h
    Include/GraphModel/Model/DataType.h
    Include/GraphModel/Model/Graph.h
    Include/GraphModel/Model/GraphElement.h
    Include/GraphModel/Model/IGraphContext.h
    Include/GraphModel/Model/Node.h
    Include/GraphModel/Model/Slot.h
    Include/GraphModel/Model/Module/InputOutputNodes.h
    Include/GraphModel/Model/Module/ModuleGraphManager.h
    Include/GraphModel/Model/Module/ModuleNode.h
    Include/GraphModel/Integration/EditorMainWindow.h
    Include/GraphModel/Integration/ReadOnlyDataInterface.h
    Include/GraphModel/Integration/ThumbnailItem.h
    Include/GraphModel/Integration/ThumbnailImageItem.h
    Include/GraphModel/Integration/GraphCanvasMetadata.h
    Include/GraphModel/Integration/GraphController.h
    Include/GraphModel/Integration/GraphControllerManager.h
    Include/GraphModel/Integration/Helpers.h
    Include/GraphModel/Integration/BooleanDataInterface.h
    Include/GraphModel/Integration/FloatDataInterface.h
    Include/GraphModel/Integration/IntegerDataInterface.h
    Include/GraphModel/Integration/StringDataInterface.h
    Include/GraphModel/Integration/VectorDataInterface.inl
    Include/GraphModel/Integration/IntegrationBus.h
    Include/GraphModel/Integration/NodePalette/InputOutputNodePaletteItem.h
    Include/GraphModel/Integration/NodePalette/ModuleNodePaletteItem.h
    Include/GraphModel/Integration/NodePalette/StandardNodePaletteItem.h
    Include/GraphModel/Integration/NodePalette/GraphCanvasNodePaletteItems.h
    Source/GraphModelSystemComponent.cpp
    Source/GraphModelSystemComponent.h
    Source/Model/Connection.cpp
    Source/Model/DataType.cpp
    Source/Model/Graph.cpp
    Source/Model/GraphElement.cpp
    Source/Model/Node.cpp
    Source/Model/Slot.cpp
    Source/Model/Module/InputOutputNodes.cpp
    Source/Model/Module/ModuleGraphManager.cpp
    Source/Model/Module/ModuleNode.cpp
    Source/Integration/EditorMainWindow.cpp
    Source/Integration/ReadOnlyDataInterface.cpp
    Source/Integration/ThumbnailItem.cpp
    Source/Integration/ThumbnailImageItem.cpp
    Source/Integration/GraphCanvasMetadata.cpp
    Source/Integration/GraphController.cpp
    Source/Integration/GraphControllerManager.cpp
    Source/Integration/BooleanDataInterface.cpp
    Source/Integration/FloatDataInterface.cpp
    Source/Integration/IntegerDataInterface.cpp
    Source/Integration/StringDataInterface.cpp
    Source/Integration/NodePalette/GraphCanvasNodePaletteItems.cpp
)
