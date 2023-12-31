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
#include "precompiled.h"

#include <QCoreApplication>
#include <qgraphicslayoutitem.h>
#include <qgraphicsscene.h>
#include <qsizepolicy.h>
#include <QGraphicsSceneDragDropEvent>

#include <Components/Slots/Data/DataSlotLayoutComponent.h>

#include <Components/Slots/Data/DataSlotComponent.h>
#include <GraphCanvas/Components/NodePropertyDisplay/NodePropertyDisplay.h>
#include <GraphCanvas/Components/Slots/SlotBus.h>
#include <GraphCanvas/Editor/GraphModelBus.h>
#include <GraphCanvas/GraphCanvasBus.h>
#include <GraphCanvas/GraphicsItems/GraphCanvasSceneEventFilter.h>
#include <Components/Slots/Data/DataSlotConnectionPin.h>
#include <Widgets/GraphCanvasLabel.h>


namespace GraphCanvas
{
    class DataSlotGraphicsEventFilter
        : public SceneEventFilter
    {
    public:
        AZ_CLASS_ALLOCATOR(DataSlotGraphicsEventFilter, AZ::SystemAllocator, 0);

        DataSlotGraphicsEventFilter(DataSlotLayout* dataSlotLayout)
            : SceneEventFilter(nullptr)
            , m_owner(dataSlotLayout)
        {

        }

        bool sceneEventFilter([[maybe_unused]] QGraphicsItem* watched, QEvent* event)
        {
            switch (event->type())
            {
            case QEvent::GraphicsSceneDragEnter:
                m_owner->OnDragEnterEvent(static_cast<QGraphicsSceneDragDropEvent*>(event));
                break;
            case QEvent::GraphicsSceneDragLeave:
                m_owner->OnDragLeaveEvent(static_cast<QGraphicsSceneDragDropEvent*>(event));
                break;
            case QEvent::GraphicsSceneDrop:
                m_owner->OnDropEvent(static_cast<QGraphicsSceneDragDropEvent*>(event));
                break;
            default:
                break;
            }

            return event->isAccepted();
        }

    private:
        DataSlotLayout* m_owner;
    };

    ////////////////////////////////////////////////
    // DataTypeConversionDataSlotDragDropInterface
    ////////////////////////////////////////////////    

    DataSlotLayout::DataTypeConversionDataSlotDragDropInterface::DataTypeConversionDataSlotDragDropInterface(const SlotId& slotId)
        : m_slotId(slotId)
    {
    }

    AZ::Outcome<DragDropState> DataSlotLayout::DataTypeConversionDataSlotDragDropInterface::OnDragEnterEvent(QGraphicsSceneDragDropEvent* dragDropEvent)
    {
        NodeId nodeId;
        SlotRequestBus::EventResult(nodeId, m_slotId, &SlotRequests::GetNode);

        GraphId graphId;
        SceneMemberRequestBus::EventResult(graphId, nodeId, &SceneMemberRequests::GetScene);

        const QMimeData* mimeData = dragDropEvent->mimeData();

        if (mimeData)
        {
            DataSlotRequests* dataSlotRequests = DataSlotRequestBus::FindFirstHandler(m_slotId);

            if (dataSlotRequests)
            {
                if (mimeData->hasFormat(GraphCanvas::k_ReferenceMimeType))
                {
                    bool canHandleEvent = false;

                    if (dataSlotRequests->GetDataSlotType() == DataSlotType::Reference || dataSlotRequests->CanConvertToReference())
                    {
                        GraphModelRequestBus::EventResult(canHandleEvent, graphId, &GraphModelRequests::CanHandleReferenceMimeEvent, Endpoint(nodeId, m_slotId), mimeData);
                    }

                    return AZ::Success(canHandleEvent ? DragDropState::Valid : DragDropState::Invalid);
                }
                else if (mimeData->hasFormat(GraphCanvas::k_ValueMimeType))
                {
                    bool canHandleEvent = false;

                    if (dataSlotRequests->GetDataSlotType() == DataSlotType::Value || dataSlotRequests->CanConvertToValue())
                    {
                        GraphModelRequestBus::EventResult(canHandleEvent, graphId, &GraphModelRequests::CanHandleValueMimeEvent, Endpoint(nodeId, m_slotId), mimeData);
                    }

                    return AZ::Success(canHandleEvent ? DragDropState::Valid : DragDropState::Invalid);
                }
            }
        }

        return AZ::Failure();
    }

