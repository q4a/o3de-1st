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

#include <EditorPythonBindings/EditorPythonBindingsSymbols.h>

#include "Source/PythonAssetBuilderSystemComponent.h"
#include <PythonAssetBuilder/PythonAssetBuilderBus.h>
#include <PythonAssetBuilder/PythonBuilderNotificationBus.h>
#include "PythonBuilderTestShared.h"

namespace UnitTest
{
    // fixtures

    class PythonBuilderCreateJobsTest
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

    // tests

    TEST_F(PythonBuilderCreateJobsTest, PythonBuilder_CreateJobs_Success)
    {
        using namespace PythonAssetBuilder;
        using namespace AssetBuilderSDK;

        const AZ::Uuid builderId = RegisterAssetBuilder(m_app.get(), m_systemEntity);

        MockJobHandler mockJobHandler;
        mockJobHandler.BusConnect(builderId);

        AssetBuilderSDK::CreateJobsRequest request;
        request.m_builderid = builderId;
        request.m_sourceFileUUID = AZ::Uuid::CreateRandom();

        AssetBuilderSDK::CreateJobsResponse response;
        response.m_result = AssetBuilderSDK::CreateJobsResultCode::Failed;

        PythonBuilderNotificationBus::EventResult(
            response,
            builderId,
            &PythonBuilderNotificationBus::Events::OnCreateJobsRequest,
            request);

        EXPECT_EQ(AssetBuilderSDK::CreateJobsResultCode::Success, response.m_result);
        EXPECT_EQ(0, mockJobHandler.m_onShutdownCount);
    }

    TEST_F(PythonBuilderCreateJobsTest, PythonBuilder_CreateJobs_Failed)
    {
        using namespace PythonAssetBuilder;
        using namespace AssetBuilderSDK;

        const AZ::Uuid builderId = RegisterAssetBuilder(m_app.get(), m_systemEntity);
        EXPECT_NE(AZ::Uuid::CreateNull(), builderId);

        MockJobHandler mockJobHandler;
        mockJobHandler.BusConnect(builderId);

        AssetBuilderSDK::CreateJobsRequest request;
        request.m_builderid = builderId;
        request.m_sourceFileUUID = AZ::Uuid::CreateNull();

        AssetBuilderSDK::CreateJobsResponse response;
        response.m_result = AssetBuilderSDK::CreateJobsResultCode::Success;

        PythonBuilderNotificationBus::EventResult(
            response,
            request.m_builderid,
            &PythonBuilderNotificationBus::Events::OnCreateJobsRequest,
            request);

        EXPECT_EQ(AssetBuilderSDK::CreateJobsResultCode::Failed, response.m_result);
        EXPECT_EQ(0, mockJobHandler.m_onShutdownCount);
    }

    TEST_F(PythonBuilderCreateJobsTest, PythonBuilder_CreateJobs_OnShutdown)
    {
        using namespace PythonAssetBuilder;
        using namespace AssetBuilderSDK;

        const AZ::Uuid builderId = RegisterAssetBuilder(m_app.get(), m_systemEntity);
        EXPECT_NE(AZ::Uuid::CreateNull(), builderId);

        MockJobHandler mockJobHandler;
        mockJobHandler.BusConnect(builderId);

        PythonBuilderNotificationBus::Event(builderId, &PythonBuilderNotificationBus::Events::OnShutdown);
        EXPECT_EQ(1, mockJobHandler.m_onShutdownCount);
    }
}
