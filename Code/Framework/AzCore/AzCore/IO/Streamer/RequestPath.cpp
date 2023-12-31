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

#include <AzCore/Debug/Trace.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/IO/Streamer/RequestPath.h>

namespace AZ
{
    namespace IO
    {
        // RequestPath
        RequestPath::RequestPath(const RequestPath& rhs)
        {
            m_path = rhs.m_path;
            m_absolutePathHash = rhs.m_absolutePathHash;
            m_relativePathOffset = rhs.m_relativePathOffset;
        }

        RequestPath::RequestPath(RequestPath&& rhs)
        {
            m_path = AZStd::move(rhs.m_path);
            m_absolutePathHash = rhs.m_absolutePathHash;
            m_relativePathOffset = rhs.m_relativePathOffset;
        }

        RequestPath& RequestPath::operator=(const RequestPath& rhs)
        {
            m_path = rhs.m_path;
            m_absolutePathHash = rhs.m_absolutePathHash;
            m_relativePathOffset = rhs.m_relativePathOffset;
            return *this;
        }

        RequestPath& RequestPath::operator=(RequestPath&& rhs)
        {
            m_path = AZStd::move(rhs.m_path);
            m_absolutePathHash = rhs.m_absolutePathHash;
            m_relativePathOffset = rhs.m_relativePathOffset;
            return *this;
        }

        bool RequestPath::operator==(const RequestPath& rhs) const
        {
            if (m_path.empty() || rhs.m_path.empty())
            {
                return m_path.empty() == rhs.m_path.empty();
            }

            ResolvePath();
            rhs.ResolvePath();
            if (m_absolutePathHash != rhs.m_absolutePathHash)
            {
                return false;
            }
            else
            {
                return m_path == rhs.m_path;
            }
        }

        bool RequestPath::operator!=(const RequestPath& rhs) const
        {
            return !operator==(rhs);
        }

        void RequestPath::InitFromRelativePath(AZStd::string path)
        {
            m_path = AZStd::move(path);
            m_relativePathOffset = FindAliasOffset(m_path);
            m_absolutePathHash = s_emptyPathHash;
        }

        void RequestPath::InitFromAbsolutePath(AZStd::string path)
        {
            m_path = AZStd::move(path);
            m_relativePathOffset = 0;
            m_absolutePathHash = AZStd::hash<AZStd::string>{}(m_path);
        }

        bool RequestPath::IsValid() const
        {
            ResolvePath();
            return m_absolutePathHash != s_invalidPathHash;
        }

        void RequestPath::Clear()
        {
            m_path.clear();
            m_absolutePathHash = s_emptyPathHash;
            m_relativePathOffset = 0;
        }

        const char* RequestPath::GetAbsolutePath() const
        {
            ResolvePath();
            return m_path.c_str();
        }

        const char* RequestPath::GetRelativePath() const
        {
            // In case m_path has not been resolved it holds the relative path with the path offset set to exclude the alias.
            // If m_path has been resolved it holds the absolute path with the path offset set to the start of the start of the relative part.
            return m_path.c_str() + m_relativePathOffset;
        }

        size_t RequestPath::GetHash() const
        {
            return m_absolutePathHash;
        }

        void RequestPath::ResolvePath() const
        {
            if (m_absolutePathHash == s_emptyPathHash)
            {
                AZ_Assert(FileIOBase::GetInstance(),
                    "Trying to resolve a path in RequestPath before the low level file system has been initialized.");

                char fullPath[AZ::IO::MaxPathLength];
                if (!FileIOBase::GetInstance()->ResolvePath(m_path.c_str(), fullPath, AZ_ARRAY_SIZE(fullPath)))
                {
                    m_absolutePathHash = s_invalidPathHash;
                    return;
                }

                size_t relativePathLength = m_path.length() - m_relativePathOffset;
                m_path = fullPath;
                m_absolutePathHash = AZStd::hash<AZStd::string>{}(m_path);
                if (m_path.length() >= relativePathLength)
                {
                    m_relativePathOffset = m_path.length() - relativePathLength;
                }
                else
                {
                    m_absolutePathHash = s_invalidPathHash;
                }
            }
        }

        size_t RequestPath::FindAliasOffset(const AZStd::string& path) const
        {
            const char* pathChar = path.c_str();
            if (*pathChar == '@')
            {
                const char* alias = pathChar;
                alias++;
                while (*alias != '@' && *alias != 0)
                {
                    alias++;
                }
                if (*alias == '@')
                {
                    alias++;
                    return alias - pathChar;
                }
            }
            return 0;
        }
    } // namespace IO
} // namespace AZ