    void DataSlotLayout::DataTypeConversionDataSlotDragDropInterface::OnDragLeaveEvent(QGraphicsSceneDragDropEvent* dragDropEvent)
    {
        AZ_UNUSED(dragDropEvent);
    }

    void DataSlotLayout::DataTypeConversionDataSlotDragDropInterface::OnDropEvent(QGraphicsSceneDragDropEvent* dragDropEvent)
    {
        const QMimeData* mimeData = dragDropEvent->mimeData();

        if (mimeData)
        {
            DataSlotRequests* dataSlotRequests = DataSlotRequestBus::FindFirstHandler(m_slotId);

            if (dataSlotRequests)
            {
                NodeId nodeId;
                SlotRequestBus::EventResult(nodeId, m_slotId, &SlotRequests::GetNode);

                GraphId graphId;
                SceneMemberRequestBus::EventResult(graphId, nodeId, &SceneMemberRequests::GetScene);

                bool postUndo = false;

                if (mimeData->hasFormat(GraphCanvas::k_ReferenceMimeType))
                {
                    ScopedGraphUndoBlocker undoBlocker(graphId);

                    if (dataSlotRequests->GetDataSlotType() != DataSlotType::Reference)
                    {
                        if (!dataSlotRequests->ConvertToReference())
                        {
                            return;
                        }
                    }

                    GraphModelRequestBus::EventResult(postUndo, graphId, &GraphModelRequests::HandleReferenceMimeEvent, Endpoint(nodeId, m_slotId), mimeData);
                }
                else if (mimeData->hasFormat(GraphCanvas::k_ValueMimeType))
                {
                    ScopedGraphUndoBlocker undoBlocker(graphId);

                    if (dataSlotRequests->GetDataSlotType() != DataSlotType::Reference)
                    {
                        if (!dataSlotRequests->ConvertToReference())
                        {
                            return;
                        }
                    }

                    GraphModelRequestBus::EventResult(postUndo, graphId, &GraphModelRequests::HandleValueMimeEvent, Endpoint(nodeId, m_slotId), mimeData);
                }

                if (postUndo)
                {
                    GraphModelRequestBus::Event(graphId, &GraphModelRequests::RequestUndoPoint);
                }
            }
        }
    }

    ///////////////////
    // DataSlotLayout
    ///////////////////
    
    DataSlotLayout::DataSlotLayout(DataSlotLayoutComponent& owner)
        : m_activeHandler(nullptr)
        , m_eventFilter(nullptr)
        , m_valueReferenceInterface(nullptr)
        , m_dragDropState(DragDropState::Idle)
        , m_connectionType(ConnectionType::CT_Invalid)
        , m_owner(owner)
        , m_nodePropertyDisplay(nullptr)
        , m_doubleClickFilter(nullptr)
    {
        setInstantInvalidatePropagation(true);
        setOrientation(Qt::Horizontal);

        m_spacer = new QGraphicsWidget();
        m_spacer->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
        m_spacer->setAutoFillBackground(true);
        m_spacer->setMinimumSize(0, 0);
        m_spacer->setPreferredWidth(0);
        m_spacer->setMaximumHeight(0);

        m_nodePropertyDisplay = aznew NodePropertyDisplayWidget();
        m_slotConnectionPin = aznew DataSlotConnectionPin(owner.GetEntityId());
        m_slotText = aznew GraphCanvasLabel();        
    }

