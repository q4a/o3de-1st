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

#include "FilteredSearchWidgetPage.h"
#include "Gallery/ui_FilteredSearchWidgetPage.h"

FilteredSearchWidgetPage::FilteredSearchWidgetPage(QWidget* parent)
    : QWidget(parent)
    , ui(new Ui::FilteredSearchWidgetPage)
{
    ui->setupUi(this);

    const auto fruitList = {"Apple", "Orange", "Pear", "Banana"};
    const auto category = tr("Fruit");
    for (const auto fruit : fruitList)
    {
        ui->filteredSearchWidget->AddTypeFilter(category, fruit);
        AzQtComponents::SearchTypeFilter filter( category, fruit );
        filter.extraIconFilename = ":/stylesheet/img/tag_visibility_on.svg";
        ui->enabledFilteredSearchWidget->AddTypeFilter(filter);
        ui->enabledHiddenFilteredSearchWidget->AddTypeFilter(filter);
        ui->shortFilteredSearchWidget->AddTypeFilter(category, fruit);
        ui->shortEnabledFilteredSearchWidget->AddTypeFilter(category, fruit);
        ui->shortEnabledHiddenFilteredSearchWidget->AddTypeFilter(category, fruit);
    }
    const auto enabledSearchWidgets = {
        ui->enabledFilteredSearchWidget,
        ui->enabledHiddenFilteredSearchWidget,
        ui->shortEnabledFilteredSearchWidget,
        ui->shortEnabledHiddenFilteredSearchWidget
    };
    for (auto searchWidget : enabledSearchWidgets)
    {
        searchWidget->SetFilterState(0, true);
        searchWidget->SetFilterState(1, true);
    }

    ui->enabledHiddenFilteredSearchWidget->setEnabledFiltersVisible(false);
    ui->shortEnabledHiddenFilteredSearchWidget->setEnabledFiltersVisible(false);

    ui->shortUnfilteredSearchWidget->setTextFilterFillsWidth(false);
    ui->shortFilteredSearchWidget->setTextFilterFillsWidth(false);
    ui->shortEnabledFilteredSearchWidget->setTextFilterFillsWidth(false);

    QString exampleText = R"(
    <pre>
#include &lt;AzQtComponents/Components/FilteredSearchWidget.h&gt;

// Create a FilteredSearchWidget
auto filteredSearchWidget = new AzQtComponents::FilteredSearchWidget(parent);

// Add a filter type
filteredSearchWidget-&gt;addTypeFilter("Category name", "Type name");

// Attach the FilterText FilteredSearchWidget to a QSortFilterProxyModel derived class:
MyProxyModel proxyModel;
connect(filteredSearchWidget, &AzQtComponents::FilteredSearchWidget::TextFilterChanged,
        proxyModel, static_cast&lt;void (QSortFilterProxyModel::*)(const QString&)&gt;(&MyProxyModel::setFilterRegExp));

// Attach the TypeFilter to a proxy model that has a slot which can handle
// FilteredSearchWidget::TypeFilterChanged(const SearchTypeFilterList& activeTypeFilters)
connect(filteredSearchWidget, &AzQtComponents::FilteredSearchWidget::TypeFilterChanged,
        proxyModel, &MyProxyModel::ApplyTypeFilters);

// Prevent the LineEdit from expanding the full width of the FilteredSearchWidget using the
// property textFilterFillsWidth:
filteredSearchWidget=&gt;setTextFilterFillsWidth(false);
    </pre>
    )";

    ui->exampleText->setHtml(exampleText);
}

FilteredSearchWidgetPage::~FilteredSearchWidgetPage()
{
}

#include "Gallery/moc_FilteredSearchWidgetPage.cpp"
