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

#include <AK/SoundEngine/Common/AkMemoryMgr.h>
#include <AK/SoundEngine/Common/AkTypes.h>
#include <AK/AkWwiseSDKVersion.h>
#include <IAudioSystem.h>
#include <AudioEngineWwise_Traits_Platform.h>


#if AZ_TRAIT_AUDIOENGINEWWISE_PROVIDE_IMPL_SECONDARY_POOL
    #include <platform.h>
    #include <CryPool/PoolAlloc.h>

    using TMemoryPoolReferenced = NCryPoolAlloc::CThreadSafe<NCryPoolAlloc::CBestFit<NCryPoolAlloc::CReferenced<NCryPoolAlloc::CMemoryDynamic, 4 * 1024, true>, NCryPoolAlloc::CListItemReference>>;

    namespace Audio
    {
        extern TMemoryPoolReferenced g_audioImplMemoryPoolSecondary_wwise;
    }
#endif // AZ_TRAIT_AUDIOENGINEWWISE_PROVIDE_IMPL_SECONDARY_POOL


#define WWISE_IMPL_VERSION_STRING   "Wwise " AK_WWISESDK_VERSIONNAME

#define ASSERT_WWISE_OK(x) (AKASSERT((x) == AK_Success))
#define IS_WWISE_OK(x)     ((x) == AK_Success)


namespace Audio
{
    ///////////////////////////////////////////////////////////////////////////////////////////////////
    // Wwise Xml Element Names
    namespace WwiseXmlTags
    {
        static constexpr const char* WwiseEventTag = "WwiseEvent";
        static constexpr const char* WwiseRtpcTag = "WwiseRtpc";
        static constexpr const char* WwiseSwitchTag = "WwiseSwitch";
        static constexpr const char* WwiseStateTag = "WwiseState";
        static constexpr const char* WwiseRtpcSwitchTag = "WwiseRtpc";
        static constexpr const char* WwiseFileTag = "WwiseFile";
        static constexpr const char* WwiseAuxBusTag = "WwiseAuxBus";
        static constexpr const char* WwiseValueTag = "WwiseValue";
        static constexpr const char* WwiseNameAttribute = "wwise_name";
        static constexpr const char* WwiseValueAttribute = "wwise_value";
        static constexpr const char* WwiseMutiplierAttribute = "atl_mult";
        static constexpr const char* WwiseShiftAttribute = "atl_shift";
        static constexpr const char* WwiseLocalizedAttribute = "wwise_localized";

        namespace Legacy
        {
            static constexpr const char* WwiseLocalizedAttribute = "wwise_localised";
        }

    } // namespace WwiseXmlTags

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    // Wwise-specific helper functions

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    inline AkVector AZVec3ToAkVector(const AZ::Vector3& vec3)
    {
        // swizzle Y <--> Z
        AkVector akVec;
        akVec.X = vec3.GetX();
        akVec.Y = vec3.GetZ();
        akVec.Z = vec3.GetY();
        return akVec;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    inline AkTransform AZVec3ToAkTransform(const AZ::Vector3& position)
    {
        AkTransform akTransform;
        akTransform.SetOrientation(0.0, 0.0, 1.0, 0.0, 1.0, 0.0);   // May add orientation support later.
        akTransform.SetPosition(AZVec3ToAkVector(position));
        return akTransform;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    inline void ATLTransformToAkTransform(const SATLWorldPosition& atlTransform, AkTransform& akTransform)
    {
        akTransform.Set(
            AZVec3ToAkVector(atlTransform.GetPositionVec()),
            AZVec3ToAkVector(atlTransform.GetForwardVec().GetNormalized()), // Wwise SDK requires that the Orientation vectors
            AZVec3ToAkVector(atlTransform.GetUpVec().GetNormalized())       // are normalized prior to sending to the apis.
        );
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    namespace Wwise
    {
        // See AkMemoryMgr.h
        inline static const char* MemoryManagerCategories[]
        {
            "Object", "Event", "Structure", "Media", "GameObject", "Processing", "ProcessingPlugin", "Streaming", "StreamingIO", "SpatialAudio",
            "SpatialAudioGeometry", "SpatialAudioPaths", "GameSim", "MonitorQueue", "Profiler", "FilePackage", "SoundEngine"
        };

        static_assert(AZ_ARRAY_SIZE(MemoryManagerCategories) == AkMemID_NUM,
            "Wwise memory categories have changed, the list of display names needs to be updated.");
    } // namespace Wwise

} // namespace Audio
