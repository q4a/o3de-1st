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

#if !defined(Q_MOC_RUN)
#include <QMainWindow>

#include "NewsShared/LogType.h"
#include "NewsShared/ErrorCodes.h"
#endif

class QSignalMapper;
class QPushButton;

namespace Ui
{
    class NewsBuilderClass;
}

namespace News
{
    class BuilderArticleViewContainer;
    class LogContainer;
    class BuilderResourceManifest;
    class ArticleDetailsContainer;

    //! A central control of News Builder.
    class NewsBuilder
        : public QMainWindow
    {
        Q_OBJECT

    public:
        explicit NewsBuilder(QWidget* parent = nullptr);
        ~NewsBuilder();

    private:
        QScopedPointer<Ui::NewsBuilderClass> m_ui;
        BuilderResourceManifest* m_manifest = nullptr;
        ArticleDetailsContainer* m_articleDetailsContainer = nullptr;
        BuilderArticleViewContainer* m_articleViewContainer = nullptr;
        LogContainer* m_logContainer = nullptr;

        void UpdateEndpointLabel();
        void AddLog(QString text, LogType logType = LogInfo) const;

        void SyncUpdate(const QString& message, LogType logType) const;
        void SyncFail(ErrorCode error);
        void SyncSuccess() const;

        void OnViewVisibilityChanged(bool visibility);
        void UpdateViewMenu();
        void OnViewLogWindow();

    private Q_SLOTS:
        void selectArticleSlot(const QString& id) const;
        void addLogSlot(QString text, LogType logType) const;
        void addArticleToBottomSlot() const;
        void updateArticleSlot(QString id) const;
        void deleteArticleSlot(QString id) const;
        void closeArticleSlot(QString id) const;
        void orderChangedSlot(QString id, bool direction) const;
        void openSlot();
        void publishSlot();
    };
} // namespace News
