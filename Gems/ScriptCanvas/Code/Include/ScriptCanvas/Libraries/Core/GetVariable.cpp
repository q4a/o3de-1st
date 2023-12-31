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

#include <ScriptCanvas/Core/ScriptCanvasBus.h>
#include <ScriptCanvas/Libraries/Core/GetVariable.h>
#include <ScriptCanvas/Variable/VariableBus.h>
#include <Libraries/Core/MethodUtility.h>

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace Core
        {
            void GetVariableNode::OnInit()
            {
                VariableNodeRequestBus::Handler::BusConnect(GetEntityId());
            }

            void GetVariableNode::OnPostActivate()
            {
                if (m_variableId.IsValid())
                {
                    RefreshPropertyFunctions();
                    PopulateNodeType();

                    if (GetExecutionType() == ScriptCanvas::ExecutionType::Editor)
                    {
                        VariableNotificationBus::Handler::BusConnect(GraphScopedVariableId(GetOwningScriptCanvasId(), m_variableId));                        
                    }
                    else
                    {
                        GraphVariable* variable = FindGraphVariable(m_variableId);

                        if (variable)
                        {
                            m_variableName = variable->GetVariableName();
                            variable->ConfigureDatumView(m_variableView);
                        }
                    }
                }
            }

            void GetVariableNode::OnInputSignal(const SlotId& slotID)
            {
                if (slotID == GetSlotId("In"))
                {
                    SC_EXECUTION_TRACE_ANNOTATE_NODE((*this), CreateAnnotationData());

                    if (m_variableView.IsValid())
                    {
                        Slot* resultSlot = GetSlot(m_variableDataOutSlotId);
                        if (resultSlot)
                        {
                            const Datum* inputDatum = m_variableView.GetDatum();

                            PushOutput(*inputDatum, *resultSlot);

                            // Push the data for each property slot out as well
                            for (auto&& propertyAccount : m_propertyAccounts)
                            {
                                Slot* propertySlot = GetSlot(propertyAccount.m_propertySlotId);
                                if (propertySlot && propertyAccount.m_getterFunction)
                                {
                                    auto outputOutcome = propertyAccount.m_getterFunction(*inputDatum);
                                    if (!outputOutcome)
                                    {
                                        SCRIPTCANVAS_REPORT_ERROR((*this), outputOutcome.TakeError().data());
                                        return;
                                    }
                                    PushOutput(outputOutcome.TakeValue(), *propertySlot);
                                }
                            }
                        }
                    }

                    SignalOutput(GetSlotId("Out"));
                }
            }

            void GetVariableNode::CollectVariableReferences(AZStd::unordered_set< ScriptCanvas::VariableId >& variableIds) const
            {
                if (m_variableId.IsValid())
                {
                    variableIds.insert(m_variableId);
                }

                return Node::CollectVariableReferences(variableIds);
            }

            bool GetVariableNode::ContainsReferencesToVariables(const AZStd::unordered_set< ScriptCanvas::VariableId >& variableIds) const
            {
                if (m_variableId.IsValid() && variableIds.count(m_variableId) > 0)
                {
                    return true;
                }

                return Node::ContainsReferencesToVariables(variableIds);
            }

            bool GetVariableNode::RemoveVariableReferences(const AZStd::unordered_set< ScriptCanvas::VariableId >& variableIds)
            {
                // These nodes should just be deleted when the variable they reference is removed. Don't try to 
                // update the variable they reference.
                if (m_variableId.IsValid() && variableIds.count(m_variableId) > 0)
                {
                    return false;
                }

                return Node::RemoveVariableReferences(variableIds);
            }

            void GetVariableNode::SetId(const VariableId& variableDatumId)
            {
                if (m_variableId != variableDatumId)
                {
                    VariableId oldVariableId = m_variableId;
                    m_variableId = variableDatumId;

                    VariableNotificationBus::Handler::BusDisconnect();

                    ScriptCanvas::Data::Type oldType = ScriptCanvas::Data::Type::Invalid();

                    if (m_variableDataOutSlotId.IsValid())
                    {
                        oldType = GetSlotDataType(m_variableDataOutSlotId);
                    }

                    ScriptCanvas::Data::Type newType = ScriptCanvas::Data::Type::Invalid();
                    VariableRequestBus::EventResult(newType, GetScopedVariableId(), &VariableRequests::GetType);

                    if (oldType != newType)
                    {
                        ScopedBatchOperation scopedBatchOperation(AZ_CRC("GetVariableIdChanged", 0x6e59a64f));
                        RemoveOutputSlot();
                        AddOutputSlot();
                    }

                    if (m_variableId.IsValid())
                    {
                        VariableNotificationBus::Handler::BusConnect(GetScopedVariableId());
                    }

                    VariableNodeNotificationBus::Event(GetEntityId(), &VariableNodeNotifications::OnVariableIdChanged, oldVariableId, m_variableId);

                    PopulateNodeType();
                }
            }

            const VariableId& GetVariableNode::GetId() const
            {
                return m_variableId;
            }

            const SlotId& GetVariableNode::GetDataOutSlotId() const
            {
                return m_variableDataOutSlotId;
            }

            const Datum* GetVariableNode::GetDatum() const
            {
                const GraphVariable* graphVariable = FindGraphVariable(m_variableId);
                return graphVariable ? graphVariable->GetDatum() : nullptr;
            }

            void GetVariableNode::AddOutputSlot()
            {
                if (m_variableId.IsValid())
                {
                    GraphScopedVariableId scopedVariableId = GetScopedVariableId();

                    AZStd::string_view varName;
                    Data::Type varType;
                    VariableRequestBus::EventResult(varName, scopedVariableId, &VariableRequests::GetName);
                    VariableRequestBus::EventResult(varType, scopedVariableId, &VariableRequests::GetType);

                    {
                        DataSlotConfiguration slotConfiguration;

                        slotConfiguration.m_name = Data::GetName(varType);
                        slotConfiguration.SetConnectionType(ConnectionType::Output);
                        slotConfiguration.SetType(varType);

                        m_variableDataOutSlotId = AddSlot(slotConfiguration);
                    }

                    AddPropertySlots(varType);
                }
            }

            void GetVariableNode::AddPropertySlots(const Data::Type& type)
            {
                Data::GetterContainer getterFunctions = Data::ExplodeToGetters(type);
                for (const auto& getterWrapperPair : getterFunctions)
                {
                    const AZStd::string& propertyName = getterWrapperPair.first;
                    const Data::GetterWrapper& getterWrapper = getterWrapperPair.second;
                    Data::PropertyMetadata propertyAccount;
                    propertyAccount.m_propertyType = getterWrapper.m_propertyType;
                    propertyAccount.m_propertyName = propertyName;

                    {
                        DataSlotConfiguration slotConfiguration;

                        slotConfiguration.m_name = AZStd::string::format("%s: %s", propertyName.data(), Data::GetName(getterWrapper.m_propertyType).data());
                        slotConfiguration.SetType(getterWrapper.m_propertyType);
                        slotConfiguration.SetConnectionType(ConnectionType::Output);

                        propertyAccount.m_propertySlotId = AddSlot(slotConfiguration);
                    }

                    propertyAccount.m_getterFunction = getterWrapper.m_getterFunction;
                    m_propertyAccounts.push_back(propertyAccount);
                }
            }

            void GetVariableNode::ClearPropertySlots()
            {
                auto oldPropertyAccounts = AZStd::move(m_propertyAccounts);
                m_propertyAccounts.clear();
                for (auto&& propertyAccount : oldPropertyAccounts)
                {
                    RemoveSlot(propertyAccount.m_propertySlotId);
                }
            }

            void GetVariableNode::RefreshPropertyFunctions()
            {
                GraphVariable* variable = FindGraphVariable(m_variableId);

                if (variable == nullptr)
                {
                    return;
                }

                Data::Type sourceType = variable->GetDataType();

                if (!sourceType.IsValid())
                {
                    return;
                }

                auto getterWrapperMap = Data::ExplodeToGetters(sourceType);

                for (auto&& propertyAccount : m_propertyAccounts)
                {
                    if (!propertyAccount.m_getterFunction)
                    {
                        auto foundPropIt = getterWrapperMap.find(propertyAccount.m_propertyName);
                        if (foundPropIt != getterWrapperMap.end() && propertyAccount.m_propertyType.IS_A(foundPropIt->second.m_propertyType))
                        {
                            propertyAccount.m_getterFunction = foundPropIt->second.m_getterFunction;
                        }
                        else
                        {
                            AZ_Error("Script Canvas", false, "Property (%s : %s) getter method could not be found in Data::PropertyTraits or the property type has changed."
                                " Output will not be pushed on the property's slot.",
                                propertyAccount.m_propertyName.c_str(), Data::GetName(propertyAccount.m_propertyType).data());
                        }
                    }
                }
            }

            void GetVariableNode::RemoveOutputSlot()
            {
                ClearPropertySlots();
                SlotId oldVariableDataOutSlotId;
                AZStd::swap(oldVariableDataOutSlotId, m_variableDataOutSlotId);

                if (oldVariableDataOutSlotId.IsValid())
                {
                    RemoveSlot(oldVariableDataOutSlotId);
                }
            }

            GraphScopedVariableId GetVariableNode::GetScopedVariableId() const
            {
                return GraphScopedVariableId(GetOwningScriptCanvasId(), m_variableId);
            }

            void GetVariableNode::OnIdChanged(const VariableId& oldVariableId)
            {
                if (m_variableId != oldVariableId)
                {
                    VariableId newVariableId = m_variableId;
                    m_variableId = oldVariableId;
                    SetId(newVariableId);
                }
            }

            AZStd::vector<AZStd::pair<VariableId, AZStd::string>> GetVariableNode::GetGraphVariables() const
            {
                AZStd::vector<AZStd::pair<VariableId, AZStd::string>> varNameToIdList;

                if (m_variableId.IsValid())
                {
                    ScriptCanvas::Data::Type baseType = ScriptCanvas::Data::Type::Invalid();
                    VariableRequestBus::EventResult(baseType, GetScopedVariableId(), &VariableRequests::GetType);

                    const GraphVariableMapping* variableMap = GetRuntimeBus()->GetVariables();

                    if (variableMap && baseType.IsValid())
                    {
                        for (const auto& variablePair : *variableMap)
                        {
                            ScriptCanvas::Data::Type variableType = variablePair.second.GetDatum()->GetType();

                            if (variableType == baseType)
                            {
                                varNameToIdList.emplace_back(variablePair.first, variablePair.second.GetVariableName());
                            }
                        }

                        AZStd::sort_heap(varNameToIdList.begin(), varNameToIdList.end(), [](const AZStd::pair<VariableId, AZStd::string>& lhs, const AZStd::pair<VariableId, AZStd::string>& rhs)
                        {
                            return lhs.second < rhs.second;
                        });
                    }
                }

                return varNameToIdList;
            }

            void GetVariableNode::OnVariableRemoved()
            {
                VariableNotificationBus::Handler::BusDisconnect();
                VariableId removedVariableId;
                AZStd::swap(removedVariableId, m_variableId);
                {
                    ScopedBatchOperation scopedBatchOperation(AZ_CRC("GetVariableRemoved", 0xf2d1d2eb));
                    RemoveOutputSlot();
                }
                VariableNodeNotificationBus::Event(GetEntityId(), &VariableNodeNotifications::OnVariableRemovedFromNode, removedVariableId);
            }

            AnnotateNodeSignal GetVariableNode::CreateAnnotationData()
            {
                AZ::EntityId assetNodeId = GetRuntimeBus()->FindAssetNodeIdByRuntimeNodeId(GetEntityId());
                return AnnotateNodeSignal(CreateGraphInfo(GetOwningScriptCanvasId(), GetGraphIdentifier()), AnnotateNodeSignal::AnnotationLevel::Info, m_variableName, AZ::NamedEntityId(assetNodeId, GetNodeName()));
            }
        }
    }
}
