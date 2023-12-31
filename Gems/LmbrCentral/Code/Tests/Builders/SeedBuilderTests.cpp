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

#include "LmbrCentral_precompiled.h"
#include <AzTest/AzTest.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzToolsFramework/Application/ToolsApplication.h>
#include <Builders/DependencyBuilder/SeedBuilderWorker/SeedBuilderWorker.h>
#include <AzCore/UserSettings/UserSettingsComponent.h>

class SeedBuilderTests
    : public ::testing::Test
{
    void SetUp() override
    {
        m_app.Start(AZ::ComponentApplication::Descriptor());

        // Without this, the user settings component would attempt to save on finalize/shutdown. Since the file is
        // shared across the whole engine, if multiple tests are run in parallel, the saving could cause a crash 
        // in the unit tests.
        AZ::UserSettingsComponentRequestBus::Broadcast(&AZ::UserSettingsComponentRequests::DisableSaveOnFinalize);

        const char* dir = m_app.GetExecutableFolder();
        AZ::IO::FileIOBase::GetInstance()->SetAlias("@root@", dir);
        AZ::UserSettingsComponentRequestBus::Broadcast(&AZ::UserSettingsComponentRequests::DisableSaveOnFinalize);
    }

    void TearDown() override
    {
        m_app.Stop();
    }

    AzToolsFramework::ToolsApplication m_app;
};

TEST_F(SeedBuilderTests, SeedBuilder_SourceDependency_Valid)
{
    DependencyBuilder::SeedBuilderWorker seedBuilderWorker;
    AssetBuilderSDK::CreateJobsRequest request;
    constexpr char testSeedFolder[] = "@root@/../Gems/LmbrCentral/Code/Tests/Seed";
    char resolvedPath[AZ_MAX_PATH_LEN];
    AZ::IO::FileIOBase::GetInstance()->ResolvePath(testSeedFolder, resolvedPath, AZ_MAX_PATH_LEN);
    request.m_watchFolder = resolvedPath;
    request.m_sourceFile = "TestSeedAssetList.seed";
    auto sourceDependencesResult = seedBuilderWorker.GetSourceDependencies(request);
    if (!sourceDependencesResult.IsSuccess())
    {
        return;
    }

    AZStd::vector<AssetBuilderSDK::SourceFileDependency> sourceFileDependencyList = sourceDependencesResult.TakeValue();

    ASSERT_EQ(sourceFileDependencyList.size(), 3);
    AZStd::unordered_set <AZ::Uuid> expectedSourceUUId{ AZ::Uuid::CreateString("2FB1A7EF-557C-577E-94E6-DC1F331E374F"),
        AZ::Uuid::CreateString("B74567AE-5C3F-5A33-B0DF-1DE40DC3C03C"), AZ::Uuid::CreateString("AD7E02A2-5658-5138-95F2-47347A9C1BE1") };
    AZStd::unordered_set <AZ::Uuid> actualSourceUUId;
    for (auto& sourceDependencyEntry : sourceFileDependencyList)
    {
        actualSourceUUId.insert(sourceDependencyEntry.m_sourceFileDependencyUUID);
    }

    ASSERT_THAT(expectedSourceUUId, actualSourceUUId);
}

TEST_F(SeedBuilderTests, SeedBuilder_EmptySourceDependency_Valid)
{
    DependencyBuilder::SeedBuilderWorker seedBuilderWorker;
    AssetBuilderSDK::CreateJobsRequest request;
    constexpr char testSeedFolder[] = "@root@/../Gems/LmbrCentral/Code/Tests/Seed";
    char resolvedPath[AZ_MAX_PATH_LEN];
    AZ::IO::FileIOBase::GetInstance()->ResolvePath(testSeedFolder, resolvedPath, AZ_MAX_PATH_LEN);
    request.m_watchFolder = resolvedPath;
    request.m_sourceFile = "EmptySeedAssetList.seed";
    auto sourceDependencesResult = seedBuilderWorker.GetSourceDependencies(request);
    if (!sourceDependencesResult.IsSuccess())
    {
        return;
    }

    AZStd::vector<AssetBuilderSDK::SourceFileDependency> sourceFileDependencyList = sourceDependencesResult.TakeValue();
    ASSERT_EQ(sourceFileDependencyList.size(), 0);
}