    DataSlotLayout::~DataSlotLayout()
    {
        AZ::SystemTickBus::Handler::BusDisconnect();
        delete m_valueReferenceInterface;

        if (m_doubleClickFilter)
        {
            m_slotText->removeSceneEventFilter(m_doubleClickFilter);
            delete m_doubleClickFilter;
        }
    }

    void DataSlotLayout::Activate()
    {
        DataSlotNotificationBus::Handler::BusConnect(m_owner.GetEntityId());
        SceneMemberNotificationBus::Handler::BusConnect(m_owner.GetEntityId());
        SlotNotificationBus::Handler::BusConnect(m_owner.GetEntityId());
        StyleNotificationBus::Handler::BusConnect(m_owner.GetEntityId());
        DataSlotLayoutRequestBus::Handler::BusConnect(m_owner.GetEntityId());
        m_slotConnectionPin->Activate();
    }

    void DataSlotLayout::Deactivate()
    {
        m_slotConnectionPin->Deactivate();
        SceneMemberNotificationBus::Handler::BusDisconnect();
        SlotNotificationBus::Handler::BusDisconnect();
        StyleNotificationBus::Handler::BusDisconnect();
        DataSlotLayoutRequestBus::Handler::BusDisconnect();
        DataSlotNotificationBus::Handler::BusDisconnect();
        NodeDataSlotRequestBus::Handler::BusDisconnect();

        AZ::SystemTickBus::Handler::BusDisconnect();
    }

    void DataSlotLayout::OnSystemTick()
    {
        UpdateFilterState();

        if (m_slotText && m_doubleClickFilter)
        {
            // Remove then re-install the event filter just in case the event filter got installed
            // between various calls.
            m_slotText->removeSceneEventFilter(m_doubleClickFilter);
            m_slotText->installSceneEventFilter(m_doubleClickFilter);
        }

        AZ::SystemTickBus::Handler::BusDisconnect();
    }

    void DataSlotLayout::OnSceneSet(const AZ::EntityId& sceneId)
    {
        SlotRequests* slotRequests = SlotRequestBus::FindFirstHandler(m_owner.GetEntityId());

        if (slotRequests)
        {
            m_connectionType = slotRequests->GetConnectionType();

            TranslationKeyedString slotName = slotRequests->GetTranslationKeyedName();

            m_slotText->SetLabel(slotName);

            TranslationKeyedString toolTip = slotRequests->GetTranslationKeyedTooltip();            
            OnTooltipChanged(toolTip);

            if (m_doubleClickFilter == nullptr)
            {
                QGraphicsScene* graphicsScene = nullptr;
                SceneRequestBus::EventResult(graphicsScene, sceneId, &SceneRequests::AsQGraphicsScene);

                if (graphicsScene)
                {
                    m_doubleClickFilter = new DoubleClickSceneEventFilter((*this));
                    graphicsScene->addItem(m_doubleClickFilter);

                    if (m_slotText->scene())
                    {
                        m_slotText->installSceneEventFilter(m_doubleClickFilter);
                    }
                    else
                    {
                        // SlotText hasn't been assigned to the layout yet. Delay this until the next tick
                        // so we know the text has been properly assigned to the layout.
                        AZ::SystemTickBus::Handler::BusConnect();
                    }
                }
            }
        }
        
        TryAndSetupSlot();
    }

    void DataSlotLayout::OnSceneReady()
    {
        TryAndSetupSlot();
    }

    void DataSlotLayout::OnRegisteredToNode(const AZ::EntityId& nodeId)
    {
        NodeDataSlotRequestBus::Handler::BusDisconnect();
        NodeDataSlotRequestBus::Handler::BusConnect(nodeId);
        TryAndSetupSlot();

        // Queue our update filter requests until the next tick since the scene might not be set immediately
        AZ::SystemTickBus::Handler::BusConnect();
    }

