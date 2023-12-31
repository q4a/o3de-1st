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

#include <Source/NetworkTime/INetworkTime.h>
#include <AzNetworking/Serialization/ISerializer.h>
#include <AzNetworking/ConnectionLayer/IConnection.h>
#include <AzNetworking/Utilities/NetworkCommon.h>
#include <AzCore/std/containers/array.h>
#include <AzCore/std/string/string.h>
#include <AzCore/Console/ILogger.h>

namespace Multiplayer
{
    //! @class RewindableObject
    //! @brief A simple serializable data container that keeps a history of previous values, and can fetch those old values on request.
    template <typename BASE_TYPE, AZStd::size_t REWIND_SIZE>
    class RewindableObject
    {
    public:

        RewindableObject() = default;

        //! Constructor.
        //! @param connectionId the connectionId of the connection that owns the object.
        RewindableObject(AzNetworking::ConnectionId owningConnectionId);

        //! Copy construct from underlying base type.
        //! @param value base type value to construct from
        //! @param owningConnectionId the entity id of the owning object
        explicit RewindableObject(const BASE_TYPE& value, AzNetworking::ConnectionId owningConnectionId);

        //! Copy construct from another rewindable history buffer.
        //! @param rhs rewindable history buffer to construct from
        RewindableObject(const RewindableObject& rhs);

        //! Assignment from underlying base type.
        //! @param rhs base type value to assign from
        RewindableObject& operator = (const BASE_TYPE& rhs);

        //! Assignment from rewindable history buffer.
        //! @param rhs rewindable history buffer to assign from
        RewindableObject& operator = (const RewindableObject& rhs);

        //! Const base type operator.
        //! @return value in const base type form
        operator const BASE_TYPE&() const;

        //! Const base type retriever.
        //! @return value in const base type form
        const BASE_TYPE& Get() const;

        //! Base type retriever.
        //! @return value in base type form
        BASE_TYPE& Modify();

        //! Equality operator.
        //! @param rhs base type value to compare against
        //! @return boolean true if this == rhs
        bool operator == (const BASE_TYPE& rhs) const;

        //! Inequality operator.
        //! @param rhs base type value to compare against
        //! @return boolean true if this != rhs
        bool operator != (const BASE_TYPE& rhs) const;

        //! Base serialize method for all serializable structures or classes to implement
        //! @param serializer ISerializer instance to use for serialization
        //! @return boolean true for success, false for serialization failure
        bool Serialize(AzNetworking::ISerializer& serializer);

    private:

        //! Returns what the appropriate current time is for this rewindable property.
        //! @return the appropriate current time is for this rewindable property
        ApplicationFrameId GetCurrentTimeForProperty() const;

        //! Updates the latest value for this object instance, if frameTime represents a current or future time.
        //! Any attempts to set old values on the object will fail
        //! @param value     the new value to set in the object history
        //! @param frameTime the time to set the value for
        void SetValueForTime(const BASE_TYPE& value, ApplicationFrameId frameTime);

        //! Const value accessor, returns the correct value for the provided input time.
        //! @param frameTime the frame time to return the associated value for
        //! @return value given the current input time
        const BASE_TYPE& GetValueForTime(ApplicationFrameId frameTime) const;

        //! Helper method to compute clamped array index values accounting for the offset head index.
        AZStd::size_t GetOffsetIndex(AZStd::size_t absoluteIndex) const;

        mutable AzNetworking::ConnectionId m_owningConnectionId = AzNetworking::InvalidConnectionId;
        ApplicationFrameId m_headTime = ApplicationFrameId{0};
        uint32_t m_headIndex = 0;
        AZStd::array<BASE_TYPE, REWIND_SIZE> m_history;
    };
}

#include <Source/NetworkTime/RewindableObject.inl>
