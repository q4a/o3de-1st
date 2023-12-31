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

#include <AzTest/AzTest.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/Component/ComponentApplication.h>
#include <AzCore/RTTI/BehaviorContext.h>

#include <EditorPythonBindings/EditorPythonBindingsSymbols.h>

#include "Source/PythonAssetBuilderSystemComponent.h"
#include <PythonAssetBuilder/PythonAssetBuilderBus.h>
#include "PythonBuilderTestShared.h"

namespace UnitTest
{
    class PythonBuilderProcessJobTest
        : public ScopedAllocatorSetupFixture
    {
    protected:
        AZStd::unique_ptr<AZ::ComponentApplication> m_app;
        AZ::Entity* m_systemEntity = nullptr;

        void SetUp() override
        {
            AZ::ComponentApplication::Descriptor appDesc;
            m_app = AZStd::make_unique<AZ::ComponentApplication>();
            m_systemEntity = m_app->Create(appDesc);
        }

        void TearDown() override
        {
            m_app.reset();
        }
    };

    TEST_F(PythonBuilderProcessJobTest, PythonBuilder_ProcessJob_ResultSuccess)
    {
        using namespace PythonAssetBuilder;
        using namespace AssetBuilderSDK;

        const AZ::Uuid builderId = RegisterAssetBuilder(m_app.get(), m_systemEntity);

        MockJobHandler mockJobHandler;
        mockJobHandler.BusConnect(builderId);

        AssetBuilderSDK::ProcessJobRequest request;
        request.m_builderGuid = builderId;
        request.m_sourceFileUUID = AZ::Uuid::CreateRandom();

        AssetBuilderSDK::ProcessJobResponse response;
        response.m_resultCode = AssetBuilderSDK::ProcessJobResult_NetworkIssue;

        PythonBuilderNotificationBus::EventResult(
            response,
            builderId,
            &PythonBuilderNotificationBus::Events::OnProcessJobRequest,
            request);

        EXPECT_EQ(AssetBuilderSDK::ProcessJobResult_Success, response.m_resultCode);
        EXPECT_EQ(0, mockJobHandler.m_onCancelCount);
    }

    TEST_F(PythonBuilderProcessJobTest, PythonBuilder_ProcessJob_ResultFailed)
    {
        using namespace PythonAssetBuilder;
        using namespace AssetBuilderSDK;

        const AZ::Uuid builderId = RegisterAssetBuilder(m_app.get(), m_systemEntity);

        MockJobHandler mockJobHandler;
        mockJobHandler.BusConnect(builderId);

        AssetBuilderSDK::ProcessJobRequest request;
        request.m_builderGuid = builderId;
        request.m_sourceFileUUID = AZ::Uuid::CreateNull();

        AssetBuilderSDK::ProcessJobResponse response;
        response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;

        PythonBuilderNotificationBus::EventResult(
            response,
            builderId,
            &PythonBuilderNotificationBus::Events::OnProcessJobRequest,
            request);

        EXPECT_EQ(AssetBuilderSDK::ProcessJobResult_Failed, response.m_resultCode);
        EXPECT_EQ(0, mockJobHandler.m_onCancelCount);
    }

    TEST_F(PythonBuilderProcessJobTest, PythonBuilder_ProcessJob_OnCancel)
    {
        using namespace PythonAssetBuilder;
        using namespace AssetBuilderSDK;

        const AZ::Uuid builderId = RegisterAssetBuilder(m_app.get(), m_systemEntity);

        MockJobHandler mockJobHandler;
        mockJobHandler.BusConnect(builderId);

        PythonBuilderNotificationBus::Event(builderId, &PythonBuilderNotificationBus::Events::OnCancel);
        EXPECT_EQ(1, mockJobHandler.m_onCancelCount);
    }

    TEST_F(PythonBuilderProcessJobTest, PythonBuilderRequestBus_Behavior_Exists)
    {
        using namespace PythonAssetBuilder;
        using namespace AssetBuilderSDK;

        RegisterAssetBuilder(m_app.get(), m_systemEntity);

        auto entry = m_app->GetBehaviorContext()->m_ebuses.find("PythonBuilderRequestBus");
        ASSERT_NE(m_app->GetBehaviorContext()->m_ebuses.end(), entry);
        EXPECT_NE(entry->second->m_events.end(), entry->second->m_events.find("WriteSliceFile"));
        EXPECT_NE(entry->second->m_events.end(), entry->second->m_events.find("CreateEditorEntity"));
    }
}
