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
    Source/VegetationEditorModule.h
    Source/VegetationEditorModule.cpp
    Include/Vegetation/Editor/EditorAreaComponentBase.h
    Include/Vegetation/Editor/EditorAreaComponentBase.inl
    Include/Vegetation/Editor/EditorVegetationComponentBase.h
    Include/Vegetation/Editor/EditorVegetationComponentBase.inl
    Include/Vegetation/Editor/EditorVegetationComponentTypeIds.h
    Source/Editor/EditorVegetationSystemComponent.cpp
    Source/Editor/EditorVegetationSystemComponent.h
    Source/Editor/EditorAreaBlenderComponent.cpp
    Source/Editor/EditorAreaBlenderComponent.h
    Source/Editor/EditorBlockerComponent.cpp
    Source/Editor/EditorBlockerComponent.h
    Source/Editor/EditorDescriptorListCombinerComponent.cpp
    Source/Editor/EditorDescriptorListCombinerComponent.h
    Source/Editor/EditorDescriptorListComponent.cpp
    Source/Editor/EditorDescriptorListComponent.h
    Source/Editor/EditorDescriptorWeightSelectorComponent.cpp
    Source/Editor/EditorDescriptorWeightSelectorComponent.h
    Source/Editor/EditorDistanceBetweenFilterComponent.cpp
    Source/Editor/EditorDistanceBetweenFilterComponent.h
    Source/Editor/EditorDistributionFilterComponent.cpp
    Source/Editor/EditorDistributionFilterComponent.h
    Source/Editor/EditorLevelSettingsComponent.cpp
    Source/Editor/EditorLevelSettingsComponent.h
    Source/Editor/EditorMeshBlockerComponent.cpp
    Source/Editor/EditorMeshBlockerComponent.h
    Source/Editor/EditorPositionModifierComponent.cpp
    Source/Editor/EditorPositionModifierComponent.h
    Source/Editor/EditorReferenceShapeComponent.cpp
    Source/Editor/EditorReferenceShapeComponent.h
    Source/Editor/EditorRotationModifierComponent.cpp
    Source/Editor/EditorRotationModifierComponent.h
    Source/Editor/EditorScaleModifierComponent.cpp
    Source/Editor/EditorScaleModifierComponent.h
    Source/Editor/EditorShapeIntersectionFilterComponent.cpp
    Source/Editor/EditorShapeIntersectionFilterComponent.h
    Source/Editor/EditorSlopeAlignmentModifierComponent.cpp
    Source/Editor/EditorSlopeAlignmentModifierComponent.h
    Source/Editor/EditorSpawnerComponent.cpp
    Source/Editor/EditorSpawnerComponent.h
    Source/Editor/EditorSurfaceAltitudeFilterComponent.cpp
    Source/Editor/EditorSurfaceAltitudeFilterComponent.h
    Source/Editor/EditorSurfaceMaskDepthFilterComponent.cpp
    Source/Editor/EditorSurfaceMaskDepthFilterComponent.h
    Source/Editor/EditorSurfaceMaskFilterComponent.cpp
    Source/Editor/EditorSurfaceMaskFilterComponent.h
    Source/Editor/EditorSurfaceSlopeFilterComponent.cpp
    Source/Editor/EditorSurfaceSlopeFilterComponent.h
    Source/Debugger/EditorAreaDebugComponent.cpp
    Source/Debugger/EditorAreaDebugComponent.h
    Source/Debugger/EditorDebugComponent.cpp
    Source/Debugger/EditorDebugComponent.h
    Source/VegetationModule.cpp
    Source/VegetationModule.h
)