    void DataSlotLayout::OnNameChanged(const TranslationKeyedString& name)
    {
        m_slotText->SetLabel(name);
    }

    void DataSlotLayout::OnTooltipChanged(const TranslationKeyedString& tooltip)
    {
        AZ::Uuid dataType;
        DataSlotRequestBus::EventResult(dataType, m_owner.GetEntityId(), &DataSlotRequests::GetDataTypeId);

        AZStd::string typeString;
        GraphModelRequestBus::EventResult(typeString, GetSceneId(), &GraphModelRequests::GetDataTypeString, dataType);

        AZStd::string displayText = tooltip.GetDisplayString();

        if (!typeString.empty())
        {
            if (displayText.empty())
            {
                displayText = typeString;
            }
            else
            {
                displayText = AZStd::string::format("%s - %s", typeString.c_str(), displayText.c_str());
            }
        }

        m_slotConnectionPin->setToolTip(displayText.c_str());
        m_slotText->setToolTip(displayText.c_str());
        m_nodePropertyDisplay->setToolTip(displayText.c_str());
    }

    void DataSlotLayout::OnStyleChanged()
    {
        m_style.SetStyle(m_owner.GetEntityId());

        m_nodePropertyDisplay->RefreshStyle();

        switch (m_connectionType)
        {
        case ConnectionType::CT_Input:
            m_slotText->SetStyle(m_owner.GetEntityId(), ".inputSlotName");
            break;
        case ConnectionType::CT_Output:
            m_slotText->SetStyle(m_owner.GetEntityId(), ".outputSlotName");
            break;
        default:
            m_slotText->SetStyle(m_owner.GetEntityId(), ".slotName");
            break;
        };

        m_slotConnectionPin->RefreshStyle();

        qreal padding = m_style.GetAttribute(Styling::Attribute::Padding, 2.);
        setContentsMargins(padding, padding, padding, padding);
        setSpacing(m_style.GetAttribute(Styling::Attribute::Spacing, 2.));

        UpdateGeometry();
    }

    const DataSlotConnectionPin* DataSlotLayout::GetConnectionPin() const
    {
        return m_slotConnectionPin;
    }

    void DataSlotLayout::UpdateDisplay()
    {
        if (m_nodePropertyDisplay != nullptr
            && m_nodePropertyDisplay->GetNodePropertyDisplay() != nullptr)
        {
            m_nodePropertyDisplay->GetNodePropertyDisplay()->UpdateDisplay();
        }
        if (m_slotConnectionPin != nullptr)
        {
            m_slotConnectionPin->RefreshStyle();
        }
    }

    void DataSlotLayout::OnDataSlotTypeChanged([[maybe_unused]] const DataSlotType& dataSlotType)
    {
        RecreatePropertyDisplay();
    }

    void DataSlotLayout::OnDisplayTypeChanged([[maybe_unused]] const AZ::Uuid& dataType, [[maybe_unused]] const AZStd::vector<AZ::Uuid>& typeIds)
    {
        DataSlotType slotType = DataSlotType::Unknown;
        DataSlotRequestBus::EventResult(slotType, m_owner.GetEntityId(), &DataSlotRequests::GetDataSlotType);

        // Only update with value changes since ideally Reference properties
        // never change their display.
        //
        // If this changes, a fix will need to be made for a crash since it will wind up deleting the node property display
        // while that node property display is sending out the signal
        if (slotType == DataSlotType::Value)
        {
            RecreatePropertyDisplay();
        }

        UpdateDisplay();
    }

    void DataSlotLayout::RecreatePropertyDisplay()
    {
        if (m_nodePropertyDisplay != nullptr)
        {
            auto propertyDisplay = m_nodePropertyDisplay->GetNodePropertyDisplay();

            if (propertyDisplay)
            {
                RemoveDataSlotDragDropInterface(propertyDisplay);
            }

            m_nodePropertyDisplay->ClearDisplay();
            TryAndSetupSlot();
        }
    }

