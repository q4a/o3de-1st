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

// Description : Tagged files database for 'SmartFileOpen' dialog
//
// Notice      : Refer  SmartFileOpenDialog h


#ifndef CRYINCLUDE_EDITOR_UTIL_INDEXEDFILES_H
#define CRYINCLUDE_EDITOR_UTIL_INDEXEDFILES_H
#pragma once


#include "FileUtil.h"
#include "STLPoolAllocator.h"
#include <functional>

class CIndexedFiles
{
    friend class CFileIndexingThread;
public:
    static CIndexedFiles& GetDB()
    {
        if (!s_pIndexedFiles)
        {
            assert(!"CIndexedFiles not created! Make sure you use CIndexedFiles::GetDB() after CIndexedFiles::StartFileIndexing() is called.");
        }
        assert(s_pIndexedFiles);
        return *s_pIndexedFiles;
    }

    static bool HasFileIndexingDone()
    { return s_bIndexingDone > 0; }

    static void Create()
    {
        assert(!s_pIndexedFiles);
        s_pIndexedFiles = new CIndexedFiles;
    }

    static void Destroy()
    {
        SAFE_DELETE(s_pIndexedFiles);
    }

    static void StartFileIndexing()
    {
        assert(s_bIndexingDone == 0);
        assert(s_pIndexedFiles);

        if (!s_pIndexedFiles)
        {
            return;
        }

        GetFileIndexingThread().Start(-1, "FileIndexing");
        m_startedFileIndexing = true;
    }

    static void AbortFileIndexing()
    {
        if (!m_startedFileIndexing)
        {
            return;
        }

        if (HasFileIndexingDone() == false)
        {
            GetFileIndexingThread().Abort();
        }
        m_startedFileIndexing = false;
    }

    static void RegisterCallback(std::function<void()> callback)
    {
        assert(s_pIndexedFiles);
        if (!s_pIndexedFiles)
        {
            return;
        }

        s_pIndexedFiles->AddUpdateCallback(callback);
    }

public:
    void Initialize(const QString& path, IFileUtil::ScanDirectoryUpdateCallBack updateCB = NULL);

    // Adds a new file to the database.
    void AddFile(const IFileUtil::FileDesc& path);
    // Removes a no-longer-existing file from the database.
    void RemoveFile(const QString& path);
    // Refreshes this database for the subdirectory.
    void Refresh(const QString& path, bool recursive = true);

    void GetFilesWithTags(IFileUtil::FileArray& files, const QStringList& tags) const;

    //! This method returns all the tags which start with a given prefix.
    //! It is useful for the tag auto-completion.
    void GetTagsOfPrefix(QStringList& tags, const QString& prefix) const;

    uint32 GetTotalCount() const
    { return (uint32)m_files.size(); }

private:
    static bool m_startedFileIndexing;

    std::vector <std::function<void()> > m_updateCallbacks;
    IFileUtil::FileArray m_files;
    std::map<QString, int> m_pathToIndex;
#if defined(_DEBUG) || defined(AZ_COMPILER_CLANG)
    // In debug, the validation phase of the pool allocator when destructed takes so much time,
    // and using the STLPoolAllocator causes a strange issue when compiling with clang
    typedef std::set<int, std::less<int> > int_set;
    typedef std::map<QString, int_set, std::less<QString> > TagTable;
#else
    typedef std::set<int, std::less<int>, stl::STLPoolAllocator<int> > int_set;
    typedef std::map<QString, int_set, std::less<QString>, stl::STLPoolAllocator<std::pair<const QString, int_set> > > TagTable;
#endif
    TagTable m_tags;
    QString m_rootPath;

    void GetTags(QStringList& tags, const QString& path) const;
    void PrepareTagTable();

    CryMutex m_updateCallbackMutex;

    void AddUpdateCallback(std::function<void()> updateCallback);
    void InvokeUpdateCallbacks();

    // A done flag for the background file indexing
    static volatile TIntAtomic s_bIndexingDone;
    // A thread for the background file indexing
    class CFileIndexingThread
        : public CryThread<CFileIndexingThread>
    {
    public:
        virtual void Run()
        {
            CIndexedFiles::GetDB().Initialize("@assets@", CallBack);
            CryInterlockedAdd(CIndexedFiles::s_bIndexingDone.Addr(), 1);
        }

        CFileIndexingThread()
            : m_abort(false) {}

        void Abort()
        {
            m_abort = true;
            WaitForThread();
        }

        virtual ~CFileIndexingThread()
        {
            Abort();
        }
    private:
        bool m_abort;
        static bool CallBack([[maybe_unused]] const QString& msg)
        {
            if (CIndexedFiles::GetFileIndexingThread().m_abort)
            {
                return false;
            }
            return true;
        }
    };

    static CFileIndexingThread& GetFileIndexingThread()
    {
        static CFileIndexingThread s_fileIndexingThread;

        return s_fileIndexingThread;
    }

    // A global database for tagged files
    static CIndexedFiles* s_pIndexedFiles;
};
#endif // CRYINCLUDE_EDITOR_UTIL_INDEXEDFILES_H
