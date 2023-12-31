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

#include <AzCore/EBus/ScheduledEventHandle.h>
#include <AzCore/EBus/ScheduledEvent.h>
#include <AzCore/Console/ILogger.h>

namespace AZ
{
    ScheduledEventHandle::ScheduledEventHandle(TimeMs executeTimeMs, TimeMs durationTimeMs, ScheduledEvent* scheduledEvent)
        : m_executeTimeMs(executeTimeMs)
        , m_durationMs(durationTimeMs)
        , m_event(scheduledEvent)
    {
        ;
    }

    bool ScheduledEventHandle::operator >(const ScheduledEventHandle& rhs) const
    {
        return m_executeTimeMs > rhs.m_executeTimeMs;
    }

    bool ScheduledEventHandle::Notify()
    {
        if (m_event)
        {
            if (m_event->m_handle == this)
            {
                m_event->Notify();

                // Check whether or not the event was deleted during the callback
                if (m_event != nullptr)
                {
                    if (m_event->m_autoRequeue)
                    {
                        m_event->Requeue();
                        return true;
                    }
                    else // Not configured to auto-requeue, so remove the handle
                    {
                        m_event->ClearHandle();
                        m_event = nullptr;
                    }
                }
            }
            else
            {
                AZLOG_WARN("ScheduledEventHandle event pointer doesn't match to the pointer of handle to the event.");
            }
        }
        return false; // Event has been deleted, so the handle class must be deleted after this function.
    }

    void ScheduledEventHandle::Clear()
    {
        m_event = nullptr;
    }

    TimeMs ScheduledEventHandle::GetExecuteTimeMs() const
    {
        return m_executeTimeMs;
    }

    TimeMs ScheduledEventHandle::GetDurationTimeMs() const
    {
        return m_durationMs;
    }
}