    void DataSlotLayout::OnDragEnterEvent(QGraphicsSceneDragDropEvent* dragDropEvent)
    {
        if (m_activeHandler != nullptr)
        {
            AZ_Error("GraphCanvas", false, "Failed got multiple drag enter events without drop or leave");
            m_activeHandler = nullptr;
        }

        for (auto dragDropInterface : m_dragDropInterfaces)
        {
            auto outcome = dragDropInterface->OnDragEnterEvent(dragDropEvent);

            if (outcome)
            {
                m_activeHandler = dragDropInterface;
                SetDragDropState(outcome.GetValue());
                break;
            }
        }

        if (m_activeHandler)
        {
            dragDropEvent->accept();
            dragDropEvent->acceptProposedAction();
        }
    }

    void DataSlotLayout::OnDragLeaveEvent(QGraphicsSceneDragDropEvent* dragDropEvent)
    {
        if (m_activeHandler)
        {
            m_activeHandler->OnDragLeaveEvent(dragDropEvent);
        }

        m_activeHandler = nullptr;
        SetDragDropState(DragDropState::Idle);

        dragDropEvent->accept();
    }

    void DataSlotLayout::OnDropEvent(QGraphicsSceneDragDropEvent* dragDropEvent)
    {
        if (m_activeHandler && m_dragDropState == DragDropState::Valid)
        {
            m_activeHandler->OnDropEvent(dragDropEvent);
        }

        m_activeHandler = nullptr;
        SetDragDropState(DragDropState::Idle);

        dragDropEvent->accept();
    }

    void DataSlotLayout::UpdateFilterState()
    {
        QGraphicsItem* ownerItem = m_owner.AsGraphicsItem();
        QGraphicsScene* scene = ownerItem->scene();

        if (scene == nullptr)
        {
            return;
        }

        if (!m_dragDropInterfaces.empty() && m_eventFilter == nullptr)
        {
            m_eventFilter = aznew DataSlotGraphicsEventFilter(this);
            scene->addItem(m_eventFilter);
            m_owner.AsGraphicsItem()->installSceneEventFilter(m_eventFilter);
            m_owner.AsGraphicsItem()->setAcceptDrops(true);
        }
        else if (m_dragDropInterfaces.empty() && m_eventFilter)
        {
            m_owner.AsGraphicsItem()->removeSceneEventFilter(m_eventFilter);
            scene->removeItem(m_eventFilter);
            delete m_eventFilter;
            m_eventFilter = nullptr;
            m_owner.AsGraphicsItem()->setAcceptDrops(false);
        }
    }

    void DataSlotLayout::RegisterDataSlotDragDropInterface(DataSlotDragDropInterface* dragDropInterface)
    {
        bool needsUpdate = m_dragDropInterfaces.empty() || m_eventFilter == nullptr;

        m_dragDropInterfaces.insert(dragDropInterface);

        if (needsUpdate)
        {
            UpdateFilterState();
        }
    }

    void DataSlotLayout::RemoveDataSlotDragDropInterface(DataSlotDragDropInterface* dragDropInterface)
    {
        m_dragDropInterfaces.erase(dragDropInterface);

        if (m_dragDropInterfaces.empty())
        {
            UpdateFilterState();
        }
    }

    void DataSlotLayout::SetDragDropState(DragDropState dragDropState)
    {
        if (m_dragDropState != dragDropState)
        {
            m_dragDropState = dragDropState;

            switch (m_dragDropState)
            {
            case DragDropState::Idle:
                m_owner.AsGraphicsItem()->setOpacity(1.0f);
                m_slotText->GetStyleHelper().RemoveSelector(Styling::States::ValidDrop);
                m_slotText->GetStyleHelper().RemoveSelector(Styling::States::InvalidDrop);
                break;
            case DragDropState::Invalid:
                m_owner.AsGraphicsItem()->setOpacity(0.5f);
                m_slotText->GetStyleHelper().AddSelector(Styling::States::InvalidDrop);
                break;
            case DragDropState::Valid:
                m_owner.AsGraphicsItem()->setOpacity(1.0f);
                m_slotText->GetStyleHelper().AddSelector(Styling::States::ValidDrop);
                break;
            default:
            {
                AZ_Warning("GraphCanvas", false, "Unknown DragDropState state given.");
                return;
            }
            }

            DataSlotNotificationBus::Event(m_owner.GetEntityId(), &DataSlotNotifications::OnDragDropStateStateChanged, m_dragDropState);

            m_slotText->update();
        }
    }

