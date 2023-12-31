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
    dllmain.cpp
    ComponentEntityEditorPlugin.h
    ComponentEntityEditorPlugin.cpp
    SandboxIntegration.h
    SandboxIntegration.cpp
    ComponentEntityEditorPlugin_precompiled.cpp
    ComponentEntityEditorPlugin_precompiled.h
    ComponentEntityDebugPrinter.h
    ComponentEntityDebugPrinter.cpp
    UI/ComponentEntityEditorOutlinerWindow.qrc
    UI/QComponentEntityEditorMainWindow.h
    UI/QComponentEntityEditorMainWindow.cpp
    UI/QComponentLevelEntityEditorMainWindow.h
    UI/QComponentLevelEntityEditorMainWindow.cpp
    UI/QComponentEntityEditorOutlinerWindow.h
    UI/QComponentEntityEditorOutlinerWindow.cpp
    UI/AssetCatalogModel.h
    UI/AssetCatalogModel.cpp
    UI/ComponentPalette/CategoriesList.h
    UI/ComponentPalette/CategoriesList.cpp
    UI/ComponentPalette/ComponentDataModel.h
    UI/ComponentPalette/ComponentDataModel.cpp
    UI/ComponentPalette/ComponentPaletteSettings.h
    UI/ComponentPalette/ComponentPaletteWindow.h
    UI/ComponentPalette/ComponentPaletteWindow.cpp
    UI/ComponentPalette/FavoriteComponentList.h
    UI/ComponentPalette/FavoriteComponentList.cpp
    UI/ComponentPalette/FilteredComponentList.h
    UI/ComponentPalette/FilteredComponentList.cpp
    UI/ComponentPalette/InformationPanel.h
    UI/ComponentPalette/InformationPanel.cpp
    UI/Outliner/OutlinerDisplayOptionsMenu.h
    UI/Outliner/OutlinerDisplayOptionsMenu.cpp
    UI/Outliner/OutlinerTreeView.hxx
    UI/Outliner/OutlinerTreeView.cpp
    UI/Outliner/OutlinerWidget.hxx
    UI/Outliner/OutlinerWidget.cpp
    UI/Outliner/OutlinerCacheBus.h
    UI/Outliner/OutlinerListModel.hxx
    UI/Outliner/OutlinerListModel.cpp
    UI/Outliner/OutlinerSearchWidget.h
    UI/Outliner/OutlinerSearchWidget.cpp
    UI/Outliner/OutlinerSortFilterProxyModel.hxx
    UI/Outliner/OutlinerSortFilterProxyModel.cpp
    UI/Outliner/OutlinerWidget.ui
    UI/Outliner/resources.qrc
    UI/Outliner/EntityOutliner.qss
    Objects/ComponentEntityObject.h
    Objects/ComponentEntityObject.cpp
)
