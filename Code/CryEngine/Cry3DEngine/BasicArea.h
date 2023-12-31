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

#ifndef CRYINCLUDE_CRY3DENGINE_BASICAREA_H
#define CRYINCLUDE_CRY3DENGINE_BASICAREA_H
#pragma once

#define COPY_MEMBER_SAVE(_dst, _src, _name) { (_dst)->_name = (_src)->_name; }
#define COPY_MEMBER_LOAD(_dst, _src, _name) { (_dst)->_name = (_src)->_name; }

enum EObjList
{
    DYNAMIC_OBJECTS = 0,
    STATIC_OBJECTS,
    PROC_OBJECTS,
    ENTITY_LISTS_NUM
};

struct SRNInfo
{
    SRNInfo()
    {
        memset(this, 0, sizeof(*this));
    }

    SRNInfo(IRenderNode* _pNode)
    {
        fMaxViewDist = _pNode->m_fWSMaxViewDist;
        AABB aabbBox = _pNode->GetBBox();
        objSphere.center = aabbBox.GetCenter();
        objSphere.radius = aabbBox.GetRadius();
        pNode = _pNode;
        nRType = _pNode->GetRenderNodeType();

        /*#ifdef _DEBUG
                erType = _pNode->GetRenderNodeType();
                cry_strcpy(szName, _pNode->GetName());
        #endif*/
    }

    bool operator == (const IRenderNode* _pNode) const { return (pNode == _pNode); }
    bool operator == (const SRNInfo& rOther) const { return (pNode == rOther.pNode); }

    float fMaxViewDist;
    Sphere objSphere;
    IRenderNode* pNode;
    EERType nRType;
    /*#ifdef _DEBUG
        EERType erType;
        char szName[32];
    #endif*/
};

struct SCasterInfo
{
    SCasterInfo()
    {
        memset(this, 0, sizeof(*this));
    }

    SCasterInfo(IRenderNode* _pNode, float fMaxDist)
    {
        fMaxCastingDist = fMaxDist;
        objBox = _pNode->GetBBox();
        objSphere.center = objBox.GetCenter();
        objSphere.radius = objBox.GetRadius();
        pNode = _pNode;
        nRType = _pNode->GetRenderNodeType();
        nRenderNodeFlags = _pNode->GetRndFlags();
        bCanExecuteAsRenderJob = _pNode->CanExecuteRenderAsJob();
    }

    SCasterInfo(IRenderNode* _pNode, float fMaxDist, EERType renderNodeType)
    {
        fMaxCastingDist = fMaxDist;
        _pNode->FillBBox(objBox);
        objSphere.center = objBox.GetCenter();
        objSphere.radius = objBox.GetRadius();
        pNode = _pNode;
        nRType = renderNodeType;
        nRenderNodeFlags = _pNode->GetRndFlags();
        bCanExecuteAsRenderJob = _pNode->CanExecuteRenderAsJob();
    }

    bool operator == (const IRenderNode* _pNode) const { return (pNode == _pNode); }
    bool operator == (const SCasterInfo& rOther) const { return (pNode == rOther.pNode); }

    void GetMemoryUsage([[maybe_unused]] ICrySizer* pSizer) const { /*nothing*/}

    float fMaxCastingDist;
    Sphere objSphere;
    AABB objBox;
    IRenderNode* pNode;
    uint32 nGSMFrameId;
    EERType nRType;
    bool bCanExecuteAsRenderJob;
    uint32 nRenderNodeFlags;
};

#define UPDATE_PTR_AND_SIZE(_pData, _nDataSize, _SIZE_PLUS) \
    {                                                       \
        _pData += (_SIZE_PLUS);                             \
        _nDataSize -= (_SIZE_PLUS);                         \
        assert(_nDataSize >= 0);                            \
    }                                                       \

enum EAreaType
{
    eAreaType_Undefined,
    eAreaType_OcNode,
    eAreaType_VisArea
};

struct CBasicArea
    : public Cry3DEngineBase
{
    CBasicArea()
    {
        m_boxArea.min = m_boxArea.max = Vec3(0, 0, 0);
        m_pObjectsTree = NULL;
    }

    ~CBasicArea();

    void CompileObjects(int nListId); // optimize objects lists for rendering

    class COctreeNode* m_pObjectsTree;

    AABB m_boxArea; // bbox containing everything in sector including child sectors
    AABB m_boxStatics; // bbox containing only objects in STATIC_OBJECTS list of this node and height-map
};

#endif // CRYINCLUDE_CRY3DENGINE_BASICAREA_H
