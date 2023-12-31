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

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Jobs/JobFunction.h>
#include <AzCore/EBus/Results.h>
#include <AzCore/std/string/wildcard.h>

#include <AzFramework/Entity/EntityContextBus.h>

#include <AzToolsFramework/API/ViewPaneOptions.h>
#include <AzToolsFramework/UI/PropertyEditor/GenericComboBoxCtrl.h>

#include <GraphCanvas/GraphCanvasBus.h>

#include <Editor/SystemComponent.h>

#include <Editor/View/Windows/MainWindow.h>
#include <Editor/View/Dialogs/NewGraphDialog.h>
#include <Editor/View/Dialogs/Settings.h>
#include <Editor/Settings.h>

#include <ScriptCanvas/Bus/EditorScriptCanvasBus.h>
#include <ScriptCanvas/Core/Datum.h>
#include <ScriptCanvas/Data/DataRegistry.h>
#include <ScriptCanvas/Libraries/Libraries.h>
#include <ScriptCanvas/Variable/VariableCore.h>
#include <ScriptCanvas/Components/EditorGraph.h>
#include <ScriptCanvas/Components/EditorGraphVariableManagerComponent.h>

#include <LyViewPaneNames.h>

#include <QMenu>

#include <ScriptCanvas/View/EditCtrls/GenericLineEditCtrl.h>
#include <Editor/Framework/ScriptCanvasGraphUtilities.h>
#include <AzToolsFramework/AssetBrowser/Entries/SourceAssetBrowserEntry.h>

namespace ScriptCanvasEditor
{
    static const size_t cs_jobThreads = 1;

    SystemComponent::SystemComponent()
    {
    }

    SystemComponent::~SystemComponent()
    {
        AzToolsFramework::UnregisterViewPane(LyViewPane::ScriptCanvas);
        AzToolsFramework::EditorEvents::Bus::Handler::BusDisconnect();
    }

    void SystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            ScriptCanvasEditor::Settings::Reflect(serialize);

            serialize->Class<SystemComponent, AZ::Component>()
                ->Version(0)
                ;

