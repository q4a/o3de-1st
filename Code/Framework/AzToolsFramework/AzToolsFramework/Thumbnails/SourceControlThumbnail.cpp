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

#include <AzFramework/StringFunc/StringFunc.h>
#include <AzToolsFramework/Thumbnails/SourceControlThumbnail.h>
#include <AzToolsFramework/SourceControl/SourceControlAPI.h>
#include <AzFramework/API/ApplicationAPI.h>

namespace AzToolsFramework
{
    namespace Thumbnailer
    {
        //////////////////////////////////////////////////////////////////////////
        // SourceControlThumbnailKey
        //////////////////////////////////////////////////////////////////////////
        SourceControlThumbnailKey::SourceControlThumbnailKey(const char* fileName)
            : ThumbnailKey()
            , m_fileName(fileName)
            , m_updateInterval(4)
        {
            m_nextUpdate = AZStd::chrono::system_clock::now();
        }

        const AZStd::string& SourceControlThumbnailKey::GetFileName() const
        {
            return m_fileName;
        }

        bool SourceControlThumbnailKey::UpdateThumbnail()
        {
            if (!IsReady() || !SourceControlThumbnail::ReadyForUpdate())
            {
                return false;
            }

            const auto now(AZStd::chrono::system_clock::now());
            if (m_nextUpdate >= now)
            {
                return false;
            }
            m_nextUpdate = now + m_updateInterval;
            emit UpdateThumbnailSignal();
            return true;
        }

        //////////////////////////////////////////////////////////////////////////
        // SourceControlThumbnail
        //////////////////////////////////////////////////////////////////////////
        static const char* WRITABLE_ICON_PATH = "Editor/Icons/AssetBrowser/Writable_16.svg";
        static const char* NONWRITABLE_ICON_PATH = "Editor/Icons/AssetBrowser/NonWritable_16.svg";

        bool SourceControlThumbnail::m_readyForUpdate = true;

        SourceControlThumbnail::SourceControlThumbnail(SharedThumbnailKey key, int thumbnailSize)
            : Thumbnail(key, thumbnailSize)
        {
            const char* engineRoot = nullptr;
            AzFramework::ApplicationRequests::Bus::BroadcastResult(engineRoot, &AzFramework::ApplicationRequests::GetEngineRoot);
            AZ_Assert(engineRoot, "Engine Root not initialized");

            AzFramework::StringFunc::Path::Join(engineRoot, WRITABLE_ICON_PATH, m_writableIconPath);
            AzFramework::StringFunc::Path::Join(engineRoot, NONWRITABLE_ICON_PATH, m_nonWritableIconPath);

            BusConnect();
        }

        SourceControlThumbnail::~SourceControlThumbnail()
        {
            BusDisconnect();
        }

        void SourceControlThumbnail::FileStatusChanged(const char* filename)
        {
            // when file status is changed, force instant update
            auto sourceControlKey = azrtti_cast<const SourceControlThumbnailKey*>(m_key.data());
            AZ_Assert(sourceControlKey, "Incorrect key type, excpected SourceControlThumbnailKey");

            AZStd::string myFileName(sourceControlKey->GetFileName());
            AzFramework::StringFunc::Path::Normalize(myFileName);
            if (AzFramework::StringFunc::Equal(myFileName.c_str(), filename))
            {
                Update();
            }
        }

        void SourceControlThumbnail::RequestSourceControlStatus()
        {
            auto sourceControlKey = azrtti_cast<const SourceControlThumbnailKey*>(m_key.data());
            AZ_Assert(sourceControlKey, "Incorrect key type, excpected SourceControlThumbnailKey");

            bool isSourceControlActive = false;
            SourceControlConnectionRequestBus::BroadcastResult(isSourceControlActive, &SourceControlConnectionRequests::IsActive);
            if (isSourceControlActive && SourceControlCommandBus::FindFirstHandler() != nullptr)
            {
                m_readyForUpdate = false;
                SourceControlCommandBus::Broadcast(&SourceControlCommands::GetFileInfo, sourceControlKey->GetFileName().c_str(),
                    AZStd::bind(&SourceControlThumbnail::SourceControlFileInfoUpdated, this, AZStd::placeholders::_1, AZStd::placeholders::_2));
            }
        }

        void SourceControlThumbnail::SourceControlFileInfoUpdated(bool succeeded, const SourceControlFileInfo& fileInfo)
        {
            if (succeeded)
            {
                if (fileInfo.HasFlag(AzToolsFramework::SCF_Writeable))
                {
                    m_icon = QIcon(m_writableIconPath.c_str());
                }
                else
                {
                    m_icon = QIcon(m_nonWritableIconPath.c_str());
                }
            }
            else
            {
                m_icon = QIcon();
            }
            m_readyForUpdate = true;
            emit Updated();
        }

        void SourceControlThumbnail::Update()
        {
            RequestSourceControlStatus();
        }

        bool SourceControlThumbnail::ReadyForUpdate()
        {
            return m_readyForUpdate;
        }

        //////////////////////////////////////////////////////////////////////////
        // SourceControlThumbnailCache
        //////////////////////////////////////////////////////////////////////////
        SourceControlThumbnailCache::SourceControlThumbnailCache()
            : ThumbnailCache<SourceControlThumbnail, SourceControlKeyHash, SourceControlKeyEqual>()
        {
        }

        SourceControlThumbnailCache::~SourceControlThumbnailCache() = default;

        const char* SourceControlThumbnailCache::GetProviderName() const
        {
            return ProviderName;
        }

        bool SourceControlThumbnailCache::IsSupportedThumbnail(SharedThumbnailKey key) const
        {
            return azrtti_istypeof<const SourceControlThumbnailKey*>(key.data());
        }

    } // namespace Thumbnailer
} // namespace AzToolsFramework

#include "Thumbnails/moc_SourceControlThumbnail.cpp"
