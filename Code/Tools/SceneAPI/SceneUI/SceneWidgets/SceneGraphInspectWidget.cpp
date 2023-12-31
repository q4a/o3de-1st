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

#include <QSplitter>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <SceneWidgets/ui_SceneGraphInspectWidget.h>
#include <SceneAPI/SceneCore/DataTypes/IGraphObject.h>
#include <SceneAPI/SceneCore/Events/GraphMetaInfoBus.h>
#include <SceneAPI/SceneUI/SceneWidgets/SceneGraphWidget.h>
#include <SceneAPI/SceneUI/SceneWidgets/SceneGraphInspectWidget.h>
#include <AzToolsFramework/UI/PropertyEditor/ReflectedPropertyEditor.hxx>

namespace AZ
{
    namespace SceneAPI
    {
        namespace UI
        {
            AZ_CLASS_ALLOCATOR_IMPL(SceneGraphInspectWidget, SystemAllocator, 0);

            SceneGraphInspectWidget::SceneGraphInspectWidget(const Containers::Scene& scene, QWidget* parent, SerializeContext* context)
                : QWidget(parent)
                , ui(new Ui::SceneGraphInspectWidget())
                , m_graphView(aznew SceneGraphWidget(scene, this))
                , m_propertyEditor(aznew AzToolsFramework::ReflectedPropertyEditor(this))
                , m_context(context)
            {
                ui->setupUi(this);
                
                if (!m_context)
                {
                    ComponentApplicationBus::BroadcastResult(m_context, &ComponentApplicationBus::Events::GetSerializeContext);
                }

                m_propertyEditor->Setup(m_context, nullptr, true, 100);
                m_propertyEditor->setEnabled(false);

                m_graphView->Build();
                
                ui->m_splitter->insertWidget(0, m_graphView.data());
                ui->m_propertyEditorLayout->addWidget(m_propertyEditor.data());

                connect(m_graphView.data(), &SceneGraphWidget::SelectionChanged, this, &SceneGraphInspectWidget::OnSelectionChanged);
            }

            SceneGraphInspectWidget::~SceneGraphInspectWidget() = default;

            void SceneGraphInspectWidget::OnSelectionChanged(AZStd::shared_ptr<const DataTypes::IGraphObject> item)
            {
                using namespace AZ::SceneAPI::Events;

                if (item)
                {
                    if (m_context)
                    {
                        // Only try to show if there's a registered editor for the class.
                        const SerializeContext::ClassData* classData = m_context->FindClassData(item->RTTI_GetType());
                        if (classData && classData->m_editData)
                        {
                            // The reflected property editor is made for editing (as the name suggest) not inspecting, 
                            //      therefore it only accepts objects it can modify.
                            DataTypes::IGraphObject* mutableItem = const_cast<DataTypes::IGraphObject*>(item.get());
                            m_propertyEditor->ClearInstances();
                            m_propertyEditor->AddInstance(mutableItem, item->RTTI_GetType());
                            m_propertyEditor->InvalidateAll();
                            m_propertyEditor->ExpandAll();

                            ui->m_infoStack->setCurrentIndex(1);
                            return;
                        }
                    }
                    
                    AZStd::string description = "<html><head/><body><p>";
                    if (item->RTTI_GetTypeName())
                    {
                        description += "<b>";
                        description += item->RTTI_GetTypeName();
                        description += "</b></p><p>";
                    }

                    AZStd::string tooltip;
                    GraphMetaInfoBus::Broadcast(&GraphMetaInfoBus::Events::GetToolTip, tooltip, item.get());
                    description += tooltip.empty() ? "No information found for this node." : tooltip;

                    description += "</p></body></html>";
                    ui->m_noSelectionLabel->setText(description.c_str());
                }
                else
                {
                    ui->m_noSelectionLabel->setText("Empty node selected.");
                }
                ui->m_infoStack->setCurrentIndex(0);
            }
        } // UI
    } // SceneAPI
} // AZ

#include <SceneWidgets/moc_SceneGraphInspectWidget.cpp>
