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

#include <AzCore/Serialization/SerializeContext.h>
#include <AzTest/AzTest.h>
#include <AzToolsFramework/ComponentMode/ComponentModeCollection.h>

#include <AzToolsFramework/Application/ToolsApplication.h>
#include <AzToolsFramework/ViewportSelection/EditorInteractionSystemViewportSelectionRequestBus.h>
#include <AzToolsFramework/ToolsComponents/TransformComponent.h>
#include <AzToolsFramework/UI/PropertyEditor/EntityPropertyEditor.hxx>
#include <AzToolsFramework/API/EntityPropertyEditorRequestsBus.h>
#include <AzToolsFramework/ToolsComponents/EditorLockComponent.h>
#include <AzToolsFramework/ToolsComponents/EditorVisibilityComponent.h>
#include <AzToolsFramework/ViewportSelection/EditorDefaultSelection.h>

#include <AzCore/IO/Streamer/StreamerComponent.h>
#include <AzCore/Asset/AssetManagerComponent.h>
#include <AzCore/std/sort.h>

#include <QApplication>

#include <AzToolsFramework/UnitTest/AzToolsFrameworkTestHelpers.h>

namespace UnitTest
{
    using namespace AZ;
    using namespace AzToolsFramework;

    class EntityPropertyEditorTests
        : public ComponentApplication
    {
    public:
        void SetExecutableFolder(const char* path)
        {
            m_exeDirectory = path;
        }

        void SetSettingsRegistrySpecializations(SettingsRegistryInterface::Specializations& specializations) override
        {
            ComponentApplication::SetSettingsRegistrySpecializations(specializations);
            specializations.Append("test");
            specializations.Append("entitypropertyeditor");
        }
    };

    TEST(EntityPropertyEditorTests, PrioritySort_NonTransformAsFirstItem_TransformMovesToTopRemainderUnchanged)
    {
        ComponentApplication app;

        AZ::Entity::ComponentArrayType unorderedComponents;
        AZ::Entity::ComponentArrayType orderedComponents;

        ToolsApplication::Descriptor desc;
        desc.m_useExistingAllocator = true;
        desc.m_enableDrilling = false;
        ToolsApplication::StartupParameters startupParams;
        startupParams.m_allocator = &AZ::AllocatorInstance<AZ::SystemAllocator>::Get();

        Entity* systemEntity = app.Create(desc, startupParams);

        // Add more than 31 components, as we are testing the case where the sort fails when there are 32 or more items.
        const int numFillerItems = 32;

        for (int commentIndex = 0; commentIndex < numFillerItems; commentIndex++)
        {
            unorderedComponents.insert(unorderedComponents.begin(), systemEntity->CreateComponent(AZ::StreamerComponent::RTTI_Type()));
        }

        // Add a TransformComponent at the end which should be sorted to the beginning by the priority sort.
        AZ::Component* transformComponent = systemEntity->CreateComponent<AzToolsFramework::Components::TransformComponent>();
        unorderedComponents.push_back(transformComponent);
        
        //add an AssetDatabase component at the beginning which should end up as the second item once the TransformComponent pushes it down
        AZ::Component* secondComponent = systemEntity->CreateComponent(AZ::AssetManagerComponent::RTTI_Type());
        unorderedComponents.insert(unorderedComponents.begin(), secondComponent);

        orderedComponents = unorderedComponents;

        // When this sort happens, the transformComponent should move to the top, the AssetDatabase should move to second, the order of the others should be unaltered, 
        // merely moved to after the AssetDatabase.
        EntityPropertyEditor::SortComponentsByPriority(orderedComponents);

        // Check the component arrays are intact.
        EXPECT_EQ(orderedComponents.size(), unorderedComponents.size());
        EXPECT_GT(orderedComponents.size(), 2);

        // Check the transform is now the first component.
        EXPECT_EQ(orderedComponents[0], transformComponent);

        // Check the AssetDatabase is now second.
        EXPECT_EQ(orderedComponents[1], secondComponent);

        // Check the order of the remaining items is preserved.
        int firstUnsortedFillerIndex = 1;
        int firstSortedFillerIndex = 2;

        for (int index = 0; index < numFillerItems; index++)
        {
            EXPECT_EQ(orderedComponents[index + firstSortedFillerIndex], unorderedComponents[index + firstUnsortedFillerIndex]);
        }

    }

    void OpenPinnedInspector(const AzToolsFramework::EntityIdList& entities, EntityPropertyEditor* editor)
    {
        if (editor)
        {
            AzToolsFramework::EntityIdSet entitiesSet(entities.begin(), entities.end());
            editor->SetOverrideEntityIds(entitiesSet);
        }
    }

    class EntityPropertyEditorRequestTest
        : public ToolsApplicationFixture
    {
        void SetUpEditorFixtureImpl() override
        {
            m_editor = new EntityPropertyEditor();
            m_editorActions.Connect();

            m_entity1 = CreateDefaultEditorEntity("Entity1");
            m_entity2 = CreateDefaultEditorEntity("Entity2");
            m_entity3 = CreateDefaultEditorEntity("Entity3");
            m_entity4 = CreateDefaultEditorEntity("Entity4");
        }

        void TearDownEditorFixtureImpl() override
        {
            m_editorActions.Disconnect();
            delete m_editor;
        }

    public:
        EntityPropertyEditor* m_editor;
        TestEditorActions m_editorActions;
        EntityIdList m_entityIds;
        AZ::EntityId m_entity1;
        AZ::EntityId m_entity2;
        AZ::EntityId m_entity3;
        AZ::EntityId m_entity4;
    };

