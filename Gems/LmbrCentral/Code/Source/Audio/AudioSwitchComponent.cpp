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
#include "AudioSwitchComponent.h"

#include <ISystem.h>

#include <LmbrCentral/Audio/AudioProxyComponentBus.h>

#include <AzCore/Component/Entity.h>
#include <AzCore/Debug/Trace.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/SerializeContext.h>


namespace LmbrCentral
{
    void AudioSwitchComponent::Reflect(AZ::ReflectContext* context)
    {
        auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<AudioSwitchComponent, AZ::Component>()
                ->Version(1)
                ->Field("Switch name", &AudioSwitchComponent::m_defaultSwitchName)
                ->Field("State name", &AudioSwitchComponent::m_defaultStateName)
                ;
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->EBus<AudioSwitchComponentRequestBus>("AudioSwitchComponentRequestBus")
                ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::Preview)
                ->Event("SetState", &AudioSwitchComponentRequestBus::Events::SetState)
                ->Event("SetSwitchState", &AudioSwitchComponentRequestBus::Events::SetSwitchState)
                ;
        }
    }

    //=========================================================================
    void AudioSwitchComponent::Activate()
    {
        OnDefaultSwitchChanged();
        OnDefaultStateChanged();

        // set the default switch state, if valid IDs were found.
        if (m_defaultSwitchID != INVALID_AUDIO_CONTROL_ID && m_defaultStateID != INVALID_AUDIO_SWITCH_STATE_ID)
        {
            AudioProxyComponentRequestBus::Event(GetEntityId(), &AudioProxyComponentRequestBus::Events::SetSwitchState, m_defaultSwitchID, m_defaultStateID);
        }

        AudioSwitchComponentRequestBus::Handler::BusConnect(GetEntityId());
    }

    //=========================================================================
    void AudioSwitchComponent::Deactivate()
    {
        AudioSwitchComponentRequestBus::Handler::BusDisconnect(GetEntityId());
    }

    //=========================================================================
    AudioSwitchComponent::AudioSwitchComponent(const AZStd::string& switchName, const AZStd::string& stateName)
        : m_defaultSwitchName(switchName)
        , m_defaultStateName(stateName)
    {
    }

    //=========================================================================
    void AudioSwitchComponent::SetState(const char* stateName)
    {
        if (m_defaultSwitchID != INVALID_AUDIO_CONTROL_ID)
        {
            // only allowed if there's a default switch that is known.
            if (stateName && stateName[0] != '\0')
            {
                Audio::TAudioSwitchStateID stateID = INVALID_AUDIO_SWITCH_STATE_ID;
                Audio::AudioSystemRequestBus::BroadcastResult(stateID, &Audio::AudioSystemRequestBus::Events::GetAudioSwitchStateID, m_defaultSwitchID, stateName);
                if (stateID != INVALID_AUDIO_SWITCH_STATE_ID)
                {
                    AudioProxyComponentRequestBus::Event(GetEntityId(), &AudioProxyComponentRequestBus::Events::SetSwitchState, m_defaultSwitchID, stateID);
                }
            }
        }
    }

    //=========================================================================
    void AudioSwitchComponent::SetSwitchState(const char* switchName, const char* stateName)
    {
        Audio::TAudioControlID switchID = INVALID_AUDIO_CONTROL_ID;
        Audio::TAudioSwitchStateID stateID = INVALID_AUDIO_SWITCH_STATE_ID;

        // lookup switch...
        if (switchName && switchName[0] != '\0')
        {
            Audio::AudioSystemRequestBus::BroadcastResult(switchID, &Audio::AudioSystemRequestBus::Events::GetAudioSwitchID, switchName);
        }

        // using the switchID (if found), lookup the state...
        if (switchID != INVALID_AUDIO_CONTROL_ID && stateName && stateName[0] != '\0')
        {
            Audio::AudioSystemRequestBus::BroadcastResult(stateID, &Audio::AudioSystemRequestBus::Events::GetAudioSwitchStateID, switchID, stateName);
        }

        // if both IDs found, make the call...
        if (switchID != INVALID_AUDIO_CONTROL_ID && stateID != INVALID_AUDIO_SWITCH_STATE_ID)
        {
            AudioProxyComponentRequestBus::Event(GetEntityId(), &AudioProxyComponentRequestBus::Events::SetSwitchState, switchID, stateID);
        }
    }

    //=========================================================================
    void AudioSwitchComponent::OnDefaultSwitchChanged()
    {
        m_defaultSwitchID = INVALID_AUDIO_CONTROL_ID;
        if (!m_defaultSwitchName.empty())
        {
            Audio::AudioSystemRequestBus::BroadcastResult(m_defaultSwitchID, &Audio::AudioSystemRequestBus::Events::GetAudioSwitchID, m_defaultSwitchName.c_str());
        }
    }

    //=========================================================================
    void AudioSwitchComponent::OnDefaultStateChanged()
    {
        m_defaultStateID = INVALID_AUDIO_SWITCH_STATE_ID;
        if (!m_defaultStateName.empty() && m_defaultSwitchID != INVALID_AUDIO_CONTROL_ID)
        {
            Audio::AudioSystemRequestBus::BroadcastResult(m_defaultStateID, &Audio::AudioSystemRequestBus::Events::GetAudioSwitchStateID, m_defaultSwitchID, m_defaultStateName.c_str());
        }
    }

} // namespace LmbrCentral
