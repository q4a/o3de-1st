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

#include <AzFramework/StringFunc/StringFunc.h>
#include <AzFramework/Application/Application.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserEntry.h>
#include <AzToolsFramework/API/EditorPythonRunnerRequestsBus.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <AzToolsFramework/PythonTerminal/ScriptTermDialog.h>
#include <AtomToolsFramework/Util/Util.h>

#include <Atom/Document/MaterialDocumentRequestBus.h>
#include <Atom/Document/MaterialDocumentSystemRequestBus.h>
#include <Atom/Window/MaterialEditorWindowNotificationBus.h>
#include <Atom/Viewport/MaterialViewportRequestBus.h>

#include <Atom/RHI/Factory.h>

#include <Atom/RPI.Edit/Common/AssetUtils.h>
#include <Atom/RPI.Edit/Common/JsonUtils.h>
#include <Atom/RPI.Edit/Material/MaterialSourceData.h>
#include <Atom/RPI.Edit/Material/MaterialTypeSourceData.h>
#include <Atom/RPI.Reflect/Image/StreamingImageAsset.h>

#include <Viewport/MaterialViewportWidget.h>
#include <Viewport/MaterialViewportWidget.h>
#include <Window/MaterialEditorWindow.h>
#include <Window/MaterialTypesLibrary.h>
#include <Window/PerformanceMonitor/PerformanceMonitorWidget.h>
#include <Window/MaterialBrowserWidget.h>
#include <Window/MaterialInspector/MaterialInspector.h>
#include <Window/ViewportSettingsInspector/ViewportSettingsInspector.h>

AZ_PUSH_DISABLE_WARNING(4251 4800, "-Wunknown-warning-option") // disable warnings spawned by QT
#include <QCloseEvent>
#include <QVariant>
#include <QFileDialog>
#include <QWindow>
#include <QVBoxLayout>
AZ_POP_DISABLE_WARNING

namespace MaterialEditor
{
    MaterialEditorWindow::MaterialEditorWindow(QWidget* parent /* = 0 */)
        : AzQtComponents::DockMainWindow(parent)
    {
        QApplication::setWindowIcon(QIcon(":/Icons/materialtype.svg"));

        AZ::Name apiName = AZ::RHI::Factory::Get().GetName();
        if (!apiName.IsEmpty())
        {
            QString title = QString{ "Material Editor (%1)" }.arg(apiName.GetCStr());
            setWindowTitle(title);
        }
        else
        {
            AZ_Assert(false, "Render API name not found");
            setWindowTitle("Material Editor");
        }

        m_advancedDockManager = new AzQtComponents::FancyDocking(this);

        setDockNestingEnabled(true);
        setCorner(Qt::TopLeftCorner, Qt::LeftDockWidgetArea);
        setCorner(Qt::BottomLeftCorner, Qt::LeftDockWidgetArea);
        setCorner(Qt::TopRightCorner, Qt::RightDockWidgetArea);
        setCorner(Qt::BottomRightCorner, Qt::RightDockWidgetArea);

        m_menuBar = new QMenuBar(this);
        setMenuBar(m_menuBar);

        m_toolBar = new MaterialEditorToolBar(this);
        addToolBar(m_toolBar);

        m_centralWidget = new QWidget(this);
        m_tabWidget = new AzQtComponents::TabWidget(m_centralWidget);
        m_tabWidget->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
        m_tabWidget->setContentsMargins(0, 0, 0, 0);

        m_materialViewport = new MaterialViewportWidget(m_centralWidget);
        m_materialViewport->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);

        QVBoxLayout* vl = new QVBoxLayout(m_centralWidget);
        vl->setMargin(0);
        vl->setContentsMargins(0, 0, 0, 0);
        vl->addWidget(m_tabWidget);
        vl->addWidget(m_materialViewport);
        m_centralWidget->setLayout(vl);
        setCentralWidget(m_centralWidget);

        m_statusBar = new StatusBarWidget(this);
        this->statusBar()->addPermanentWidget(m_statusBar, 1);

        SetupMenu();
        SetupTabs();

        AddDockWidget("Asset Browser", new MaterialBrowserWidget, Qt::BottomDockWidgetArea, Qt::Vertical);
        AddDockWidget("Inspector", new MaterialInspector, Qt::RightDockWidgetArea, Qt::Horizontal);
        AddDockWidget("Viewport Settings", new ViewportSettingsInspector, Qt::LeftDockWidgetArea, Qt::Horizontal);
        AddDockWidget("Performance Monitor", new PerformanceMonitorWidget, Qt::RightDockWidgetArea, Qt::Horizontal);
        AddDockWidget("Python Terminal", new AzToolsFramework::CScriptTermDialog, Qt::BottomDockWidgetArea, Qt::Horizontal);

