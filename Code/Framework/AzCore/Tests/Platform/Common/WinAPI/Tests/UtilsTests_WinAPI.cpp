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

#include <AzCore/IO/SystemFile.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/Utils/Utils.h>

namespace UnitTest
{
    class UtilsTestFixture
        : public ScopedAllocatorSetupFixture
    {
    };

    TEST_F(UtilsTestFixture, GetExecutablePath_ReturnsValidPath)
    {
        char executablePath[AZ_MAX_PATH_LEN];
        AZ::Utils::GetExecutablePathReturnType result = AZ::Utils::GetExecutablePath(executablePath, AZ_MAX_PATH_LEN);
        ASSERT_EQ(AZ::Utils::ExecutablePathResult::Success, result.m_pathStored);
        EXPECT_TRUE(result.m_pathIncludesFilename);
        EXPECT_GT(strlen(executablePath), 0);
    }

    TEST_F(UtilsTestFixture, GetExecutablePath_ReturnsBufferSizeTooLarge)
    {
        char executablePath[1];
        AZ::Utils::GetExecutablePathReturnType result = AZ::Utils::GetExecutablePath(executablePath, 1);
        EXPECT_EQ(AZ::Utils::ExecutablePathResult::BufferSizeNotLargeEnough, result.m_pathStored);
    }

    TEST_F(UtilsTestFixture, GetExecutableDirectory_ReturnsValidDirectory)
    {
        char executablePath[AZ_MAX_PATH_LEN];
        char executableDirectory[AZ_MAX_PATH_LEN];
        AZ::Utils::GetExecutablePathReturnType executablePathResult = AZ::Utils::GetExecutablePath(executablePath, AZ_ARRAY_SIZE(executablePath));
        AZ::Utils::ExecutablePathResult executableDirectoryResult = AZ::Utils::GetExecutableDirectory(executableDirectory, AZ_ARRAY_SIZE(executableDirectory));
        ASSERT_EQ(AZ::Utils::ExecutablePathResult::Success, executableDirectoryResult);

        EXPECT_TRUE(executablePathResult.m_pathIncludesFilename);
        EXPECT_GT(strlen(executablePath), 0);
        EXPECT_TRUE(AZStd::string_view(executablePath).starts_with(executableDirectory));
        EXPECT_LT(strlen(executableDirectory), strlen(executablePath));
    }

    TEST_F(UtilsTestFixture, ConvertToAbsolutePath_OnInvalidPath_Fails)
    {
        AZStd::fixed_string<AZ::Utils::MaxPathLength> invalidPath{ "Z:\\" };
        invalidPath.append(AZ::Utils::MaxPathLength - invalidPath.size(), '@');
        EXPECT_FALSE(AZ::Utils::ConvertToAbsolutePath(invalidPath));
    }

    TEST_F(UtilsTestFixture, ConvertToAbsolutePath_OnRelativePath_Succeeds)
    {
        AZStd::optional<AZStd::fixed_string<AZ::Utils::MaxPathLength>> absolutePath = AZ::Utils::ConvertToAbsolutePath("./");
        EXPECT_TRUE(absolutePath);
    }

    TEST_F(UtilsTestFixture, ConvertToAbsolutePath_OnAbsolutePath_Succeeds)
    {
        char executableDirectory[AZ::Utils::MaxPathLength];
        AZ::Utils::ExecutablePathResult result = AZ::Utils::GetExecutableDirectory(executableDirectory, AZ_ARRAY_SIZE(executableDirectory));
        EXPECT_EQ(AZ::Utils::ExecutablePathResult::Success, result);
        AZStd::optional<AZStd::fixed_string<AZ::Utils::MaxPathLength>> absolutePath = AZ::Utils::ConvertToAbsolutePath(executableDirectory);
        ASSERT_TRUE(absolutePath);
        EXPECT_STRCASEEQ(executableDirectory, absolutePath->c_str());
    }
}
