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

#include <QObject>
#include <QColor>
#include <QHash>
AZ_PUSH_DISABLE_WARNING(4251, "-Wunknown-warning-option") // 4251: 'AzQtComponents::StyleManager::m_widgetToStyleSheetMap': class 'QHash<QWidget *,QString>' needs to have dll-interface to be used by clients of class 'AzQtComponents::StyleManager'
#include <QPointer>
#endif
AZ_POP_DISABLE_WARNING

class QApplication;
class QStyle;
class QWidget;
class QStyleSheetStyle;

namespace AzQtComponents
{
    class StyleSheetCache;
    class StylesheetPreprocessor;
    class TitleBarOverdrawHandler;
    class AutoCustomWindowDecorations;

    /**
     * Wrapper around classes dealing with Lumberyard style.
     *
     * New applications should work like this:
     *   
     *   int main(int argv, char **argc)
     *   {
     *           QApplication app(argv, argc);
     *
     *           AzQtComponents::StyleManager styleManager(&app);
     *           const bool useUI10 = false;
     *           styleManager.Initialize(&app, useUI10);
     *           .
     *           .
     *           .
     *   }
     *
     */
    class AZ_QT_COMPONENTS_API StyleManager
        : public QObject
    {
        Q_OBJECT

        static StyleManager* s_instance;

    public:
        static bool isInstanced() { return s_instance; }

        static void addSearchPaths(const QString& searchPrefix, const QString& pathOnDisk, const QString& qrcPrefix);

        static bool setStyleSheet(QWidget* widget, QString styleFileName);

        static QStyleSheetStyle* styleSheetStyle(const QWidget* widget);
        static QStyle* baseStyle(const QWidget* widget);

        static void repolishStyleSheet(QWidget* widget);

        explicit StyleManager(QObject* parent);
        ~StyleManager() override;

        /*!
        * Call to initialize the StyleManager, allowing it to hook into the application and apply the global style
        */
        void initialize(QApplication* application);
        // deprecated; introduced before the new camelCase Qt based method names were adopted.
        void Initialize(QApplication* application) { initialize(application); }

        /*!
        * Call this to force a refresh of the global stylesheet and a reload of any settings files.
        * Note that you should never need to do this manually.
        */
        void refresh();
        // deprecated; introduced before the new camelCase Qt based method names were adopted.
        void Refresh() { refresh(); }

        /*!
        * Used to get a global color value by name.
        * Deprecated; do not use.
        * This was implemented to support skinning of the Editor,
        * but that functionality is no longer supported. If you
        * want to load a color instead of hard coding it, please
        * embed the color into a stylesheet instead of using
        * GetColorByName.
        */
        const QColor& getColorByName(const QString& name);
        // deprecated; introduced before the new camelCase Qt based method names were adopted.
        const QColor& GetColorByName(const QString& name) { return getColorByName(name); }

    private Q_SLOTS:
        void cleanupStyles();
        void stopTrackingWidget(QObject* object);

    private:
        void initializeFonts();
        void initializeSearchPaths(QApplication* application);

        void resetWidgetSheets();

        StylesheetPreprocessor* m_stylesheetPreprocessor = nullptr;
        StyleSheetCache* m_stylesheetCache = nullptr;
        TitleBarOverdrawHandler* m_titleBarOverdrawHandler = nullptr;

        using WidgetToStyleSheetMap = QHash<QWidget*, QString>;
        AZ_PUSH_DISABLE_WARNING(4251, "-Wunknown-warning-option") // 4251: 'AzQtComponents::StyleManager::m_widgetToStyleSheetMap': class 'QHash<QWidget *,QString>' needs to have dll-interface to be used by clients of class 'AzQtComponents::StyleManager'
        WidgetToStyleSheetMap m_widgetToStyleSheetMap;
        QStyleSheetStyle* m_styleSheetStyle = nullptr;

        // Track the style as a QPointer, as the QApplication will delete it if it still has a pointer to it
        QPointer<QStyle> m_style;
        AZ_POP_DISABLE_WARNING

        AutoCustomWindowDecorations* m_autoCustomWindowDecorations = nullptr;
    };
} // namespace AzQtComponents

