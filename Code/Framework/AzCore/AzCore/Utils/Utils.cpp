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

#include <AzCore/Utils/Utils.h>
#include <AzCore/IO/ByteContainerStream.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/IO/GenericStreams.h>
#include <AzCore/IO/SystemFile.h>
#include <AzCore/IO/Path/Path.h>
#include <AzCore/StringFunc/StringFunc.h>

namespace AZ::Utils
{
    ExecutablePathResult GetExecutableDirectory(char* exeStorageBuffer, size_t exeStorageSize)
    {
        AZ::Utils::GetExecutablePathReturnType result = GetExecutablePath(exeStorageBuffer, exeStorageSize);
        if (result.m_pathStored != AZ::Utils::ExecutablePathResult::Success)
        {
            *exeStorageBuffer = '\0';
            return result.m_pathStored;
        }

        // If it contains the filename, zero out the last path separator character...
        if (result.m_pathIncludesFilename)
        {
            AZ::IO::PathView exePathView(exeStorageBuffer);
            AZStd::string_view exeDirectory = exePathView.ParentPath().Native();
            exeStorageBuffer[exeDirectory.size()] = '\0';
        }

        return result.m_pathStored;
    }

    AZ::Outcome<void, AZStd::string> WriteFile(AZStd::string_view content, AZStd::string_view filePath)
    {
        AZ::IO::FixedMaxPath filePathFixed = filePath; // Because FileIOStream requires a null-terminated string
        AZ::IO::FileIOStream stream(filePathFixed.c_str(), AZ::IO::OpenMode::ModeWrite);

        bool success = false;

        if (stream.IsOpen())
        {
            AZ::IO::SizeType bytesWritten = stream.Write(content.size(), content.data());

            if (bytesWritten == content.size())
            {
                success = true;
            }
        }

        if (success)
        {
            return AZ::Success();
        }
        else
        {
            return AZ::Failure(AZStd::string::format("Could not write to file '%s'", filePathFixed.c_str()));
        }
    }

    template<typename Container>
    AZ::Outcome<Container, AZStd::string> ReadFile(AZStd::string_view filePath, size_t maxFileSize)
    {
        IO::FileIOStream file;
        if (!file.Open(filePath.data(), IO::OpenMode::ModeRead))
        {
            return AZ::Failure(AZStd::string::format("Failed to open '%.*s'.", AZ_STRING_ARG(filePath)));
        }

        AZ::IO::SizeType length = file.GetLength();

        if (length > maxFileSize)
        {
            return AZ::Failure(AZStd::string{ "Data is too large." });
        }
        else if (length == 0)
        {
            return AZ::Failure(AZStd::string::format("Failed to load '%.*s'. File is empty.", AZ_STRING_ARG(filePath)));
        }

        Container fileContent;
        fileContent.resize(length);
        AZ::IO::SizeType bytesRead = file.Read(length, fileContent.data());
        file.Close();

        // Resize again just in case bytesRead is less than length for some reason
        fileContent.resize(bytesRead);

        return AZ::Success(AZStd::move(fileContent));
    }

    template AZ::Outcome<AZStd::string, AZStd::string> ReadFile(AZStd::string_view filePath, size_t maxFileSize);
    template AZ::Outcome<AZStd::vector<int8_t>, AZStd::string> ReadFile(AZStd::string_view filePath, size_t maxFileSize);
    template AZ::Outcome<AZStd::vector<uint8_t>, AZStd::string> ReadFile(AZStd::string_view filePath, size_t maxFileSize);


}
