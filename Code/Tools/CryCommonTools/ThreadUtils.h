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

#ifndef CRYINCLUDE_CRYCOMMONTOOLS_THREADUTILS_H
#define CRYINCLUDE_CRYCOMMONTOOLS_THREADUTILS_H
#pragma once

#include <AzCore/std/parallel/mutex.h>

#if defined(AZ_PLATFORM_WINDOWS)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>   // CRITICAL_SECTION
#endif

#include <deque>
#include <vector>

namespace ThreadUtils
{
#if defined(AZ_PLATFORM_WINDOWS)
    class CriticalSection
    {
        friend class ConditionVariable;

    public:
        CriticalSection()
        {
            memset(&m_cs, 0, sizeof(m_cs));
            InitializeCriticalSection(&m_cs);
        }

        ~CriticalSection()
        {
            DeleteCriticalSection(&m_cs);
        }

        void Lock()
        {
            EnterCriticalSection(&m_cs);
        }
        void Unlock()
        {
            LeaveCriticalSection(&m_cs);
        }
        bool TryLock()
        {
            return TryEnterCriticalSection(&m_cs) != FALSE;
        }

#if defined(AZ_DEBUG_BUILD)
        bool IsLocked()
        {
            return m_cs.RecursionCount > 0 && m_cs.OwningThread == GetCurrentThread();
        }
#endif

    private:
        // You are not allowed to copy or move a CRITICAL_SECTION
        // handle, so make this class non-copyable
        CriticalSection(const CriticalSection& cs);
        CriticalSection& operator=(const CriticalSection& cs);

        CRITICAL_SECTION m_cs;
    };

    class ConditionVariable
    {
    public:
        ConditionVariable()
        {
            InitializeConditionVariable(&m_cv);
        }

        void Wake()
        {
            WakeConditionVariable(&m_cv);
        }

        void WakeAll()
        {
            WakeAllConditionVariable(&m_cv);
        }

        void Sleep(CriticalSection& cs, DWORD milliseconds = INFINITE)
        {
            SleepConditionVariableCS(&m_cv, &cs.m_cs, milliseconds);
        }

    private:
        // You are not allowed to copy or move a CONDITION_VARIABLE
        // handle, so make this class non-copyable
        ConditionVariable(const ConditionVariable& cs);
        ConditionVariable& operator=(const ConditionVariable& cs);

        CONDITION_VARIABLE m_cv;
    };
#elif AZ_TRAIT_OS_PLATFORM_APPLE || defined(AZ_PLATFORM_LINUX)
    class CriticalSection
    {
    public:
        CriticalSection()
            : m_locked(false)
        {
        }

        ~CriticalSection()
        {
        }

        void Lock()
        {
            m_cs.lock();
            m_locked = true;
        }
        void Unlock()
        {
            m_cs.unlock();
            m_locked = false;
        }
        bool TryLock()
        {
            m_locked = m_cs.try_lock();
            return m_locked;
        }

#if defined (AZ_DEBUG_BUILD)
        bool IsLocked()
        {
            return m_locked;
        }
#endif

    private:
        // You are not allowed to copy or move a CRITICAL_SECTION
        // handle, so make this class non-copyable
        CriticalSection(const CriticalSection& cs);
        CriticalSection& operator=(const CriticalSection& cs);

        bool m_locked;
        AZStd::recursive_mutex m_cs;
    };
#endif

    class AutoLock
    {
    private:
        CriticalSection& m_lock;

        AutoLock();
        AutoLock(const AutoLock&);
        AutoLock& operator = (const AutoLock&);

    public:
        AutoLock(CriticalSection& lock)
            : m_lock(lock)
        {
            m_lock.Lock();
        }
        ~AutoLock()
        {
            m_lock.Unlock();
        }
    };

    typedef void(* JobFunc)(void*);

    struct Job
    {
        JobFunc m_func;
        void* m_data;
        int m_debugInitialThread;

        Job()
            : m_func(0)
            , m_data(0)
            , m_debugInitialThread(0)
        {
        }

        Job(JobFunc func, void* data)
            : m_func(func)
            , m_data(data)
            , m_debugInitialThread(0)
        {
        }

        void Run()
        {
            m_func(m_data);
        }
    };
    typedef std::deque<Job> Jobs;

    struct JobTrace
    {
        Job m_job;
        bool m_stolen;
        int m_duration;

        JobTrace()
            : m_duration(0)
            , m_stolen(false)
        {
        }
    };
    typedef std::vector<JobTrace> JobTraces;

    class SimpleWorker;

    class SimpleThreadPool
    {
    public:
        SimpleThreadPool(bool trace);
        ~SimpleThreadPool();

        bool GetJob(Job& job, int threadIndex);

        // Submits single independent job
        template<class T>
        void Submit(void(* jobFunc)(T*), T* data)
        {
            Submit(Job((JobFunc)jobFunc, data));
        }

        void Start(int numThreads);
        void WaitAllJobs();

    private:
        void Submit(const Job& job);

        bool m_started;
        bool m_trace;

        std::vector<SimpleWorker*> m_workers;

        std::vector<JobTraces> m_threadTraces;

        int m_numProcessedJobs;
        AZStd::mutex m_lockJobs;
        std::vector<Job> m_jobs;
    };

#if defined(AZ_PLATFORM_WINDOWS)
#pragma pack(push, 8)
    struct ThreadNameInfo
    {
        DWORD dwType; // Must be 0x1000.
        LPCSTR szName; // Pointer to name (in user addr space).
        DWORD dwThreadID; // Thread ID (-1=caller thread).
        DWORD dwFlags; // Reserved for future use, must be zero.
    };
#pragma pack(pop)

    // Usage: SetThreadName (-1, "MainThread");
    // From http://msdn.microsoft.com/en-us/library/xcb2z8hs.aspx
    inline void SetThreadName([[maybe_unused]] DWORD dwThreadID, [[maybe_unused]] const char* threadName)
    {
#ifdef _DEBUG
        ThreadNameInfo info;
        info.dwType = 0x1000;
        info.szName = threadName;
        info.dwThreadID = dwThreadID;
        info.dwFlags = 0;

        __try
        {
            const DWORD exceptionCode = 0x406D1388;
            RaiseException(exceptionCode, 0, sizeof(info) / sizeof(ULONG_PTR), (ULONG_PTR*)&info);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
        }
#endif
    }
#endif
}

#endif // CRYINCLUDE_CRYCOMMONTOOLS_THREADUTILS_H
