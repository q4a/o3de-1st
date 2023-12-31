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
    Include/Atom/Viewport/InputController/MaterialEditorViewportInputControllerBus.h
    Include/Atom/Viewport/MaterialViewportModule.h
    Include/Atom/Viewport/MaterialViewportRequestBus.h
    Include/Atom/Viewport/MaterialViewportNotificationBus.h
    Include/Atom/Viewport/PerformanceMetrics.h
    Include/Atom/Viewport/PerformanceMonitorRequestBus.h
    Source/Viewport/InputController/MaterialEditorViewportInputController.cpp
    Source/Viewport/InputController/MaterialEditorViewportInputController.h
    Source/Viewport/InputController/Behavior.cpp
    Source/Viewport/InputController/Behavior.h
    Source/Viewport/InputController/DollyCameraBehavior.cpp
    Source/Viewport/InputController/DollyCameraBehavior.h
    Source/Viewport/InputController/IdleBehavior.cpp
    Source/Viewport/InputController/IdleBehavior.h
    Source/Viewport/InputController/MoveCameraBehavior.cpp
    Source/Viewport/InputController/MoveCameraBehavior.h
    Source/Viewport/InputController/PanCameraBehavior.cpp
    Source/Viewport/InputController/PanCameraBehavior.h
    Source/Viewport/InputController/OrbitCameraBehavior.cpp
    Source/Viewport/InputController/OrbitCameraBehavior.h
    Source/Viewport/InputController/RotateEnvironmentBehavior.cpp
    Source/Viewport/InputController/RotateEnvironmentBehavior.h
    Source/Viewport/InputController/RotateModelBehavior.cpp
    Source/Viewport/InputController/RotateModelBehavior.h
    Source/Viewport/MaterialViewportModule.cpp
    Source/Viewport/MaterialViewportComponent.cpp
    Source/Viewport/MaterialViewportComponent.h
    Source/Viewport/MaterialViewportWidget.cpp
    Source/Viewport/MaterialViewportWidget.h
    Source/Viewport/MaterialViewportWidget.ui
    Source/Viewport/MaterialViewportRenderer.cpp
    Source/Viewport/MaterialViewportRenderer.h
    Source/Viewport/PerformanceMonitorComponent.cpp
    Source/Viewport/PerformanceMonitorComponent.h
)
