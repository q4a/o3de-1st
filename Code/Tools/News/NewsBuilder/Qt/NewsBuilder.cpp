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

#include <AzCore/PlatformDef.h>
#include "NewsBuilder.h"
#include "Qt/ArticleDetails.h"
#include "Qt/BuilderArticleViewContainer.h"
#include "NewsShared/Qt/ArticleViewContainer.h"
#include "NewsShared/ResourceManagement/Resource.h"
#include "ResourceManagement/BuilderResourceManifest.h"
#include "ArticleDetailsContainer.h"
#include "LogContainer.h"
#include "EndpointManagerView.h"
#include "EndpointManager.h"
#include "Qt/QCustomMessageBox.h"

#include <AzQtComponents/Components/StyleManager.h>
#include <AzQtComponents/Utilities/QtPluginPaths.h>

AZ_PUSH_DISABLE_WARNING(4251 4996, "-Wunknown-warning-option")
#include <aws/core/utils/HashingUtils.h>
AZ_POP_DISABLE_WARNING

#include <QImage>
#include <QFileDialog>
#include <QSignalMapper>
#include <QTextDocument>

#include "Qt/ui_Newsbuilder.h"

namespace News
{

    NewsBuilder::NewsBuilder(QWidget* parent)
        : QMainWindow(parent)
        , m_ui(new Ui::NewsBuilderClass())
        , m_manifest(new BuilderResourceManifest(
            std::bind(&NewsBuilder::SyncSuccess, this),
            std::bind(&NewsBuilder::SyncFail, this, std::placeholders::_1),
            std::bind(&NewsBuilder::SyncUpdate, this, std::placeholders::_1, std::placeholders::_2)))
        , m_articleDetailsContainer(new ArticleDetailsContainer(this, *m_manifest))
        , m_articleViewContainer(new BuilderArticleViewContainer(this, *m_manifest))
        , m_logContainer(new LogContainer(this))
    {
        AzQtComponents::StyleManager* m_styleSheet = new AzQtComponents::StyleManager(this);
        m_styleSheet->initialize(qApp);

        m_ui->setupUi(this);

        QDir rootDir(AzQtComponents::FindEngineRootDir(qApp));
        const auto pathOnDisk = rootDir.absoluteFilePath("Code/Tools/News/NewsBuilder/Resources");
        const auto qrcPath = QStringLiteral(":/NewsBuilder");
        AzQtComponents::StyleManager::addSearchPaths("newsbuilder", pathOnDisk, qrcPath);

        AzQtComponents::StyleManager::setStyleSheet(this, QStringLiteral("newsbuilder:NewsBuilder.qss"));

        UpdateEndpointLabel();

        m_ui->articleViewContainerRoot->layout()->addWidget(m_articleViewContainer);
        m_ui->articleDetailsContainerRoot->layout()->addWidget(m_articleDetailsContainer);
        m_ui->dockWidgetContents->layout()->addWidget(m_logContainer);

        connect(m_articleViewContainer, SIGNAL(articleSelectedSignal(QString)),
            this, SLOT(selectArticleSlot(QString)));
        connect(m_articleViewContainer, SIGNAL(logSignal(QString, LogType)),
            this, SLOT(addLogSlot(QString, LogType)));

        connect(m_articleDetailsContainer, &ArticleDetailsContainer::updateArticleSignal,
            this, &NewsBuilder::updateArticleSlot);
        connect(m_articleDetailsContainer, &ArticleDetailsContainer::deleteArticleSignal,
            this, &NewsBuilder::deleteArticleSlot);
        connect(m_articleDetailsContainer, &ArticleDetailsContainer::orderChangedSignal,
            this, &NewsBuilder::orderChangedSlot);
        connect(m_articleDetailsContainer, &ArticleDetailsContainer::closeArticleSignal,
            this, &NewsBuilder::closeArticleSlot);
        connect(m_articleDetailsContainer, SIGNAL(logSignal(QString, LogType)),
            this, SLOT(addLogSlot(QString, LogType)));

        m_manifest->SetSyncType(SyncType::Merge);
        m_manifest->Sync();

        // Sync Console pane with View menu
        connect(m_ui->actionConsole, &QAction::triggered, this, &NewsBuilder::OnViewLogWindow);
        connect(m_ui->dockWidget, &QDockWidget::visibilityChanged, this, &NewsBuilder::OnViewVisibilityChanged);
    }

