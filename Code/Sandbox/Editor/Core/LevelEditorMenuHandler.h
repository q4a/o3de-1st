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

#ifndef LEVELEDITORMENUHANDLER_H
#define LEVELEDITORMENUHANDLER_H

#if !defined(Q_MOC_RUN)
#include <QObject>
#include <QString>
#include <QMenu>
#include <QPointer>
#include "ActionManager.h"
#include "QtViewPaneManager.h"
#include <AzToolsFramework/ComponentMode/EditorComponentModeBus.h>
#endif

class MainWindow;
class QtViewPaneManager;
class QSettings;
struct QtViewPane;

class LevelEditorMenuHandler
    : public QObject
    , private AzToolsFramework::ComponentModeFramework::EditorComponentModeNotificationBus::Handler
    , private AzToolsFramework::EditorMenuRequestBus::Handler
{
    Q_OBJECT
public:
    LevelEditorMenuHandler(MainWindow* mainWindow, QtViewPaneManager* const viewPaneManager, QSettings& settings);
    ~LevelEditorMenuHandler();

    void Initialize();

    static bool MRUEntryIsValid(const QString& entry, const QString& gameFolderPath);

    void IncrementViewPaneVersion();
    int GetViewPaneVersion() const;

    void UpdateViewLayoutsMenu(ActionManager::MenuWrapper& layoutsMenu);

    void ResetToolsMenus();

    QAction* CreateViewPaneAction(const QtViewPane* view);

    // It's used when users update the Tool Box Macro list in the Configure Tool Box Macro dialog
    void UpdateMacrosMenu();

Q_SIGNALS:
    void ActivateAssetImporter();

private:
    QMenu* CreateFileMenu();
    QMenu* CreateEditMenu();
    void PopulateEditMenu(ActionManager::MenuWrapper& editMenu);
    QMenu* CreateGameMenu();
    QMenu* CreateToolsMenu();
    QMenu* CreateViewMenu();
    QMenu* CreateHelpMenu();

    void checkOrOpenView();

    QMap<QString, QList<QtViewPane*>> CreateMenuMap(QMap<QString, QList<QtViewPane*>>& menuMap, QtViewPanes& allRegisteredViewPanes);
    void CreateMenuOptions(QMap<QString, QList<QtViewPane*>>* menuMap, ActionManager::MenuWrapper& menu, const char* category);

    void CreateDebuggingSubMenu(ActionManager::MenuWrapper gameMenu);

    void UpdateMRUFiles();

    void ClearAll();
    void ToggleSelection(bool hide);
    void ShowLastHidden();
    void OnUpdateOpenRecent();

    void OnUpdateMacrosMenu();

    void UpdateOpenViewPaneMenu();

    QAction* CreateViewPaneMenuItem(ActionManager* actionManager, ActionManager::MenuWrapper& menu, const QtViewPane* view);

    void InitializeViewPaneMenu(ActionManager* actionManager, ActionManager::MenuWrapper& menu, AZStd::function < bool(const QtViewPane& view)> functor);

    void LoadComponentLayout();

    void AddDisableActionInSimModeListener(QAction* action);

    // EditorComponentModeNotificationBus
    void EnteredComponentMode(const AZStd::vector<AZ::Uuid>& componentModeTypes) override;
    void LeftComponentMode(const AZStd::vector<AZ::Uuid>& componentModeTypes) override;

    // EditorMenuRequestBus
    void AddEditMenuAction(QAction* action) override;
    void AddMenuAction(AZStd::string_view categoryId, QAction* action) override;
    void RestoreEditMenuToDefault() override;

    MainWindow* m_mainWindow;
    ActionManager* m_actionManager;
    QtViewPaneManager* m_viewPaneManager;

    QPointer<QMenu> m_viewportViewsMenu;

    ActionManager::MenuWrapper m_toolsMenu;

    QMenu* m_mostRecentLevelsMenu = nullptr;
    QMenu* m_mostRecentProjectsMenu = nullptr;
    QMenu* m_editmenu = nullptr;

    ActionManager::MenuWrapper m_viewPanesMenu;
    ActionManager::MenuWrapper m_layoutsMenu;
    ActionManager::MenuWrapper m_macrosMenu;

    int m_viewPaneVersion = 0;

    QList<QMenu*> m_topLevelMenus;
    QSettings& m_settings;
};

#endif // LEVELEDITORMENUHANDLER_H
