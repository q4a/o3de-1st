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

#include "Maestro_precompiled.h"
#include "CharacterTrackAnimator.h"
#include "AnimNode.h"
#include "CharacterTrack.h"

/*static*/ const float CCharacterTrackAnimator::s_minClipDuration = 0.01666666666f;         // 1/60th of a second, or one-frame for 60Hz rendering

//////////////////////////////////////////////////////////////////////////
CCharacterTrackAnimator::CCharacterTrackAnimator()
{
    m_forceAnimKeyChange = false;

    m_baseAnimState.m_layerPlaysAnimation[0] = m_baseAnimState.m_layerPlaysAnimation[1] = m_baseAnimState.m_layerPlaysAnimation[2] = false;

    ResetLastAnimKeys();

    m_baseAnimState.m_bTimeJumped[0] = m_baseAnimState.m_bTimeJumped[1] = m_baseAnimState.m_bTimeJumped[2] = false;
    m_baseAnimState.m_jumpTime[0] = m_baseAnimState.m_jumpTime[1] = m_baseAnimState.m_jumpTime[2] = 0.0f;
}

//////////////////////////////////////////////////////////////////////////
void CCharacterTrackAnimator::ResetLastAnimKeys()
{
    m_baseAnimState.m_lastAnimationKeys[0][0] = -1;
    m_baseAnimState.m_lastAnimationKeys[0][1] = -1;
    m_baseAnimState.m_lastAnimationKeys[1][0] = -1;
    m_baseAnimState.m_lastAnimationKeys[1][1] = -1;
    m_baseAnimState.m_lastAnimationKeys[2][0] = -1;
    m_baseAnimState.m_lastAnimationKeys[2][1] = -1;
}

//////////////////////////////////////////////////////////////////////////
void CCharacterTrackAnimator::OnReset(IAnimNode* animNode)
{
    ResetLastAnimKeys();

    ReleaseAllAnimations(animNode);

    m_baseAnimState.m_layerPlaysAnimation[0] = m_baseAnimState.m_layerPlaysAnimation[1] = m_baseAnimState.m_layerPlaysAnimation[2] = false;
}

//////////////////////////////////////////////////////////////////////////
f32 CCharacterTrackAnimator::ComputeAnimKeyNormalizedTime(const ICharacterKey& key, float ectime) const
{
    float endTime = key.GetValidEndTime();
    const float clipDuration = clamp_tpl(endTime - key.m_startTime, s_minClipDuration, key.m_duration);
    float t;
    f32   retNormalizedTime;

    if (clipDuration > s_minClipDuration)
    {
        t = (ectime - key.time) * key.m_speed;

        if (key.m_bLoop && t > clipDuration)
        {
            // compute t for repeating clip
            t = fmod(t, clipDuration);
        }

        t += key.m_startTime;
        t = clamp_tpl(t, key.m_startTime, endTime);
    }
    else
    {
        // clip has perceptably no length - set time to beginning or end frame, whichever comes first in time
        t = (key.m_startTime < endTime) ? key.m_startTime : endTime;
    }

    retNormalizedTime = clamp_tpl(t / key.m_duration, .0f, 1.0f);
    return retNormalizedTime;
}

//////////////////////////////////////////////////////////////////////////
ILINE bool CCharacterTrackAnimator::IsAnimationPlaying(const SAnimState& animState) const
{
    return animState.m_layerPlaysAnimation[0] || animState.m_layerPlaysAnimation[1] || animState.m_layerPlaysAnimation[2];
}

//////////////////////////////////////////////////////////////////////////
void CCharacterTrackAnimator::ReleaseAllAnimations([[maybe_unused]] IAnimNode* animNode)
{
}

//////////////////////////////////////////////////////////////////////////
void CCharacterTrackAnimator::AnimateTrack([[maybe_unused]] class CCharacterTrack* pTrack, [[maybe_unused]] SAnimContext& ec, [[maybe_unused]] int layer, [[maybe_unused]] int trackIndex)
{
}

void CCharacterTrackAnimator::ForceAnimKeyChange()
{
    m_forceAnimKeyChange = true;
}
