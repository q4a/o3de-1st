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

#include "CrySystem_precompiled.h"

#include "ProjectDefines.h"
#if defined(MAP_LOADING_SLICING)

#include "SyncLock.h"

SSyncLock::SSyncLock(const char* name, int id, bool own)
{
    stack_string ss;
    ss.Format("%s_%d", name, id);

    Open(ss);
    if (own)
    {
        if (!IsValid())
        {
            Create(ss);
            number = id;
        }
        else
        {
            Close();
        }
    }
    else
    {
        number = id;
    }
}

SSyncLock::SSyncLock(const char* name, int minId, int maxId)
{
    ev = 0;
    stack_string ss;
    for (int i = minId; i < maxId; ++i)
    {
        ss.Format("%s_%d", name, i);
        if (Open(ss))
        {
            Close();
            continue;
        }
        if (Create(ss))
        {
            number = i;
        }
        break;
    }
}

SSyncLock::~SSyncLock()
{
    Close();
}

void SSyncLock::Own(const char* name)
{
    o_name.Format("%s_%d", name, number);
}

#if defined(LINUX) || defined(APPLE)

bool SSyncLock::Open(const char* name)
{
    ev = sem_open(name, 0);
    if (ev != SEM_FAILED)
    {
        CryLogAlways("Opened semaphore %p %s", ev, name);
    }
    return IsValid();
}

bool SSyncLock::Create(const char* name)
{
    ev = sem_open(name, O_CREAT | O_EXCL, 0777, 0);
    if (ev != SEM_FAILED)
    {
        CryLogAlways("Created semaphore %p %s", ev, name);
    }
    else
    {
        CryLogAlways("Failed to create semaphore %s %d", name, errno);
    }
    return IsValid();
}

void SSyncLock::Signal()
{
    if (ev)
    {
        sem_post(ev);
    }
}

bool SSyncLock::Wait(int ms)
{
    if (!ev)
    {
        return false;
    }

    timespec t = { 0 };
#if defined(LINUX)
    clock_gettime(CLOCK_REALTIME, &t);
#elif defined(APPLE)
    // On OSX/iOS there is no sem_timedwait()
    // We use repeated sem_trywait() instead
    if (sem_trywait(ev) == 0)
    {
        return true;
    }
#endif

    static const long NANOSECS_IN_MSEC = 1000000L;
    static const long NANOSECS_IN_SEC = 1000000000L;

    t.tv_sec += ms / 1000;
    t.tv_nsec += (ms % 1000) * NANOSECS_IN_MSEC;
    if (t.tv_nsec > NANOSECS_IN_SEC)
    {
        t.tv_nsec -= NANOSECS_IN_SEC;
        ++t.tv_sec;
    }
#if defined(LINUX)
    return sem_timedwait(ev, &t) == 0; //ETIMEDOUT for timeout
#elif defined (APPLE)
    // t = time left, interval = max time between tries, elapsed = actual time elapsed during a try
    const int num_ms_interval = 50; // poll time, in ms
    const timespec interval = { 0, NANOSECS_IN_MSEC * num_ms_interval };
    while (t.tv_sec >= 0 || t.tv_nsec > interval.tv_nsec)
    {
        timespec remaining;
        timespec elapsed = interval;
        if (nanosleep(&interval, &remaining) == -1)
        {
            elapsed.tv_nsec -= remaining.tv_nsec;
        }
        t.tv_nsec -= elapsed.tv_nsec;
        if (t.tv_nsec < 0L)
        {
            t.tv_nsec += NANOSECS_IN_SEC;
            t.tv_sec -= 1;
        }
        if (sem_trywait(ev) == 0)
        {
            return true;
        }
    }
    nanosleep(&t, NULL);
    return sem_trywait(ev) == 0;
#else
#error Not implemented
#endif
}

void SSyncLock::Close()
{
    if (ev)
    {
        sem_close(ev);
        ev = nullptr;
        if (!o_name.empty())
        {
            sem_unlink(o_name);
        }
    }
}

#else // defined(LINUX) || defined(APPLE)

bool SSyncLock::Open(const char* name)
{
    ev = OpenEvent(SYNCHRONIZE, FALSE, name);
    if (ev)
    {
        CryLogAlways("Opened event %p %s", ev, name);
    }
    return IsValid();
}

bool SSyncLock::Create(const char* name)
{
    ev = CreateEvent(NULL, FALSE, FALSE, name);
    if (ev)
    {
        CryLogAlways("Created event %p %s", ev, name);
    }
    else
    {
        CryLogAlways("Failed to create event %s", name);
    }
    return IsValid();
}

bool SSyncLock::Wait(int ms)
{
    //  CryLogAlways("Waiting %p", ev);
    DWORD res = WaitForSingleObject(ev, ms);
    if (res != WAIT_OBJECT_0)
    {
        CryLogAlways("WFS result %d", res);
    }
    return res == WAIT_OBJECT_0;
}

void SSyncLock::Signal()
{
    //CryLogAlways("Signaled %p", ev);
    if (!SetEvent(ev))
    {
        CryLogAlways("Error signalling!");
    }
}

void SSyncLock::Close()
{
    if (ev)
    {
        CryLogAlways("Closed event %p", ev);
        CloseHandle(ev);
        ev = 0;
    }
}

#endif // defined(LINUX) || defined(APPLE)

bool SSyncLock::IsValid() const
{
    return ev != 0;
}

#endif // defined(MAP_LOADING_SLICING)
