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
#include <AzQtComponents/AzQtComponentsAPI.h>

#include <QDockWidget>
#include <QPoint>
#endif

namespace AzQtComponents
{
    class TitleBar;

    class AZ_QT_COMPONENTS_API StyledDockWidget
        : public QDockWidget
    {
        Q_OBJECT

    public:
        explicit StyledDockWidget(QWidget* parent = nullptr);
        explicit StyledDockWidget(const QString& name, QWidget* parent = nullptr);
        explicit StyledDockWidget(const QString& name, bool skipTitleBarOverdraw /* = false */, QWidget* parent = nullptr);
        ~StyledDockWidget();

        static void drawFrame(QPainter& p, QRect rect, bool drawTop = true);
        void createCustomTitleBar();
        TitleBar* customTitleBar() const;
        bool isSingleFloatingChild();

        /**
        * Returns true if title bar overdraw is being used.
        * Title bar overdraw is only used on Windows 10. In this mode we do have native title bar but
        * we draw our own on top of it instead of removing it. It's a workaround against Win10
        * bug where a white stripe appears if we have native border + no native title bar.
        */
        bool doesTitleBarOverdraw() const;
        bool skipTitleBarOverdraw() const;

    Q_SIGNALS:
        void undock();
        void aboutToClose();

    protected:
        void closeEvent(QCloseEvent* event) override;
        bool nativeEvent(const QByteArray& eventType, void* message, long* result) override;
        bool event(QEvent* event) override;
        void showEvent(QShowEvent* event) override;
        void paintEvent(QPaintEvent*) override;

    private:

        void fixFramelessFlags();
        void onFloatingChanged(bool floating);
        void init();

        bool m_skipTitleBarOverdraw = false;
    };
} // namespace AzQtComponents
