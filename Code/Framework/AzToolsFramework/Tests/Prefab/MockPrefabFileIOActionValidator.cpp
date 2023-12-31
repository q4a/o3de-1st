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

#include <Prefab/MockPrefabFileIOActionValidator.h>

#include <AzCore/JSON/prettywriter.h>

namespace UnitTest
{
    MockPrefabFileIOActionValidator::MockPrefabFileIOActionValidator()
    {
        // Cache the existing file io instance and build our mock file io
        m_priorFileIO = AZ::IO::FileIOBase::GetInstance();
        m_fileIOMock = AZStd::make_unique<testing::NiceMock<AZ::IO::MockFileIOBase>>();

        // Swap out current file io instance for our mock
        AZ::IO::FileIOBase::SetInstance(nullptr);
        AZ::IO::FileIOBase::SetInstance(m_fileIOMock.get());

        // Setup the default returns for our mock file io calls
        AZ::IO::MockFileIOBase::InstallDefaultReturns(*m_fileIOMock.get());
    }

    MockPrefabFileIOActionValidator::~MockPrefabFileIOActionValidator()
    {
        // Restore our original file io instance
        AZ::IO::FileIOBase::SetInstance(nullptr);
        AZ::IO::FileIOBase::SetInstance(m_priorFileIO);
    }

    void MockPrefabFileIOActionValidator::ReadPrefabDom(
        const AZStd::string& prefabFilePath,
        const AzToolsFramework::Prefab::PrefabDom& prefabFileContentDom,
        AZ::IO::ResultCode expectedReadResultCode,
        AZ::IO::ResultCode expectedOpenResultCode,
        AZ::IO::ResultCode expectedSizeResultCode,
        AZ::IO::ResultCode expectedCloseResultCode)
    {
        rapidjson::StringBuffer prefabFileContentBuffer;
        rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(prefabFileContentBuffer);
        prefabFileContentDom.Accept(writer);

        AZStd::string prefabFileContent(prefabFileContentBuffer.GetString());

        ReadPrefabDom(prefabFilePath, prefabFileContent,
            expectedReadResultCode, expectedOpenResultCode, expectedSizeResultCode, expectedCloseResultCode);
    }

    void MockPrefabFileIOActionValidator::ReadPrefabDom(
        const AZStd::string& prefabFilePath,
        const AZStd::string& prefabFileContent,
        AZ::IO::ResultCode expectedReadResultCode,
        AZ::IO::ResultCode expectedOpenResultCode,
        AZ::IO::ResultCode expectedSizeResultCode,
        AZ::IO::ResultCode expectedCloseResultCode)
    {
        AZ::IO::HandleType fileHandle = m_fileHandleCounter++;

        EXPECT_CALL(*m_fileIOMock.get(), Open(
            testing::StrEq(prefabFilePath.c_str()), testing::_, testing::_))
            .WillRepeatedly(
                testing::DoAll(
                    testing::SetArgReferee<2>(fileHandle),
                    testing::Return(AZ::IO::Result(expectedOpenResultCode))));

        EXPECT_CALL(*m_fileIOMock.get(), Size(fileHandle, testing::_))
            .WillRepeatedly(
                testing::DoAll(
                    testing::SetArgReferee<1>(prefabFileContent.size()),
                    testing::Return(AZ::IO::Result(expectedSizeResultCode))));

        EXPECT_CALL(*m_fileIOMock.get(), Read(fileHandle, testing::_, prefabFileContent.size(), testing::_, testing::_))
            .WillRepeatedly(testing::Invoke([prefabFileContent, expectedReadResultCode](AZ::IO::HandleType, void* buffer, AZ::u64, bool, AZ::u64* bytesRead)
                {
                    memcpy(buffer, prefabFileContent.data(), prefabFileContent.size());
                    *bytesRead = prefabFileContent.size();

                    return AZ::IO::Result(expectedReadResultCode);
                }));

        EXPECT_CALL(*m_fileIOMock.get(), Close(fileHandle))
            .WillRepeatedly(testing::Return(AZ::IO::Result(expectedCloseResultCode)));
    }
}