            if (AZ::EditContext* ec = serialize->GetEditContext())
            {
                ec->Class<SystemComponent>("Script Canvas Editor", "Script Canvas Editor System Component")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Category, "Scripting")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("System", 0xc94d118b))
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ;
            }
        }
    }

    void SystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC("ScriptCanvasEditorService", 0x4fe2af98));
    }

    void SystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        (void)incompatible;
    }

    void SystemComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC("MemoryService", 0x5c4d473c)); // AZ::JobManager needs the thread pool allocator
        required.push_back(AZ_CRC("ScriptCanvasService", 0x41fd58f3));
        required.push_back(GraphCanvas::GraphCanvasRequestsServiceId);
        required.push_back(AZ_CRC("ScriptCanvasReflectService", 0xb3bfe139));
    }

    void SystemComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        (void)dependent;
    }

    void SystemComponent::Init()
    {
        AzToolsFramework::EditorEvents::Bus::Handler::BusConnect();
    }

    void SystemComponent::Activate()
    {
        m_assetTracker.Activate();
        AZ::JobManagerDesc jobDesc;
        for (size_t i = 0; i < cs_jobThreads; ++i)
        {
            jobDesc.m_workerThreads.push_back({ static_cast<int>(i) });
        }
        m_jobManager = AZStd::make_unique<AZ::JobManager>(jobDesc);
        m_jobContext = AZStd::make_unique<AZ::JobContext>(*m_jobManager);

        PopulateEditorCreatableTypes();

        m_propertyHandlers.emplace_back(AzToolsFramework::RegisterGenericComboBoxHandler<ScriptCanvas::VariableId>());

        SystemRequestBus::Handler::BusConnect();
        ScriptCanvasExecutionBus::Handler::BusConnect();
        AzToolsFramework::EditorEvents::Bus::Handler::BusConnect();
        AzToolsFramework::AssetBrowser::AssetBrowserInteractionNotificationBus::Handler::BusConnect();
    }

    void SystemComponent::NotifyRegisterViews()
    {
        QtViewOptions options;
        options.canHaveMultipleInstances = false;
        options.isPreview = true;
        options.showInMenu = true;

        AzToolsFramework::RegisterViewPane<ScriptCanvasEditor::MainWindow>(LyViewPane::ScriptCanvas, LyViewPane::CategoryTools, options);
    }

    void SystemComponent::Deactivate()
    {
        AzToolsFramework::AssetBrowser::AssetBrowserInteractionNotificationBus::Handler::BusDisconnect();
        AzToolsFramework::EditorEvents::Bus::Handler::BusDisconnect();
        ScriptCanvasExecutionBus::Handler::BusDisconnect();
        SystemRequestBus::Handler::BusDisconnect();

        for (auto&& propertyHandler : m_propertyHandlers)
        {
            AzToolsFramework::PropertyTypeRegistrationMessages::Bus::Broadcast(&AzToolsFramework::PropertyTypeRegistrationMessages::UnregisterPropertyType, propertyHandler.get());
        }
        m_propertyHandlers.clear();

        m_jobContext.reset();
        m_jobManager.reset();
        m_assetTracker.Deactivate();
    }

    void SystemComponent::AddAsyncJob(AZStd::function<void()>&& jobFunc)
    {
        auto* asyncFunction = AZ::CreateJobFunction(AZStd::move(jobFunc), true, m_jobContext.get());
        asyncFunction->Start();
    }

    void SystemComponent::GetEditorCreatableTypes(AZStd::unordered_set<ScriptCanvas::Data::Type>& outCreatableTypes)
    {
        outCreatableTypes.insert(m_creatableTypes.begin(), m_creatableTypes.end());
    }

    void SystemComponent::CreateEditorComponentsOnEntity(AZ::Entity* entity, const AZ::Data::AssetType& assetType)
    {
        if (entity)
        {
            auto graph = entity->CreateComponent<Graph>();
            graph->SetAssetType(assetType);

            entity->CreateComponent<EditorGraphVariableManagerComponent>(graph->GetScriptCanvasId());
        }
    }

    void SystemComponent::PopulateEditorGlobalContextMenu(QMenu* menu, const AZ::Vector2& point, int flags)
    {
        (void)point;
        (void)flags;

        AzToolsFramework::EntityIdList entitiesWithScriptCanvas;

        AzToolsFramework::EntityIdList selectedEntities;
        AzToolsFramework::EntityIdList highlightedEntities;

        EBUS_EVENT_RESULT(selectedEntities,
            AzToolsFramework::ToolsApplicationRequests::Bus,
            GetSelectedEntities);

        FilterForScriptCanvasEnabledEntities(selectedEntities, entitiesWithScriptCanvas);

        EBUS_EVENT_RESULT(highlightedEntities,
            AzToolsFramework::ToolsApplicationRequests::Bus,
            GetHighlightedEntities);

        FilterForScriptCanvasEnabledEntities(highlightedEntities, entitiesWithScriptCanvas);

        if (!entitiesWithScriptCanvas.empty())
        {
            QMenu* scriptCanvasMenu = nullptr;
            QAction* action = nullptr;

            // For entities with script canvas component, create a context menu to open any existing script canvases within each selected entity.
            for (const AZ::EntityId& entityId : entitiesWithScriptCanvas)
            {
                if (!scriptCanvasMenu)
                {
                    menu->addSeparator();
                    scriptCanvasMenu = menu->addMenu(QObject::tr("Edit Script Canvas"));
                    scriptCanvasMenu->setEnabled(false);
                    menu->addSeparator();
                }

                AZ::Entity* entity = nullptr;
                EBUS_EVENT_RESULT(entity, AZ::ComponentApplicationBus, FindEntity, entityId);

                if (entity)
                {
                    AZ::EBusAggregateResults<AZ::Data::AssetId> assetIds;
                    EditorContextMenuRequestBus::EventResult(assetIds, entity->GetId(), &EditorContextMenuRequests::GetAssetId);

                    if (!assetIds.values.empty())
                    {
                        QMenu* entityMenu = scriptCanvasMenu;
                        if (entitiesWithScriptCanvas.size() > 1)
                        {
                            scriptCanvasMenu->setEnabled(true);
                            entityMenu = scriptCanvasMenu->addMenu(entity->GetName().c_str());
                            entityMenu->setEnabled(false);
                        }

                        AZStd::unordered_set< AZ::Data::AssetId > usedIds;

                        for (const auto& assetId : assetIds.values)
                        {
                            if (!assetId.IsValid() || usedIds.count(assetId) != 0)
                            {
                                continue;
                            }

                            entityMenu->setEnabled(true);

                            usedIds.insert(assetId);

                            AZStd::string rootPath;
                            AZ::Data::AssetInfo assetInfo = AssetHelpers::GetAssetInfo(assetId, rootPath);

                            AZStd::string displayName;
                            AZ::StringFunc::Path::GetFileName(assetInfo.m_relativePath.c_str(), displayName);

                            action = entityMenu->addAction(QString("%1").arg(QString(displayName.c_str())));

                            QObject::connect(action, &QAction::triggered, [assetId]
                            {
                                AzToolsFramework::OpenViewPane(LyViewPane::ScriptCanvas);
                                GeneralRequestBus::Broadcast(&GeneralRequests::OpenScriptCanvasAsset, assetId, -1);
                            });
                        }
                    }
                }
            }
        }
    }

    void SystemComponent::FilterForScriptCanvasEnabledEntities(AzToolsFramework::EntityIdList& sourceList, AzToolsFramework::EntityIdList& targetList)
    {
        for (const AZ::EntityId& entityId : sourceList)
        {
            if (entityId.IsValid())
            {
                if (EditorContextMenuRequestBus::FindFirstHandler(entityId))
                {
                    if (targetList.end() == AZStd::find(targetList.begin(), targetList.end(), entityId))
                    {
                        targetList.push_back(entityId);
                    }
                }
            }
        }
    }

    AzToolsFramework::AssetBrowser::SourceFileDetails SystemComponent::GetSourceFileDetails(const char* fullSourceFileName)
    {
        if (AZStd::wildcard_match("*.scriptcanvas", fullSourceFileName))
        {
            return AzToolsFramework::AssetBrowser::SourceFileDetails("Editor/Icons/AssetBrowser/ScriptCanvas_16.png");
        }

        // not one of our types.
        return AzToolsFramework::AssetBrowser::SourceFileDetails();
    }

    void SystemComponent::AddSourceFileOpeners(const char* fullSourceFileName, [[maybe_unused]] const AZ::Uuid& sourceUUID, AzToolsFramework::AssetBrowser::SourceFileOpenerList& openers)
    {
        using namespace AzToolsFramework;
        using namespace AzToolsFramework::AssetBrowser;

        bool isScriptCanvasAsset = false;
        ScriptCanvasAssetDescription scriptCanvasAssetDescription;
        if (AZStd::wildcard_match(AZStd::string::format("*%s", scriptCanvasAssetDescription.GetExtensionImpl()).c_str(), fullSourceFileName))
        {
            isScriptCanvasAsset = true;
        }

        ScriptCanvasFunctionDescription scriptCanvasFunctionAssetDescription;
        if (!isScriptCanvasAsset && AZStd::wildcard_match(AZStd::string::format("*%s", scriptCanvasFunctionAssetDescription.GetExtensionImpl()).c_str(), fullSourceFileName))
        {
            isScriptCanvasAsset = true;
        }

        if (isScriptCanvasAsset)
        {
            auto scriptCanvasEditorCallback = [this]([[maybe_unused]] const char* fullSourceFileNameInCall, const AZ::Uuid& sourceUUIDInCall)
            {
                AZ::Outcome<int, AZStd::string> openOutcome = AZ::Failure(AZStd::string());
                const SourceAssetBrowserEntry* fullDetails = SourceAssetBrowserEntry::GetSourceByUuid(sourceUUIDInCall);
                if (fullDetails)
                {
                    AzToolsFramework::OpenViewPane(LyViewPane::ScriptCanvas);

                    AzToolsFramework::EditorRequests::Bus::Broadcast(&AzToolsFramework::EditorRequests::OpenViewPane, "Script Canvas");
                    GeneralRequestBus::BroadcastResult(openOutcome, &GeneralRequests::OpenScriptCanvasAsset, sourceUUIDInCall, -1);
                }
            };

            openers.push_back({ "Lumberyard_ScriptCanvasEditor", "Open In Script Canvas Editor...", QIcon(), scriptCanvasEditorCallback });
        }
    }

    void SystemComponent::OnUserSettingsActivated()
    {
        PopulateEditorCreatableTypes();
    }

    void SystemComponent::PopulateEditorCreatableTypes()
    {
        AZ::BehaviorContext* behaviorContext{};
        AZ::ComponentApplicationBus::BroadcastResult(behaviorContext, &AZ::ComponentApplicationRequests::GetBehaviorContext);
        AZ_Assert(behaviorContext, "Behavior Context should not be missing at this point");

        auto dataRegistry = ScriptCanvas::GetDataRegistry();
        for (const auto& scType : dataRegistry->m_creatableTypes)
        {
            if (scType.first.GetType() == ScriptCanvas::Data::eType::BehaviorContextObject)
            {
                if (const AZ::BehaviorClass* behaviorClass = AZ::BehaviorContextHelper::GetClass(behaviorContext, ScriptCanvas::Data::ToAZType(scType.first)))
                {
                    // BehaviorContext classes with the ExcludeFrom attribute with a value of the ExcludeFlags::Preview are not added to the list of 
                    // types that can be created in the editor
                    const AZ::u64 exclusionFlags = AZ::Script::Attributes::ExcludeFlags::Preview;
                    auto excludeClassAttributeData = azrtti_cast<const AZ::Edit::AttributeData<AZ::Script::Attributes::ExcludeFlags>*>(AZ::FindAttribute(AZ::Script::Attributes::ExcludeFrom, behaviorClass->m_attributes));
                    if (excludeClassAttributeData && (excludeClassAttributeData->Get(nullptr) & exclusionFlags))
                    {
                        continue;
                    }
                }
            }

            m_creatableTypes.insert(scType.first);
        }
    }

    Reporter SystemComponent::RunGraph(AZStd::string_view path)
    {
        Reporter reporter;
        ScriptCanvasEditor::RunGraph(path, ExecutionMode::Interpreted, DurationSpec(), reporter);
        return reporter;
    }

}