        SetDockWidgetVisible("Viewport Settings", false);
        SetDockWidgetVisible("Performance Monitor", false);
        SetDockWidgetVisible("Python Terminal", false);

        MaterialEditorWindowRequestBus::Handler::BusConnect();
        MaterialDocumentNotificationBus::Handler::BusConnect();
        OnDocumentOpened(AZ::Uuid::CreateNull());
    }

    MaterialEditorWindow::~MaterialEditorWindow()
    {
        MaterialDocumentNotificationBus::Handler::BusDisconnect();
        MaterialEditorWindowRequestBus::Handler::BusDisconnect();
    }

    bool MaterialEditorWindow::AddDockWidget(const AZStd::string& name, QWidget* widget, uint32_t area, uint32_t orientation)
    {
        auto dockWidgetItr = m_dockWidgets.find(name);
        if (dockWidgetItr != m_dockWidgets.end() || !widget)
        {
            return false;
        }

        auto dockWidget = new AzQtComponents::StyledDockWidget(name.c_str());
        dockWidget->setObjectName(name.c_str());
        dockWidget->setFeatures(QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetFloatable | QDockWidget::DockWidgetMovable);
        widget->setParent(dockWidget);
        widget->setMinimumSize(QSize(300, 300));
        dockWidget->setWidget(widget);
        addDockWidget(aznumeric_cast<Qt::DockWidgetArea>(area), dockWidget);
        resizeDocks({ dockWidget }, { 400 }, aznumeric_cast<Qt::Orientation>(orientation));
        m_dockWidgets[name] = dockWidget;
        return true;
    }

    void MaterialEditorWindow::RemoveDockWidget(const AZStd::string& name)
    {
        auto dockWidgetItr = m_dockWidgets.find(name);
        if (dockWidgetItr != m_dockWidgets.end())
        {
            delete dockWidgetItr->second;
            m_dockWidgets.erase(dockWidgetItr);
        }
    }

    void MaterialEditorWindow::SetDockWidgetVisible(const AZStd::string& name, bool visible)
    {
        auto dockWidgetItr = m_dockWidgets.find(name);
        if (dockWidgetItr != m_dockWidgets.end())
        {
            dockWidgetItr->second->setVisible(visible);
        }
    }

    bool MaterialEditorWindow::IsDockWidgetVisible(const AZStd::string& name) const
    {
        auto dockWidgetItr = m_dockWidgets.find(name);
        if (dockWidgetItr != m_dockWidgets.end())
        {
            return dockWidgetItr->second->isVisible();
        }
        return false;
    }

    AZStd::vector<AZStd::string> MaterialEditorWindow::GetDockWidgetNames() const
    {
        AZStd::vector<AZStd::string> names;
        names.reserve(m_dockWidgets.size());
        for (const auto& dockWidgetPair : m_dockWidgets)
        {
            names.push_back(dockWidgetPair.first);
        }
        return names;
    }

    void MaterialEditorWindow::ResizeViewportRenderTarget(uint32_t width, uint32_t height)
    {
        QSize requestedViewportSize = QSize(width, height) / devicePixelRatioF();
        QSize currentViewportSize = m_materialViewport->size();
        QSize offset = requestedViewportSize - currentViewportSize;
        QSize requestedWindowSize = size() + offset;
        resize(requestedWindowSize);

        AZ_Assert(m_materialViewport->size() == requestedViewportSize,
            "Resizing the window did not give the expected viewport size. Requested %d x %d but got %d x %d.",
            requestedViewportSize.width(), requestedViewportSize.height(),
            m_materialViewport->size().width(), m_materialViewport->size().height());

        QSize newDeviceSize = m_materialViewport->size();
        AZ_Warning("Material Editor", newDeviceSize.width() == width && newDeviceSize.height() == height,
            "Resizing the window did not give the expected frame size. Requested %d x %d but got %d x %d.",
            width, height,
            newDeviceSize.width(), newDeviceSize.height());
    }

    void MaterialEditorWindow::LockViewportRenderTargetSize(uint32_t width, uint32_t height)
    {
        m_materialViewport->LockRenderTargetSize(width, height);
    }

    void MaterialEditorWindow::UnlockViewportRenderTargetSize()
    {
        m_materialViewport->UnlockRenderTargetSize();
    }

    void MaterialEditorWindow::closeEvent(QCloseEvent* closeEvent)
    {
        bool didClose = true;
        MaterialDocumentSystemRequestBus::BroadcastResult(didClose, &MaterialDocumentSystemRequestBus::Events::CloseAllDocuments);
        if (!didClose)
        {
            closeEvent->ignore();
            return;
        }

        MaterialEditorWindowNotificationBus::Broadcast(&MaterialEditorWindowNotifications::OnMaterialEditorWindowClosing);
    }

    void MaterialEditorWindow::OnDocumentOpened(const AZ::Uuid& documentId)
    {
        bool isOpen = false;
        MaterialDocumentRequestBus::EventResult(isOpen, documentId, &MaterialDocumentRequestBus::Events::IsOpen);
        bool isSavable = false;
        MaterialDocumentRequestBus::EventResult(isSavable, documentId, &MaterialDocumentRequestBus::Events::IsSavable);
        bool canUndo = false;
        MaterialDocumentRequestBus::EventResult(canUndo, documentId, &MaterialDocumentRequestBus::Events::CanUndo);
        bool canRedo = false;
        MaterialDocumentRequestBus::EventResult(canRedo, documentId, &MaterialDocumentRequestBus::Events::CanRedo);

        // Update UI to display the new document
        AddTabForDocumentId(documentId);
        UpdateTabForDocumentId(documentId);

        const bool hasTabs = m_tabWidget->count() > 0;

        // Update menu options
        m_actionNewFromOther->setEnabled(true);
        m_actionOpen->setEnabled(true);
        m_actionOpenRecent->setEnabled(false);
        m_actionClose->setEnabled(hasTabs);
        m_actionCloseAll->setEnabled(hasTabs);
        m_actionCloseOthers->setEnabled(hasTabs);

        m_actionSave->setEnabled(isOpen && isSavable);
        m_actionSaveAsCopy->setEnabled(isOpen && isSavable);
        m_actionSaveAsChild->setEnabled(isOpen);
        m_actionSaveAll->setEnabled(hasTabs);

        m_actionExit->setEnabled(true);

        m_actionUndo->setEnabled(canUndo);
        m_actionRedo->setEnabled(canRedo);
        m_actionPreferences->setEnabled(false);

        m_actionAssetBrowser->setEnabled(true);
        m_actionInspector->setEnabled(true);
        m_actionConsole->setEnabled(false);
        m_actionPythonTerminal->setEnabled(true);
        m_actionPerfMonitor->setEnabled(true);
        m_actionViewportSettings->setEnabled(true);
        m_actionPreviousTab->setEnabled(m_tabWidget->count() > 1);
        m_actionNextTab->setEnabled(m_tabWidget->count() > 1);

        m_actionHelp->setEnabled(false);
        m_actionAbout->setEnabled(false);

        activateWindow();
        raise();

        const QString documentPath = GetDocumentPath(documentId);
        if (!documentPath.isEmpty())
        {
            m_statusBar->UpdateStatusInfo(QString("Material opened: %1").arg(documentPath));
        }
    }

    void MaterialEditorWindow::OnDocumentClosed(const AZ::Uuid& documentId)
    {
        RemoveTabForDocumentId(documentId);

        const QString documentPath = GetDocumentPath(documentId);
        m_statusBar->UpdateStatusInfo(QString("Material closed: %1").arg(documentPath));
    }

    void MaterialEditorWindow::OnDocumentModified(const AZ::Uuid& documentId)
    {
        UpdateTabForDocumentId(documentId);
    }

    void MaterialEditorWindow::OnDocumentUndoStateChanged(const AZ::Uuid& documentId)
    {
        if (documentId == GetDocumentIdFromTab(m_tabWidget->currentIndex()))
        {
            bool canUndo = false;
            MaterialDocumentRequestBus::EventResult(canUndo, documentId, &MaterialDocumentRequestBus::Events::CanUndo);
            bool canRedo = false;
            MaterialDocumentRequestBus::EventResult(canRedo, documentId, &MaterialDocumentRequestBus::Events::CanRedo);
            m_actionUndo->setEnabled(canUndo);
            m_actionRedo->setEnabled(canRedo);
        }
    }

    void MaterialEditorWindow::OnDocumentSaved(const AZ::Uuid& documentId)
    {
        UpdateTabForDocumentId(documentId);

        const QString documentPath = GetDocumentPath(documentId);
        m_statusBar->UpdateStatusInfo(QString("Material saved: %1").arg(documentPath));
    }

    void MaterialEditorWindow::SetupMenu()
    {
        // Generating the main menu manually because it's easier and we will have some dynamic or data driven entries
        m_menuFile = m_menuBar->addMenu("&File");

        SetupMenu_New();

        m_actionOpen = m_menuFile->addAction("&Open...", [this]() {
            const AZStd::vector<AZ::Data::AssetType> assetTypes = { azrtti_typeid<AZ::RPI::MaterialAsset>() };
            const AZStd::string filePath = AtomToolsFramework::GetOpenFileInfo(assetTypes).absoluteFilePath().toUtf8().constData();
            if (!filePath.empty())
            {
                MaterialDocumentSystemRequestBus::Broadcast(&MaterialDocumentSystemRequestBus::Events::OpenDocument, filePath);
            }
        }, QKeySequence::Open);

        m_actionOpenRecent = m_menuFile->addAction("Open &Recent");

        m_menuFile->addSeparator();

        m_actionSave = m_menuFile->addAction("&Save", [this]() {
            const AZ::Uuid documentId = GetDocumentIdFromTab(m_tabWidget->currentIndex());
            bool result = false;
            MaterialDocumentSystemRequestBus::BroadcastResult(result, &MaterialDocumentSystemRequestBus::Events::SaveDocument, documentId);
            if (!result)
            {
                const QString documentPath = GetDocumentPath(documentId);
                m_statusBar->UpdateStatusError(QString("Failed to save material: %1").arg(documentPath));
            }
        }, QKeySequence::Save);

        m_actionSaveAsCopy = m_menuFile->addAction("Save &As...", [this]() {
            const AZ::Uuid documentId = GetDocumentIdFromTab(m_tabWidget->currentIndex());
            const QString documentPath = GetDocumentPath(documentId);

            bool result = false;
            MaterialDocumentSystemRequestBus::BroadcastResult(result, &MaterialDocumentSystemRequestBus::Events::SaveDocumentAsCopy,
                documentId, AtomToolsFramework::GetSaveFileInfo(documentPath).absoluteFilePath().toUtf8().constData());
            if (!result)
            {
                m_statusBar->UpdateStatusError(QString("Failed to save material: %1").arg(documentPath));
            }
        }, QKeySequence::SaveAs);

        m_actionSaveAsChild = m_menuFile->addAction("Save As &Child...", [this]() {
            const AZ::Uuid documentId = GetDocumentIdFromTab(m_tabWidget->currentIndex());
            const QString documentPath = GetDocumentPath(documentId);

            bool result = false;
            MaterialDocumentSystemRequestBus::BroadcastResult(result, &MaterialDocumentSystemRequestBus::Events::SaveDocumentAsChild,
                documentId, AtomToolsFramework::GetSaveFileInfo(documentPath).absoluteFilePath().toUtf8().constData());
            if (!result)
            {
                m_statusBar->UpdateStatusError(QString("Failed to save material: %1").arg(documentPath));
            }
        });

        m_actionSaveAll = m_menuFile->addAction("Save A&ll", [this]() {
            bool result = false;
            MaterialDocumentSystemRequestBus::BroadcastResult(result, &MaterialDocumentSystemRequestBus::Events::SaveAllDocuments);
            if (!result)
            {
                m_statusBar->UpdateStatusError(QString("Failed to save materials."));
            }
        });

        m_menuFile->addSeparator();

        m_actionClose = m_menuFile->addAction("&Close", [this]() {
            const AZ::Uuid documentId = GetDocumentIdFromTab(m_tabWidget->currentIndex());
            MaterialDocumentSystemRequestBus::Broadcast(&MaterialDocumentSystemRequestBus::Events::CloseDocument, documentId);
        }, QKeySequence::Close);

        m_actionCloseAll = m_menuFile->addAction("Close All", [this]() {
            MaterialDocumentSystemRequestBus::Broadcast(&MaterialDocumentSystemRequestBus::Events::CloseAllDocuments);
        });

        m_actionCloseOthers = m_menuFile->addAction("Close Others", [this]() {
            const AZ::Uuid documentId = GetDocumentIdFromTab(m_tabWidget->currentIndex());
            MaterialDocumentSystemRequestBus::Broadcast(&MaterialDocumentSystemRequestBus::Events::CloseAllDocumentsExcept, documentId);
        });

        m_menuFile->addSeparator();

        auto presetMenu = m_menuFile->addMenu("Presets");

        presetMenu->addAction("New Model Preset", [this]() {
            AZ::Render::ModelPresetPtr preset;
            MaterialViewportRequestBus::BroadcastResult(preset, &MaterialViewportRequestBus::Events::AddModelPreset, AZ::Render::ModelPreset());
            MaterialViewportRequestBus::Broadcast(&MaterialViewportRequestBus::Events::SelectModelPreset, preset);
        });

        presetMenu->addAction("Save Model Preset", [this]() {
            const QString defaultPath = AtomToolsFramework::GetUniqueFileInfo(
                QString(AZ::IO::FileIOBase::GetInstance()->GetAlias("@devassets@")) +
                AZ_CORRECT_FILESYSTEM_SEPARATOR + "Materials" +
                AZ_CORRECT_FILESYSTEM_SEPARATOR + "untitled.modelpreset.azasset").absoluteFilePath();

            MaterialViewportRequestBus::Broadcast(&MaterialViewportRequestBus::Events::SaveModelPresetSelection,
                AtomToolsFramework::GetSaveFileInfo(defaultPath).absoluteFilePath().toUtf8().constData());
        });

        presetMenu->addSeparator();

        presetMenu->addAction("New Lighting Preset", [this]() {
            AZ::Render::LightingPresetPtr preset;
            MaterialViewportRequestBus::BroadcastResult(preset, &MaterialViewportRequestBus::Events::AddLightingPreset, AZ::Render::LightingPreset());
            MaterialViewportRequestBus::Broadcast(&MaterialViewportRequestBus::Events::SelectLightingPreset, preset);
        });

        presetMenu->addAction("Save Lighting Preset", [this]() {
            const QString defaultPath = AtomToolsFramework::GetUniqueFileInfo(
                QString(AZ::IO::FileIOBase::GetInstance()->GetAlias("@devassets@")) +
                AZ_CORRECT_FILESYSTEM_SEPARATOR + "Materials" +
                AZ_CORRECT_FILESYSTEM_SEPARATOR + "untitled.lightingpreset.azasset").absoluteFilePath();

            MaterialViewportRequestBus::Broadcast(&MaterialViewportRequestBus::Events::SaveLightingPresetSelection,
                AtomToolsFramework::GetSaveFileInfo(defaultPath).absoluteFilePath().toUtf8().constData());
        });

        presetMenu->addSeparator();

        presetMenu->addAction("Reload Presets", [this]() {
            MaterialViewportRequestBus::Broadcast(&MaterialViewportRequestBus::Events::ReloadContent);
        }, QKeySequence::Refresh);

        m_menuFile->addSeparator();

        m_menuFile->addAction("Run &Python...", [this]() {
            const QString script = QFileDialog::getOpenFileName(this, "Run Script", QString(), QString("*.py"));
            if (!script.isEmpty())
            {
                AzToolsFramework::EditorPythonRunnerRequestBus::Broadcast(&AzToolsFramework::EditorPythonRunnerRequestBus::Events::ExecuteByFilename, script.toUtf8().constData());
            }
        });

        m_menuFile->addSeparator();

        m_actionExit = m_menuFile->addAction("E&xit", [this]() {
            close();
        }, QKeySequence::Quit);

        m_menuEdit = m_menuBar->addMenu("&Edit");

        m_actionUndo = m_menuEdit->addAction("&Undo", [this]() {
            const AZ::Uuid documentId = GetDocumentIdFromTab(m_tabWidget->currentIndex());
            bool result = false;
            MaterialDocumentRequestBus::EventResult(result, documentId, &MaterialDocumentRequestBus::Events::Undo);
            if (!result)
            {
                const QString documentPath = GetDocumentPath(documentId);
                m_statusBar->UpdateStatusError(QString("Failed to perform Undo in material: %1").arg(documentPath));
            }
        }, QKeySequence::Undo);

        m_actionRedo = m_menuEdit->addAction("&Redo", [this]() {
            const AZ::Uuid documentId = GetDocumentIdFromTab(m_tabWidget->currentIndex());
            bool result = false;
            MaterialDocumentRequestBus::EventResult(result, documentId, &MaterialDocumentRequestBus::Events::Redo);
            if (!result)
            {
                const QString documentPath = GetDocumentPath(documentId);
                m_statusBar->UpdateStatusError(QString("Failed to perform Undo in material: %1").arg(documentPath));
            }
        }, QKeySequence::Redo);

        m_menuEdit->addSeparator();

        m_actionPreferences = m_menuEdit->addAction("&Preferences...", [this]() {
        }, QKeySequence::Preferences);
        m_actionPreferences->setEnabled(false);

        m_menuView = m_menuBar->addMenu("&View");

        m_actionAssetBrowser = m_menuView->addAction("&Asset Browser", [this]() {
            const AZStd::string label = "Asset Browser";
            SetDockWidgetVisible(label, !IsDockWidgetVisible(label));
        });

        m_actionInspector = m_menuView->addAction("&Inspector", [this]() {
            const AZStd::string label = "Inspector";
            SetDockWidgetVisible(label, !IsDockWidgetVisible(label));
        });

        m_actionConsole = m_menuView->addAction("&Console", [this]() {
        });

        m_actionPythonTerminal = m_menuView->addAction("Python &Terminal", [this]() {
            const AZStd::string label = "Python Terminal";
            SetDockWidgetVisible(label, !IsDockWidgetVisible(label));
        });

        m_actionPerfMonitor = m_menuView->addAction("Performance &Monitor", [this]() {
            const AZStd::string label = "Performance Monitor";
            SetDockWidgetVisible(label, !IsDockWidgetVisible(label));
        });

        m_actionViewportSettings = m_menuView->addAction("Viewport Settings", [this]() {
            const AZStd::string label = "Viewport Settings";
            SetDockWidgetVisible(label, !IsDockWidgetVisible(label));
        });

        m_menuView->addSeparator();

        m_actionPreviousTab = m_menuView->addAction("&Previous Tab", [this]() {
            SelectPreviousTab();
        }, Qt::CTRL | Qt::SHIFT | Qt::Key_Tab); //QKeySequence::PreviousChild is mapped incorrectly in Qt

        m_actionNextTab = m_menuView->addAction("&Next Tab", [this]() {
            SelectNextTab();
        }, Qt::CTRL | Qt::Key_Tab); //QKeySequence::NextChild works as expected but mirroring Previous

        m_menuHelp = m_menuBar->addMenu("&Help");

        m_actionHelp = m_menuHelp->addAction("&Help...", [this]() {
        });

        m_actionAbout = m_menuHelp->addAction("&About...", [this]() {
        });
    }

    void MaterialEditorWindow::SetupMenu_New()
    {
        m_menuNew = m_menuFile->addMenu("&New");

        // Enumerate and load all the relevant config files in the project.
        // (The files are stored in a temporary list instead of processed in the callback because deep operations inside
        // AssetCatalogRequestBus::EnumerateAssets can lead to deadlocked)
        AZStd::list<AZ::Data::AssetInfo> materialTypeAssetInfoList;
        AZ::Data::AssetCatalogRequests::AssetEnumerationCB enumerateCB = [&materialTypeAssetInfoList]([[maybe_unused]] const AZ::Data::AssetId id, const AZ::Data::AssetInfo& info)
        {
            if (AzFramework::StringFunc::EndsWith(info.m_relativePath.c_str(), ".materialtypes.json"))
            {
                materialTypeAssetInfoList.push_front(info);
            }
        };

        AZ::Data::AssetCatalogRequestBus::Broadcast(&AZ::Data::AssetCatalogRequestBus::Events::EnumerateAssets, nullptr, enumerateCB, nullptr);

        AZStd::vector<MaterialTypeEntry> materialTypeEntries;
        for (const auto& info : materialTypeAssetInfoList)
        {
            const AZStd::string& sourcePath = AZ::RPI::AssetUtils::GetSourcePathByAssetId(info.m_assetId);
            if (!sourcePath.empty())
            {
                MaterialTypesLibrary library;
                if (AZ::RPI::JsonUtils::LoadObjectFromFile(sourcePath, library))
                {
                    for (const auto& entry : library.m_entries)
                    {
                        materialTypeEntries.push_back(entry);
                    }
                    AZ_TracePrintf("Material Editor", "Loaded materialType library: %s.\n", info.m_relativePath.c_str());
                }
            }
        }

        const char* engineRoot = nullptr;
        AzFramework::ApplicationRequests::Bus::BroadcastResult(engineRoot, &AzFramework::ApplicationRequests::GetEngineRoot);
        AZ_Assert(engineRoot, "Engine Root not initialized");

        // Generate the menu actions for creating new materials from types
        for (const auto& materialTypeEntry : materialTypeEntries)
        {
            AZStd::string absoluteMaterialTypePath;
            AZ::StringFunc::Path::Join(engineRoot, materialTypeEntry.m_filePath.c_str(), absoluteMaterialTypePath);

            m_menuNew->addAction(materialTypeEntry.m_displayName.c_str(), [this, absoluteMaterialTypePath]() {
                const QString defaultPath = AtomToolsFramework::GetUniqueFileInfo(
                    QString(AZ::IO::FileIOBase::GetInstance()->GetAlias("@devassets@")) +
                    AZ_CORRECT_FILESYSTEM_SEPARATOR + "Materials" +
                    AZ_CORRECT_FILESYSTEM_SEPARATOR + "untitled." +
                    AZ::RPI::MaterialSourceData::Extension).absoluteFilePath();

                MaterialDocumentSystemRequestBus::Broadcast(&MaterialDocumentSystemRequestBus::Events::CreateDocumentFromFile,
                    absoluteMaterialTypePath, AtomToolsFramework::GetSaveFileInfo(defaultPath).absoluteFilePath().toUtf8().constData());
            });
        }

        m_menuNew->addSeparator();

        // Add the final entry that allows selection of any source file
        m_actionNewFromOther = m_menuNew->addAction("Other &Type...", [this]() {
            // [GFX TODO] This is functional but UI is not as designed
            // This will open the parent material or type then save it as a child material
            // if the operation fails the material document is closed/destroyed
            const AZStd::vector<AZ::Data::AssetType> assetTypes = {
                azrtti_typeid<AZ::RPI::MaterialTypeAsset>(),
            };

            const QString& sourcePath = AtomToolsFramework::GetOpenFileInfo(assetTypes).absoluteFilePath();
            if (!sourcePath.isEmpty())
            {
                const QString defaultPath = AtomToolsFramework::GetUniqueFileInfo(
                    QString(AZ::IO::FileIOBase::GetInstance()->GetAlias("@devassets@")) +
                    AZ_CORRECT_FILESYSTEM_SEPARATOR + "Materials" +
                    AZ_CORRECT_FILESYSTEM_SEPARATOR + "untitled." +
                    AZ::RPI::MaterialSourceData::Extension).absoluteFilePath();

                const QString& targetPath = AtomToolsFramework::GetSaveFileInfo(defaultPath).absoluteFilePath();
                if (!targetPath.isEmpty())
                {
                    MaterialDocumentSystemRequestBus::Broadcast(&MaterialDocumentSystemRequestBus::Events::CreateDocumentFromFile,
                        sourcePath.toUtf8().constData(), targetPath.toUtf8().constData());
                }
            }
        }, QKeySequence::New);
    }

    void MaterialEditorWindow::SetupTabs()
    {
        // The tab bar should only be visible if it has active documents
        m_tabWidget->setVisible(false);
        m_tabWidget->setTabBarAutoHide(false);
        m_tabWidget->setMovable(true);
        m_tabWidget->setTabsClosable(true);
        m_tabWidget->setUsesScrollButtons(true);

        // Add context menu for right-clicking on tabs
        m_tabWidget->setContextMenuPolicy(Qt::ContextMenuPolicy::CustomContextMenu);
        connect(m_tabWidget, &QWidget::customContextMenuRequested, this, [this]() {
            OpenTabContextMenu();
        });

        // This signal will be triggered whenever a tab is added, removed, selected, clicked, dragged
        // When the last tab is removed tabIndex will be -1 and the document ID will be null
        // This should automatically clear the active document
        connect(m_tabWidget, &QTabWidget::currentChanged, this, [this](int tabIndex) {
            const AZ::Uuid documentId = GetDocumentIdFromTab(tabIndex);
            MaterialDocumentNotificationBus::Broadcast(&MaterialDocumentNotificationBus::Events::OnDocumentOpened, documentId);
        });

        connect(m_tabWidget, &QTabWidget::tabCloseRequested, this, [this](int tabIndex) {
            const AZ::Uuid documentId = GetDocumentIdFromTab(tabIndex);
            MaterialDocumentSystemRequestBus::Broadcast(&MaterialDocumentSystemRequestBus::Events::CloseDocument, documentId);
        });
    }

    void MaterialEditorWindow::AddTabForDocumentId(const AZ::Uuid& documentId)
    {
        bool isOpen = false;
        MaterialDocumentRequestBus::EventResult(isOpen, documentId, &MaterialDocumentRequestBus::Events::IsOpen);

        if (documentId.IsNull() || !isOpen)
        {
            return;
        }

        // Blocking signals from the tab bar so the currentChanged signal is not sent while a document is already being opened.
        // This prevents the OnDocumentOpened notification from being sent recursively.
        const QSignalBlocker blocker(m_tabWidget);

        // If a tab for this document already exists then select it instead of creating a new one
        for (int tabIndex = 0; tabIndex < m_tabWidget->count(); ++tabIndex)
        {
            if (documentId == GetDocumentIdFromTab(tabIndex))
            {
                m_tabWidget->setCurrentIndex(tabIndex);
                m_tabWidget->repaint();
                return;
            }
        }

        // Create a new tab for the document ID and assign it's label to the file name of the document.
        AZStd::string absolutePath;
        MaterialDocumentRequestBus::EventResult(absolutePath, documentId, &MaterialDocumentRequestBus::Events::GetAbsolutePath);

        AZStd::string filename;
        AzFramework::StringFunc::Path::GetFullFileName(absolutePath.c_str(), filename);

        // The tab widget requires a dummy page per tab
        QWidget* placeHolderWidget = new QWidget(m_centralWidget);
        placeHolderWidget->setContentsMargins(0, 0, 0, 0);
        placeHolderWidget->resize(0, 0);
        placeHolderWidget->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

        const int tabIndex = m_tabWidget->addTab(placeHolderWidget, filename.c_str());

        // The user can manually reorder tabs which will invalidate any association by index.
        // We need to store the document ID with the tab using the tab instead of a separate mapping.
        m_tabWidget->tabBar()->setTabData(tabIndex, QVariant(documentId.ToString<QString>()));
        m_tabWidget->setTabToolTip(tabIndex, absolutePath.c_str());
        m_tabWidget->setCurrentIndex(tabIndex);
        m_tabWidget->setVisible(true);
        m_tabWidget->repaint();
    }

    void MaterialEditorWindow::RemoveTabForDocumentId(const AZ::Uuid& documentId)
    {
        // We are not blocking signals here because we want closing tabs to close the associated document
        // and automatically select the next document. 
        for (int tabIndex = 0; tabIndex < m_tabWidget->count(); ++tabIndex)
        {
            if (documentId == GetDocumentIdFromTab(tabIndex))
            {
                m_tabWidget->removeTab(tabIndex);
                m_tabWidget->setVisible(m_tabWidget->count() > 0);
                m_tabWidget->repaint();
                break;
            }
        }
    }

    void MaterialEditorWindow::UpdateTabForDocumentId(const AZ::Uuid& documentId)
    {
        // Whenever a document is opened, saved, or modified we need to update the tab label
        if (!documentId.IsNull())
        {
            // Because tab order and indexes can change from user interactions, we cannot store a map
            // between a tab index and document ID.
            // We must iterate over all of the tabs to find the one associated with this document.
            for (int tabIndex = 0; tabIndex < m_tabWidget->count(); ++tabIndex)
            {
                if (documentId == GetDocumentIdFromTab(tabIndex))
                {
                    AZStd::string absolutePath;
                    MaterialDocumentRequestBus::EventResult(absolutePath, documentId, &MaterialDocumentRequestBus::Events::GetAbsolutePath);

                    AZStd::string filename;
                    AzFramework::StringFunc::Path::GetFullFileName(absolutePath.c_str(), filename);

                    bool isModified = false;
                    MaterialDocumentRequestBus::EventResult(isModified, documentId, &MaterialDocumentRequestBus::Events::IsModified);

                    // We use an asterisk appended to the file name to denote modified document
                    if (isModified)
                    {
                        filename += " *";
                    }

                    m_tabWidget->setTabText(tabIndex, filename.c_str());
                    m_tabWidget->setTabToolTip(tabIndex, absolutePath.c_str());
                    m_tabWidget->repaint();
                    break;
                }
            }
        }
    }

    QString MaterialEditorWindow::GetDocumentPath(const AZ::Uuid& documentId) const
    {
        AZStd::string absolutePath;
        MaterialDocumentRequestBus::EventResult(absolutePath, documentId, &MaterialDocumentRequestBus::Handler::GetAbsolutePath);
        return absolutePath.c_str();
    }

    AZ::Uuid MaterialEditorWindow::GetDocumentIdFromTab(const int tabIndex) const
    {
        const QVariant tabData = m_tabWidget->tabBar()->tabData(tabIndex);
        if (!tabData.isNull())
        {
            // We need to be able to convert between a UUID and a string to store and retrieve a document ID from the tab bar
            const QString documentIdString = tabData.toString();
            const QByteArray documentIdBytes = documentIdString.toUtf8();
            const AZ::Uuid documentId(documentIdBytes.data(), documentIdBytes.size());
            return documentId;
        }
        return AZ::Uuid::CreateNull();
    }

    void MaterialEditorWindow::OpenTabContextMenu()
    {
        const QTabBar* tabBar = m_tabWidget->tabBar();
        const QPoint position = tabBar->mapFromGlobal(QCursor::pos());
        const int clickedTabIndex = tabBar->tabAt(position);
        const int currentTabIndex = tabBar->currentIndex();
        if (clickedTabIndex >= 0)
        {
            QMenu tabMenu;
            const QString selectActionName = (currentTabIndex == clickedTabIndex) ? "Select in Browser" : "Select";
            tabMenu.addAction(selectActionName, [this, clickedTabIndex]() {
                const AZ::Uuid documentId = GetDocumentIdFromTab(clickedTabIndex);
                MaterialDocumentNotificationBus::Broadcast(&MaterialDocumentNotificationBus::Events::OnDocumentOpened, documentId);
            });
            tabMenu.addAction("Close", [this, clickedTabIndex]() {
                const AZ::Uuid documentId = GetDocumentIdFromTab(clickedTabIndex);
                MaterialDocumentSystemRequestBus::Broadcast(&MaterialDocumentSystemRequestBus::Events::CloseDocument, documentId);
            });
            auto closeOthersAction = tabMenu.addAction("Close Others", [this, clickedTabIndex]() {
                const AZ::Uuid documentId = GetDocumentIdFromTab(clickedTabIndex);
                MaterialDocumentSystemRequestBus::Broadcast(&MaterialDocumentSystemRequestBus::Events::CloseAllDocumentsExcept, documentId);
            });
            closeOthersAction->setEnabled(tabBar->count() > 1);
            tabMenu.exec(QCursor::pos());
        }
    }

    void MaterialEditorWindow::SelectPreviousTab()
    {
        if (m_tabWidget->count() > 1)
        {
            // Adding count to wrap around when index <= 0
            m_tabWidget->setCurrentIndex((m_tabWidget->currentIndex() + m_tabWidget->count() - 1) % m_tabWidget->count());
        }
    }

    void MaterialEditorWindow::SelectNextTab()
    {
        if (m_tabWidget->count() > 1)
        {
            m_tabWidget->setCurrentIndex((m_tabWidget->currentIndex() + 1) % m_tabWidget->count());
        }
    }
} // namespace MaterialEditor

#include <Window/moc_MaterialEditorWindow.cpp>
