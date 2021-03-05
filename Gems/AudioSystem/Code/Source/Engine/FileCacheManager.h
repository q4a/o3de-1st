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
// Original file Copyright Crytek GMBH or its affiliates, used under license.

#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzCore/IO/GenericStreams.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/XML/rapidxml.h>

#include <IAudioInterfacesCommonData.h>
#include <ATLEntities.h>

// Forward declarations
class CCustomMemoryHeap;
struct IRenderAuxGeom;

namespace Audio
{
    ///////////////////////////////////////////////////////////////////////////////////////////////
    class AudioFileCacheManagerNotifications
        : public AZ::EBusTraits
    {
    public:
        virtual ~AudioFileCacheManagerNotifications() = default;

        ///////////////////////////////////////////////////////////////////////////////////////////
        // EBusTraits - Single Bus Address, Single Handler, Mutex, Queued
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const bool EnableEventQueue = true;
        using MutexType = AZStd::recursive_mutex;
        ///////////////////////////////////////////////////////////////////////////////////////////

        virtual void FinishAsyncStreamRequest(AZ::IO::FileRequestHandle request) = 0;
    };

    using AudioFileCacheManagerNotficationBus = AZ::EBus<AudioFileCacheManagerNotifications>;

    ///////////////////////////////////////////////////////////////////////////////////////////////
    class CFileCacheManager
        : public AudioFileCacheManagerNotficationBus::Handler
    {
    public:
        explicit CFileCacheManager(TATLPreloadRequestLookup& preloadRequests);
        ~CFileCacheManager() override;

        CFileCacheManager(const CFileCacheManager&) = delete;            // Copy protection
        CFileCacheManager& operator=(const CFileCacheManager&) = delete; // Copy protection

        // Public methods
        void Initialize();
        void Release();
        void Update();

        virtual TAudioFileEntryID TryAddFileCacheEntry(const AZ::rapidxml::xml_node<char>* fileXmlNode, const EATLDataScope dataScope, bool autoLoad);  // 'virtual' is needed for unit tests/mocking
        bool TryRemoveFileCacheEntry(const TAudioFileEntryID audioFileID, const EATLDataScope dataScope);
        void UpdateLocalizedFileCacheEntries();

        EAudioRequestStatus TryLoadRequest(const TAudioPreloadRequestID preloadRequestID, const bool loadSynchronously, const bool autoLoadOnly);
        EAudioRequestStatus TryUnloadRequest(const TAudioPreloadRequestID preloadRequestID);
        EAudioRequestStatus UnloadDataByScope(const EATLDataScope dataScope);

    #if !defined(AUDIO_RELEASE)
        void DrawDebugInfo(IRenderAuxGeom& auxGeom, const float posX, const float posY);
    #endif // !AUDIO_RELEASE

    private:
        // Internal type definitions.
        using TAudioFileEntries = ATLMapLookupType<TAudioFileEntryID, CATLAudioFileEntry*>;

        // Internal methods
        void AllocateHeap(const size_t size, const char* const usage);
        bool UncacheFileCacheEntryInternal(CATLAudioFileEntry* const audioFileEntry, const bool now, const bool ignoreUsedCount = false);
        bool DoesRequestFitInternal(const size_t requestSize);
        void UpdatePreloadRequestsStatus();
        bool FinishCachingFileInternal(CATLAudioFileEntry* const audioFileEntry, AZ::IO::SizeType sizeBytes,
            AZ::IO::IStreamerTypes::RequestStatus requestState);

        ///////////////////////////////////////////////////////////////////////////////////////////
        // AudioFileCacheManagerNotficationBus
        void FinishAsyncStreamRequest(AZ::IO::FileRequestHandle request) override;
        ///////////////////////////////////////////////////////////////////////////////////////////

        bool AllocateMemoryBlockInternal(CATLAudioFileEntry* const audioFileEntry);
        void UncacheFile(CATLAudioFileEntry* const audioFileEntry);
        void TryToUncacheFiles();
        void UpdateLocalizedFileEntryData(CATLAudioFileEntry* const audioFileEntry);
        bool TryCacheFileCacheEntryInternal(CATLAudioFileEntry* const audioFileEntry, const TAudioFileEntryID fileID, const bool loadSynchronously, const bool overrideUseCount = false, const size_t useCount = 0);

        // Internal members
        TATLPreloadRequestLookup& m_preloadRequests;
        TAudioFileEntries m_audioFileEntries;

        AZStd::unique_ptr<CCustomMemoryHeap> m_memoryHeap;
        size_t m_currentByteTotal;
        size_t m_maxByteTotal;
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////
    // Filter for drawing debug info to the screen
    enum EAudioFileCacheManagerDebugFilter
    {
        eAFCMDF_ALL = 0,
        eAFCMDF_GLOBALS         = AUDIO_BIT(6),   // a
        eAFCMDF_LEVEL_SPECIFICS = AUDIO_BIT(7),   // b
        eAFCMDF_USE_COUNTED     = AUDIO_BIT(8),   // c
        eAFCMDF_LOADED          = AUDIO_BIT(9),   // d
    };

} // namespace Audio