    TEST_F(EntityPropertyEditorRequestTest, GetSelectedEntitiesReturnsEitherSelectedEntitiesOrPinnedEntities)
    {
        EntityIdList entityIds;
        entityIds.insert(entityIds.begin(), { m_entity1, m_entity4 });

        // Set entity1 and entity4 as selected
        ToolsApplicationRequestBus::Broadcast(
            &ToolsApplicationRequests::SetSelectedEntities, entityIds);

        // Find the entities that are selected
        EntityIdList selectedEntityIds;
        AzToolsFramework::EntityPropertyEditorRequestBus::Broadcast(
            &AzToolsFramework::EntityPropertyEditorRequestBus::Events::GetSelectedEntities, selectedEntityIds);

        // Make sure the correct number of entities are returned
        EXPECT_EQ(selectedEntityIds.size(), 2);

        // Check they are the same entities as selected above
        int found = 0;
        for (auto& id : selectedEntityIds)
        {
            if (id == m_entity1)
            {
                found |= 1;
            }
            if (id == m_entity4)
            {
                found |= 8;
            }
        }
        EXPECT_EQ(found, 9);

        // Clear the selected entities
        entityIds.clear();
        ToolsApplicationRequestBus::Broadcast(
            &ToolsApplicationRequests::SetSelectedEntities, entityIds);

        // Open the pinned Inspector with a different set of entities
        entityIds.insert(entityIds.begin(), { m_entity1, m_entity2, m_entity3 });
        OpenPinnedInspector(entityIds, m_editor);

        // Find the entities that are selected
        AzToolsFramework::EntityPropertyEditorRequestBus::Broadcast(
            &AzToolsFramework::EntityPropertyEditorRequestBus::Events::GetSelectedEntities, selectedEntityIds);

        // Make sure the correct number of entities are returned
        EXPECT_EQ(selectedEntityIds.size(), 3);

        // Check they are the same entities as selected above
        found = 0;
        for (auto& id : selectedEntityIds)
        {
            if (id == m_entity1)
            {
                found |= 1;
            }
            if (id == m_entity2)
            {
                found |= 2;
            }
            if (id == m_entity3)
            {
                found |= 4;
            }
        }
        EXPECT_EQ(found, 7);
    }

    class LevelEntityPropertyEditorRequestTest
        : public ToolsApplicationFixture
        , public AzToolsFramework::EditorRequestBus::Handler
    {
        void SetUpEditorFixtureImpl() override
        {
            // Create an EntityPropertyEditor initialized to be a Level Inspector
            m_levelEditor = new EntityPropertyEditor(nullptr, {}, true);
            m_levelEntity = CreateDefaultEditorEntity("LevelEntity");

            // Level Inspector expects to have one override entity ID, which would normally be the root slice entity.
            AzToolsFramework::EntityIdSet entities;
            entities.insert(m_levelEntity);
            m_levelEditor->SetOverrideEntityIds(entities);

            m_editorActions.Connect();

            // Connect to the EditorRequestBus so that we can intercept calls checking whether or not a level is currently open.
            AzToolsFramework::EditorRequestBus::Handler::BusConnect();
        }

        void TearDownEditorFixtureImpl() override
        {
            AzToolsFramework::EditorRequestBus::Handler::BusDisconnect();

            m_editorActions.Disconnect();
            delete m_levelEditor;
        }

        // Mock out this call so that we can control whether or not the Level Inspector thinks a level is open.
        bool IsLevelDocumentOpen() override { return m_levelOpen; }

        // These are required by implementing the EditorRequestBus
        void BrowseForAssets(AssetBrowser::AssetSelectionModel& /*selection*/) override {}
        int GetIconTextureIdFromEntityIconPath([[maybe_unused]] const AZStd::string& entityIconPath) override { return 0; }
        bool DisplayHelpersVisible() override { return false; }

    public:
        EntityPropertyEditor* m_levelEditor;
        TestEditorActions m_editorActions;
        AZ::EntityId m_levelEntity;
        bool m_levelOpen = false;
    };

    TEST_F(LevelEntityPropertyEditorRequestTest, GetSelectedEntitiesForLevelInspectorWhenLevelIsNotLoaded)
    {
        m_levelOpen = false;

        // Find the entities that are selected
        EntityIdList selectedEntityIds;
        AzToolsFramework::EntityPropertyEditorRequestBus::Broadcast(
            &AzToolsFramework::EntityPropertyEditorRequestBus::Events::GetSelectedEntities, selectedEntityIds);

        // Make sure the correct number of entities are returned
        EXPECT_EQ(selectedEntityIds.size(), 0);
    }

    TEST_F(LevelEntityPropertyEditorRequestTest, GetSelectedEntitiesForLevelInspectorWhenLevelIsLoaded)
    {
        m_levelOpen = true;

        // Find the entities that are selected
        EntityIdList selectedEntityIds;

        // Find the entities that are selected
        AzToolsFramework::EntityPropertyEditorRequestBus::Broadcast(
            &AzToolsFramework::EntityPropertyEditorRequestBus::Events::GetSelectedEntities, selectedEntityIds);

        // Make sure the correct number of entities are returned
        EXPECT_EQ(selectedEntityIds.size(), 1);
        EXPECT_EQ(selectedEntityIds[0], m_levelEntity);
    }

}
