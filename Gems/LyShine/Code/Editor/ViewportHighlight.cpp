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
#include "UiCanvasEditor_precompiled.h"

#include "EditorCommon.h"

ViewportHighlight::ViewportHighlight()
    : m_highlightIconSelected(new ViewportIcon("Editor/Plugins/UiCanvasEditor/CanvasIcons/Border_Selected.tif"))
    , m_highlightIconUnselected(new ViewportIcon("Editor/Plugins/UiCanvasEditor/CanvasIcons/Border_Unselected.tif"))
{
}

ViewportHighlight::~ViewportHighlight()
{
}

void ViewportHighlight::Draw(Draw2dHelper& draw2d,
    QTreeWidgetItem* invisibleRootItem,
    HierarchyItemRawPtrList& selectedItems,
    uint32 flags)
{
    // first draw any unselected element borders (if flag is set to draw them)
    if (flags & ViewportWidget::DrawElementBorders_Unselected)
    {
        HierarchyItemRawPtrList allItems;
        HierarchyHelpers::AppendAllChildrenToEndOfList(invisibleRootItem, allItems);

        for (auto item : allItems)
        {
            AZ::Entity* element = item->GetElement();
            AZ_Warning("UI", element, "Missing entity for hierarchy item");
            if (!element)
            {
                continue;
            }

            bool shouldDrawBorder = true;
            if (item->isSelected())
            {
                // this element is in the selected list
                // ignore the selected items - we draw border for those afterwards so that they are on top

                shouldDrawBorder = false;
            }
            else
            {
                // This element is not selected

                if (!(flags & ViewportWidget::DrawElementBorders_Parent))
                {
                    // the flag is NOT set to draw borders on elements that are parents
                    // so if this element has children we should NOT draw a border for it

                    int numChildren = 0;
                    EBUS_EVENT_ID_RESULT(numChildren, element->GetId(), UiElementBus, GetNumChildElements);
                    if (numChildren > 0)
                    {
                        shouldDrawBorder = false;
                    }
                }

                if (!(flags & ViewportWidget::DrawElementBorders_Visual))
                {
                    // the flag is NOT set to draw borders on elements that are visual
                    // so if this element has any visual components we should NOT draw a border for it

                    if (UiVisualBus::FindFirstHandler(element->GetId()))
                    {
                        shouldDrawBorder = false;
                    }
                }

                if (!(flags & ViewportWidget::DrawElementBorders_Hidden))
                {
                    // the flag is NOT set to draw borders on elements that are hidden
                    // so if this element is hidden we should NOT draw a border for it

                    bool isVisible = false;
                    EBUS_EVENT_ID_RESULT(isVisible, element->GetId(), UiEditorBus, GetIsVisible);

                    bool areAllAncestorsVisible = false;
                    EBUS_EVENT_ID_RESULT(areAllAncestorsVisible, element->GetId(), UiEditorBus, AreAllAncestorsVisible);

                    if (!(isVisible && areAllAncestorsVisible))
                    {
                        shouldDrawBorder = false;
                    }
                }
            }

            if (shouldDrawBorder)
            {
                m_highlightIconUnselected->DrawElementRectOutline(draw2d, element->GetId(), ViewportHelpers::unselectedColor);
            }
        }
    }

    // Now draw the borders for any selected elements
    for (auto item : selectedItems)
    {
        AZ::Entity* element = item->GetElement();
        AZ_Warning("UI", element, "Missing entity for hierarchy item");
        if (element)
        {
            m_highlightIconSelected->DrawElementRectOutline(draw2d, element->GetId(), ViewportHelpers::selectedColor);
        }
    }
}

void ViewportHighlight::DrawHover(Draw2dHelper& draw2d, AZ::EntityId hoverElement)
{
    m_highlightIconSelected->DrawElementRectOutline(draw2d, hoverElement, ViewportHelpers::highlightColor);
}
