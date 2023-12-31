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

namespace AZ
{
    template <typename TYPE>
    inline AZStd::size_t ThreadSafeDeque<TYPE>::Size() const
    {
        AZStd::scoped_lock<AZStd::recursive_mutex> lock(m_mutex);
        return m_deque.size();
    }

    template <typename TYPE>
    void ThreadSafeDeque<TYPE>::Clear()
    {
        AZStd::scoped_lock<AZStd::recursive_mutex> lock(m_mutex);
        m_deque.clear();
    }

    template <typename TYPE>
    template <typename TYPE_DEDUCED>
    inline void ThreadSafeDeque<TYPE>::PushFrontItem(TYPE_DEDUCED&& item)
    {
        AZStd::scoped_lock<AZStd::recursive_mutex> lock(m_mutex);
        m_deque.push_front(AZStd::forward<TYPE_DEDUCED>(item));
    }
    
    template <typename TYPE>
    template <typename TYPE_DEDUCED>
    inline void ThreadSafeDeque<TYPE>::PushBackItem(TYPE_DEDUCED&& item)
    {
        AZStd::scoped_lock<AZStd::recursive_mutex> lock(m_mutex);
        m_deque.push_back(AZStd::forward<TYPE_DEDUCED>(item));
    }

    template <typename TYPE>
    inline bool ThreadSafeDeque<TYPE>::PopFrontItem(TYPE& item)
    {
        AZStd::scoped_lock<AZStd::recursive_mutex> lock(m_mutex);
        if (m_deque.size() <= 0)
        {
            return false;
        }
        item = m_deque.front();
        m_deque.pop_front();
        return true;
    }

    template <typename TYPE>
    inline bool ThreadSafeDeque<TYPE>::PopBackItem(TYPE& item)
    {
        AZStd::scoped_lock<AZStd::recursive_mutex> lock(m_mutex);
        if (m_deque.size() <= 0)
        {
            return false;
        }
        item = m_deque.back();
        m_deque.pop_back();
        return true;
    }

    template <typename TYPE>
    inline void ThreadSafeDeque<TYPE>::Swap(DequeType& swapDeque)
    {
        AZStd::scoped_lock<AZStd::recursive_mutex> lock(m_mutex);
        swapDeque.swap(m_deque);
    }

    template <typename TYPE>
    inline void ThreadSafeDeque<TYPE>::Visit(const AZStd::function<void(TYPE&)>& visitor)
    {
        AZStd::scoped_lock<AZStd::recursive_mutex> lock(m_mutex);
        for (TYPE& element : m_deque)
        {
            visitor(element);
        }
    }

    template <typename TYPE>
    inline void ThreadSafeDeque<TYPE>::VisitDeque(const AZStd::function<void(DequeType&)>& visitor)
    {
        AZStd::scoped_lock<AZStd::recursive_mutex> lock(m_mutex);
        visitor(m_deque);
    }
}