    NewsBuilder::~NewsBuilder() {}

    void NewsBuilder::selectArticleSlot(const QString& id) const
    {
        m_articleDetailsContainer->SelectArticle(id);
    }

    void NewsBuilder::addLogSlot(QString text, LogType logType) const
    {
        AddLog(text, logType);
    }

    void NewsBuilder::addArticleToBottomSlot() const
    {
        m_articleViewContainer->AddArticle();
    }

    void NewsBuilder::updateArticleSlot(QString id) const
    {
        m_articleViewContainer->UpdateArticle(id);
    }

    void NewsBuilder::deleteArticleSlot(QString id) const
    {
        m_articleViewContainer->DeleteArticle(id);
    }

    void NewsBuilder::closeArticleSlot(QString id) const
    {
        m_articleViewContainer->CloseArticle(id);
    }

    void NewsBuilder::orderChangedSlot(QString id, bool direction) const
    {
        m_articleViewContainer->UpdateArticleOrder(id, direction);
    }

    void NewsBuilder::openSlot()
    {
        EndpointManagerView endpointManagerView(
            m_ui->centralWidget,
            *m_manifest);
        if (endpointManagerView.exec() == QDialog::Accepted)
        {
            m_manifest->Sync();
        }
        UpdateEndpointLabel();
    }

    void NewsBuilder::publishSlot()
    {
        if (!m_manifest->HasChanges())
        {
            QCustomMessageBox msgBox(
                QCustomMessageBox::Information,
                tr("Nothing to publish"),
                tr("No local changes were made, nothing to publish."),
                this);
            msgBox.AddButton(tr("Good"), 0);
            msgBox.exec();
            return;
        }

        enum Response
        {
            Yes, No
        };

        QCustomMessageBox msgBox(
            QCustomMessageBox::Critical,
            tr("Publish resources"),
            tr("You are about to overwrite the current Lumberyard Welcome Message. Are you sure you want to publish?"),
            this);
        msgBox.AddButton("Yes", Yes);
        msgBox.AddButton("No", No);
        if (msgBox.exec() == Yes)
        {
            m_manifest->SetSyncType(SyncType::Verify);
            m_manifest->Sync();
        }
    }

    void NewsBuilder::OnViewVisibilityChanged([[maybe_unused]] bool visibility)
    {
        UpdateViewMenu();
    }

    void NewsBuilder::UpdateViewMenu()
    {
        if (m_ui->actionConsole->isChecked() != m_ui->dockWidget->isVisible())
        {
            QSignalBlocker signalBlocker(m_ui->actionConsole);
            m_ui->actionConsole->setChecked(m_ui->dockWidget->isVisible());
        }
    }

    void NewsBuilder::OnViewLogWindow()
    {
        if (m_ui->dockWidget)
        {
            m_ui->dockWidget->toggleViewAction()->trigger();
        }
    }

    void NewsBuilder::UpdateEndpointLabel()
    {
        auto pEndpoint = m_manifest->GetEndpointManager()->GetSelectedEndpoint();
        if (pEndpoint)
        {
            this->setWindowTitle("News Builder (" + pEndpoint->GetName() + ")");
        }
        else
        {
            this->setWindowTitle("News Builder (No endpoint selected)");
        }
    }

    void NewsBuilder::AddLog(QString text, LogType logType) const
    {
        m_logContainer->AddLog(text, logType);
    }

    void NewsBuilder::SyncUpdate(const QString& message, LogType logType) const
    {
        AddLog(message, logType);
    }

    void NewsBuilder::SyncFail(ErrorCode error)
    {
        const char* errorMessage = GetErrorMessage(error);
        AddLog(tr("Sync failed: %1").arg(errorMessage), LogError);
        QCustomMessageBox msgBoxSyncFail(
            QCustomMessageBox::Critical,
            tr("Sync failed"),
            errorMessage,
            this);
        msgBoxSyncFail.AddButton("Ok", 0);
        msgBoxSyncFail.exec();

        m_articleViewContainer->PopulateArticles();
        m_articleDetailsContainer->Reset();
    }

    void NewsBuilder::SyncSuccess() const
    {
        AddLog("Sync completed", LogOk);
        m_articleViewContainer->PopulateArticles();
        m_articleDetailsContainer->Reset();
    }
#include "Qt/moc_NewsBuilder.cpp"

}
