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

#include <AzQtComponents/Components/FilteredSearchWidget.h>

#include <QStyledItemDelegate>
#include <QStandardItem>
#endif

Q_DECLARE_METATYPE(AZ::Uuid);

namespace Ui
{
    class OutlinerSearchWidget;
}

namespace AzQtComponents
{
    class OutlinerSearchItemDelegate;

    class OutlinerSearchTypeSelector
        : public SearchTypeSelector
    {
    public:
        OutlinerSearchTypeSelector(QWidget* parent = nullptr);

    protected:
        // can be used to override the logic when adding items in RepopulateDataModel
        bool filterItemOut(int unfilteredDataIndex, bool itemMatchesFilter, bool categoryMatchesFilter) override;
        void initItem(QStandardItem* item, const SearchTypeFilter& filter, int unfilteredDataIndex) override;
        int GetNumFixedItems() override;
    };

    class OutlinerCriteriaButton
        : public FilterCriteriaButton
    {
        Q_OBJECT

    public:
        explicit OutlinerCriteriaButton(QString labelText, QWidget* parent = nullptr, int index = -1);
   };

    class OutlinerSearchWidget
        : public FilteredSearchWidget
    {
        Q_OBJECT
    public:
        explicit OutlinerSearchWidget(QWidget* parent = nullptr);
        ~OutlinerSearchWidget() override;

        FilterCriteriaButton* createCriteriaButton(const SearchTypeFilter& filter, int filterIndex) override;

        enum class GlobalSearchCriteria : int
        {
            Unlocked,
            Locked,
            Visible,
            Hidden,
            Separator,
            FirstRealFilter
        };
    protected:
        void SetupPaintDelegates() override;
    private:
        OutlinerSearchItemDelegate* m_delegate = nullptr;
    };

    class OutlinerIcons
    {
    public:
        static OutlinerIcons& GetInstance()
        {
            static OutlinerIcons instance;
            return instance;
        }
        OutlinerIcons(OutlinerIcons const &) = delete;
        void operator=(OutlinerIcons const &) = delete;

        QIcon& GetIcon(int iconWanted) { return m_globalIcons[iconWanted]; }
    private:
        OutlinerIcons();

        QIcon m_globalIcons[static_cast<int>(AzQtComponents::OutlinerSearchWidget::GlobalSearchCriteria::FirstRealFilter)];
    };

    class OutlinerSearchItemDelegate : public QStyledItemDelegate
    {
    public:
        explicit OutlinerSearchItemDelegate(QWidget* parent = nullptr);

        void PaintRichText(QPainter* painter, QStyleOptionViewItem& opt, QString& text) const;
        void SetSelector(SearchTypeSelector* selector) { m_selector = selector; }

        // QStyleItemDelegate overrides.
        void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
        QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override;

    private:
        SearchTypeSelector* m_selector = nullptr;
    };
}