    AZ::EntityId DataSlotLayout::GetSceneId() const
    {
        AZ::EntityId sceneId;
        SceneMemberRequestBus::EventResult(sceneId, m_owner.GetEntityId(), &SceneMemberRequests::GetScene);
        return sceneId;
    }

    void DataSlotLayout::TryAndSetupSlot()
    {
        if (m_nodePropertyDisplay->GetNodePropertyDisplay() == nullptr)
        {
            CreateDataDisplay();
        }

        if (m_valueReferenceInterface == nullptr)
        {
            QGraphicsItem* graphicsItem = m_owner.AsGraphicsItem();

            DataSlotRequests* dataSlotRequests = DataSlotRequestBus::FindFirstHandler(m_owner.GetEntityId());

            if (dataSlotRequests && graphicsItem)
            {
                bool canConvertToReference = dataSlotRequests->CanConvertToReference();
                bool canConvertToValue = dataSlotRequests->CanConvertToValue();

                if (canConvertToReference || canConvertToValue)
                {
                    m_valueReferenceInterface = aznew DataTypeConversionDataSlotDragDropInterface(m_owner.GetEntityId());
                    RegisterDataSlotDragDropInterface(m_valueReferenceInterface);
                }
            }
        }
    }

    void DataSlotLayout::CreateDataDisplay()
    {
        NodePropertyDisplay* nodePropertyDisplay = nullptr;

        bool isInput = m_connectionType == CT_Input;

        DataSlotType slotType = DataSlotType::Unknown;
        DataSlotRequestBus::EventResult(slotType, m_owner.GetEntityId(), &DataSlotRequests::GetDataSlotType);

        bool isReference = slotType == DataSlotType::Reference;

        if (isInput || isReference)
        {
            DataSlotType dataSlotType = DataSlotType::Unknown;
            DataSlotRequestBus::EventResult(dataSlotType, m_owner.GetEntityId(), &DataSlotRequests::GetDataSlotType);

            AZ::EntityId nodeId;
            SlotRequestBus::EventResult(nodeId, m_owner.GetEntityId(), &SlotRequests::GetNode);

            AZ::Uuid typeId;
            DataSlotRequestBus::EventResult(typeId, m_owner.GetEntityId(), &DataSlotRequests::GetDataTypeId);

            AZ::EntityId sceneId;
            SceneMemberRequestBus::EventResult(sceneId, m_owner.GetEntityId(), &SceneMemberRequests::GetScene);

            GraphModelRequestBus::EventResult(nodePropertyDisplay, sceneId, &GraphModelRequests::CreateDataSlotPropertyDisplay, typeId, nodeId, m_owner.GetEntityId());

            if (nodePropertyDisplay)
            {
                nodePropertyDisplay->SetNodeId(nodeId);
                nodePropertyDisplay->SetSlotId(m_owner.GetEntityId());

                m_nodePropertyDisplay->SetNodePropertyDisplay(nodePropertyDisplay);

                if (nodePropertyDisplay->EnableDropHandling())
                {
                    RegisterDataSlotDragDropInterface(nodePropertyDisplay);
                }
            }
        }

        UpdateLayout();
        OnStyleChanged();
    }

