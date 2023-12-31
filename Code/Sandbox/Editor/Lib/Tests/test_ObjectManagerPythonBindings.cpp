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
#include "EditorDefs.h"
#include <AzTest/AzTest.h>
#include <Util/EditorUtils.h>
#include <AzCore/base.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Debug/TraceMessageBus.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzCore/UserSettings/UserSettingsComponent.h>

#include <AzToolsFramework/Application/ToolsApplication.h>
#include <Objects/ObjectManager.h>
#include <AzCore/RTTI/BehaviorContext.h>

namespace ObjectManagerPythonBindingsUnitTests
{

    class ObjectManagerPythonBindingsFixture
        : public testing::Test
    {
    public:
        AzToolsFramework::ToolsApplication m_app;

        void SetUp() override
        {
            AzFramework::Application::Descriptor appDesc;
            appDesc.m_enableDrilling = false;

            m_app.Start(appDesc);
            // Without this, the user settings component would attempt to save on finalize/shutdown. Since the file is
            // shared across the whole engine, if multiple tests are run in parallel, the saving could cause a crash 
            // in the unit tests.
            AZ::UserSettingsComponentRequestBus::Broadcast(&AZ::UserSettingsComponentRequests::DisableSaveOnFinalize);
            m_app.RegisterComponentDescriptor(AzToolsFramework::ObjectManagerFuncsHandler::CreateDescriptor());
        }

        void TearDown() override
        {
            m_app.Stop();
        }
    };

    TEST_F(ObjectManagerPythonBindingsFixture, ObjectManagerEditorCommands_ApiExists)
    {
        AZ::BehaviorContext* behaviorContext = m_app.GetBehaviorContext();
        ASSERT_TRUE(behaviorContext);

        EXPECT_TRUE(behaviorContext->m_methods.find("get_all_objects") != behaviorContext->m_methods.end());
        EXPECT_TRUE(behaviorContext->m_methods.find("get_names_of_selected_objects") != behaviorContext->m_methods.end());

        EXPECT_TRUE(behaviorContext->m_methods.find("select_object") != behaviorContext->m_methods.end());
        EXPECT_TRUE(behaviorContext->m_methods.find("unselect_objects") != behaviorContext->m_methods.end());
        EXPECT_TRUE(behaviorContext->m_methods.find("select_objects") != behaviorContext->m_methods.end());
        EXPECT_TRUE(behaviorContext->m_methods.find("get_num_selected") != behaviorContext->m_methods.end());
        EXPECT_TRUE(behaviorContext->m_methods.find("clear_selection") != behaviorContext->m_methods.end());

        EXPECT_TRUE(behaviorContext->m_methods.find("get_selection_center") != behaviorContext->m_methods.end());
        EXPECT_TRUE(behaviorContext->m_methods.find("get_selection_aabb") != behaviorContext->m_methods.end());

        EXPECT_TRUE(behaviorContext->m_methods.find("hide_object") != behaviorContext->m_methods.end());
        EXPECT_TRUE(behaviorContext->m_methods.find("is_object_hidden") != behaviorContext->m_methods.end());
        EXPECT_TRUE(behaviorContext->m_methods.find("unhide_object") != behaviorContext->m_methods.end());
        EXPECT_TRUE(behaviorContext->m_methods.find("hide_all_objects") != behaviorContext->m_methods.end());
        EXPECT_TRUE(behaviorContext->m_methods.find("unhide_all_objects") != behaviorContext->m_methods.end());

        EXPECT_TRUE(behaviorContext->m_methods.find("freeze_object") != behaviorContext->m_methods.end());
        EXPECT_TRUE(behaviorContext->m_methods.find("is_object_frozen") != behaviorContext->m_methods.end());
        EXPECT_TRUE(behaviorContext->m_methods.find("unfreeze_object") != behaviorContext->m_methods.end());

        EXPECT_TRUE(behaviorContext->m_methods.find("delete_object") != behaviorContext->m_methods.end());
        EXPECT_TRUE(behaviorContext->m_methods.find("delete_selected") != behaviorContext->m_methods.end());

        EXPECT_TRUE(behaviorContext->m_methods.find("get_position") != behaviorContext->m_methods.end());
        EXPECT_TRUE(behaviorContext->m_methods.find("set_position") != behaviorContext->m_methods.end());

        EXPECT_TRUE(behaviorContext->m_methods.find("get_rotation") != behaviorContext->m_methods.end());
        EXPECT_TRUE(behaviorContext->m_methods.find("set_rotation") != behaviorContext->m_methods.end());

        EXPECT_TRUE(behaviorContext->m_methods.find("get_scale") != behaviorContext->m_methods.end());
        EXPECT_TRUE(behaviorContext->m_methods.find("set_scale") != behaviorContext->m_methods.end());

        EXPECT_TRUE(behaviorContext->m_methods.find("rename_object") != behaviorContext->m_methods.end());
    }
}
