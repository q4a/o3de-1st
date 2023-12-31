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

#pragma once

namespace Multiplayer
{
    template <typename BASE_TYPE, AZStd::size_t REWIND_SIZE>
    inline RewindableObject<BASE_TYPE, REWIND_SIZE>::RewindableObject(AzNetworking::ConnectionId owningConnectionId)
        : m_owningConnectionId(owningConnectionId)
    {
        m_history.fill(BASE_TYPE());
    }

    template <typename BASE_TYPE, AZStd::size_t REWIND_SIZE>
    inline RewindableObject<BASE_TYPE, REWIND_SIZE>::RewindableObject(const BASE_TYPE& value, AzNetworking::ConnectionId owningConnectionId)
        : m_owningConnectionId(owningConnectionId)
        , m_history(value)
    {
        m_history.fill(value);
    }

    template <typename BASE_TYPE, AZStd::size_t REWIND_SIZE>
    inline RewindableObject<BASE_TYPE, REWIND_SIZE>::RewindableObject(const RewindableObject<BASE_TYPE, REWIND_SIZE>& rhs)
        : m_owningConnectionId(rhs.m_owningConnectionId)
        , m_headTime(GetCurrentTimeForProperty())
        , m_headIndex(0)
    {
        m_history.fill(static_cast<BASE_TYPE>(rhs));
    }

    template <typename BASE_TYPE, AZStd::size_t REWIND_SIZE>
    inline RewindableObject<BASE_TYPE, REWIND_SIZE> &RewindableObject<BASE_TYPE, REWIND_SIZE>::operator =(const BASE_TYPE& rhs)
    {
        SetValueForTime(rhs, GetCurrentTimeForProperty());
        return *this;
    }

    template <typename BASE_TYPE, AZStd::size_t REWIND_SIZE>
    inline RewindableObject<BASE_TYPE, REWIND_SIZE> &RewindableObject<BASE_TYPE, REWIND_SIZE>::operator =(const RewindableObject<BASE_TYPE, REWIND_SIZE>& rhs)
    {
        INetworkTime* networkTime = AZ::Interface<INetworkTime>::Get();
        SetValueForTime(rhs.GetValueForTime(networkTime->GetApplicationFrameId()), GetCurrentTimeForProperty());
        return *this;
    }

    template <typename BASE_TYPE, AZStd::size_t REWIND_SIZE>
    inline RewindableObject<BASE_TYPE, REWIND_SIZE>::operator const BASE_TYPE& () const
    {
        return Get();
    }

    template <typename BASE_TYPE, AZStd::size_t REWIND_SIZE>
    inline const BASE_TYPE& RewindableObject<BASE_TYPE, REWIND_SIZE>::Get() const
    {
        return GetValueForTime(GetCurrentTimeForProperty());
    }

    template <typename BASE_TYPE, AZStd::size_t REWIND_SIZE>
    inline BASE_TYPE& RewindableObject<BASE_TYPE, REWIND_SIZE>::Modify()
    {
        ApplicationFrameId frameTime = GetCurrentTimeForProperty();
        if (frameTime < m_headTime)
        {
            AZ_Assert(false, "Trying to mutate a rewindable in the past");
        }
        else if (m_headTime < frameTime)
        {
            SetValueForTime(GetValueForTime(frameTime), frameTime);
        }
        const BASE_TYPE& returnValue = GetValueForTime(GetCurrentTimeForProperty());
        return const_cast<BASE_TYPE&>(returnValue);
    }

    template <typename BASE_TYPE, AZStd::size_t REWIND_SIZE>
    inline bool RewindableObject<BASE_TYPE, REWIND_SIZE>::operator == (const BASE_TYPE& rhs) const
    {
        const BASE_TYPE lhs = GetValueForTime(GetCurrentTimeForProperty());
        return (lhs == rhs);
    }

    template <typename BASE_TYPE, AZStd::size_t REWIND_SIZE>
    inline bool RewindableObject<BASE_TYPE, REWIND_SIZE>::operator != (const BASE_TYPE& rhs) const
    {
        const BASE_TYPE lhs = GetValueForTime(GetCurrentTimeForProperty());
        return (lhs != rhs);
    }

    template <typename BASE_TYPE, AZStd::size_t REWIND_SIZE>
    inline bool RewindableObject<BASE_TYPE, REWIND_SIZE>::Serialize(AzNetworking::ISerializer& serializer)
    {
        BASE_TYPE current = GetValueForTime(GetCurrentTimeForProperty());
        if (serializer.Serialize(current, "Element") && (serializer.GetSerializerMode() == AzNetworking::SerializerMode::WriteToObject))
        {
            SetValueForTime(current, GetCurrentTimeForProperty());
        }
        return serializer.IsValid();
    }

    template <typename BASE_TYPE, AZStd::size_t REWIND_SIZE>
    inline ApplicationFrameId RewindableObject<BASE_TYPE, REWIND_SIZE>::GetCurrentTimeForProperty() const
    {
        INetworkTime* networkTime = AZ::Interface<INetworkTime>::Get();
        return networkTime->GetApplicationFrameIdForRewindingConnection(m_owningConnectionId);
    }

    template <typename BASE_TYPE, AZStd::size_t REWIND_SIZE>
    inline void RewindableObject<BASE_TYPE, REWIND_SIZE>::SetValueForTime(const BASE_TYPE& value, ApplicationFrameId frameTime)
    {
        if (frameTime < m_headTime)
        {
            // Don't try and set values older than our current head value
            return;
        }

        // Keeping a reference to copy to head so that delta bitset differences are only applied from the prev version
        const BASE_TYPE& prevHead = m_history[m_headIndex];

        if (static_cast<size_t>(frameTime - m_headTime) >= m_history.size())
        {
            // This update represents a large enough time delta that we'll just flush the whole buffer with the new value
            m_headTime = frameTime;
            m_headIndex = 0;
            for (uint32_t i = 0; i < m_history.size(); ++i)
            {
                m_history[i] = value;
            }
            return;
        }

        while (m_headTime < frameTime)
        {
            m_history[m_headIndex] = prevHead;
            m_headIndex = (m_headIndex + 1) % m_history.size();
            m_headTime++;
        }

        m_history[m_headIndex] = value;
        AZ_Assert(m_headTime == frameTime, "Invalid head value");
    }

    template <typename BASE_TYPE, AZStd::size_t REWIND_SIZE>
    inline const BASE_TYPE &RewindableObject<BASE_TYPE, REWIND_SIZE>::GetValueForTime(ApplicationFrameId frameTime) const
    {
        if (frameTime > m_headTime)
        {
            return m_history[m_headIndex];
        }
        const AZStd::size_t frameDelta = static_cast<AZStd::size_t>(m_headTime) - static_cast<AZStd::size_t>(frameTime);
        return m_history[GetOffsetIndex(frameDelta)];
    }

    template <typename BASE_TYPE, AZStd::size_t REWIND_SIZE>
    inline AZStd::size_t RewindableObject<BASE_TYPE, REWIND_SIZE>::GetOffsetIndex(AZStd::size_t absoluteIndex) const
    {
        if (absoluteIndex >= m_history.size())
        {
            AZLOG(NET_Rewind, "Request for value which is too old");
            absoluteIndex = m_history.size() - 1;
        }
        return ((m_headIndex + m_history.size()) - absoluteIndex) % m_history.size();
    }
}