    void DataSlotLayout::UpdateLayout()
    {
        // make sure the connection type or visible items have changed before redoing the layout
        if (m_connectionType != m_atLastUpdate.connectionType || 
            m_slotConnectionPin != m_atLastUpdate.slotConnectionPin ||
            m_slotText != m_atLastUpdate.slotText ||  
            m_nodePropertyDisplay != m_atLastUpdate.nodePropertyDisplay || 
            m_spacer != m_atLastUpdate.spacer)
        {
            for (int i = count() - 1; i >= 0; --i)
            {
                removeAt(i);
            }

            switch (m_connectionType)
            {
            case ConnectionType::CT_Input:
                addItem(m_slotConnectionPin);
                setAlignment(m_slotConnectionPin, Qt::AlignLeft);

                addItem(m_slotText);
                setAlignment(m_slotText, Qt::AlignLeft);

                addItem(m_nodePropertyDisplay);
                setAlignment(m_nodePropertyDisplay, Qt::AlignLeft);

                addItem(m_spacer);
                setAlignment(m_spacer, Qt::AlignLeft);
                break;
            case ConnectionType::CT_Output:
                addItem(m_spacer);
                setAlignment(m_spacer, Qt::AlignRight);

                if (m_nodePropertyDisplay)
                {
                    addItem(m_nodePropertyDisplay);
                    setAlignment(m_nodePropertyDisplay, Qt::AlignRight);
                }

                addItem(m_slotText);
                setAlignment(m_slotText, Qt::AlignRight);

                addItem(m_slotConnectionPin);
                setAlignment(m_slotConnectionPin, Qt::AlignRight);
                break;
            default:
                addItem(m_slotConnectionPin);
                addItem(m_slotText);
                addItem(m_spacer);
                break;
            }
            UpdateGeometry();

            m_atLastUpdate.connectionType = m_connectionType;
            m_atLastUpdate.slotConnectionPin = m_slotConnectionPin;
            m_atLastUpdate.slotText = m_slotText;
            m_atLastUpdate.nodePropertyDisplay = m_nodePropertyDisplay;
            m_atLastUpdate.spacer = m_spacer;
        }
    }

    void DataSlotLayout::UpdateGeometry()
    {
        m_slotConnectionPin->updateGeometry();
        m_slotText->update();

        invalidate();
        updateGeometry();
    }

    void DataSlotLayout::OnSlotTextDoubleClicked()
    {
        bool isConnected = false;
        SlotRequestBus::EventResult(isConnected, m_owner.GetEntityId(), &SlotRequests::HasConnections);

        if (!isConnected)
        {
            DataSlotRequests* dataRequests = DataSlotRequestBus::FindFirstHandler(m_owner.GetEntityId());

            if (dataRequests)
            {
                if (dataRequests->GetDataSlotType() == DataSlotType::Value && dataRequests->CanConvertToReference())
                {
                    dataRequests->ConvertToReference();
                }
                else if (dataRequests->GetDataSlotType() == DataSlotType::Reference && dataRequests->CanConvertToValue())
                {
                    dataRequests->ConvertToValue();
                }
            }
        }
    }

    ////////////////////////////
    // DataSlotLayoutComponent
    ////////////////////////////

    void DataSlotLayoutComponent::Reflect(AZ::ReflectContext* reflectContext)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflectContext);

        if (serializeContext)
        {
            serializeContext->Class<DataSlotLayoutComponent, AZ::Component>()
                ->Version(1)
                ;
        }
    }

    DataSlotLayoutComponent::DataSlotLayoutComponent()
        : m_layout(nullptr)
    {
    }

    void DataSlotLayoutComponent::Init()
    {
        SlotLayoutComponent::Init();

        m_layout = aznew DataSlotLayout((*this));
        SetLayout(m_layout);
    }

    void DataSlotLayoutComponent::Activate()
    {
        SlotLayoutComponent::Activate();
        m_layout->Activate();
    }

    void DataSlotLayoutComponent::Deactivate()
    {
        SlotLayoutComponent::Deactivate();
        m_layout->Deactivate();
    }    
}
