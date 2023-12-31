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

#include "Cry3DEngine_precompiled.h"

#include "ClipVolume.h"

CClipVolume::CClipVolume()
    : m_nStencilRef(0)
    , m_WorldTM(IDENTITY)
    , m_InverseWorldTM(IDENTITY)
    , m_BBoxWS(AABB::RESET)
    , m_BBoxLS(AABB::RESET)
    , m_pBspTree(NULL)
{
    memset(m_sName, 0x0, sizeof(m_sName));
}

CClipVolume::~CClipVolume()
{
    m_pRenderMesh = NULL;

    for (size_t i = 0; i < m_lstRenderNodes.size(); ++i)
    {
        if (m_lstRenderNodes[i]->m_pRNTmpData)
        {
            m_lstRenderNodes[i]->m_pRNTmpData->userData.m_pClipVolume = NULL;
        }
    }
}

void CClipVolume::SetName(const char* szName)
{
    cry_strcpy(m_sName, szName);
}

void CClipVolume::GetClipVolumeMesh(_smart_ptr<IRenderMesh>& renderMesh, Matrix34& worldTM) const
{
    renderMesh = m_pRenderMesh;
    worldTM = m_WorldTM;
}

AABB CClipVolume::GetClipVolumeBBox() const
{
    return m_BBoxWS;
}

void CClipVolume::Update(_smart_ptr<IRenderMesh> pRenderMesh, IBSPTree3D* pBspTree, const Matrix34& worldTM, uint32 flags)
{
    const bool bMeshUpdated = m_pRenderMesh != pRenderMesh;

    m_pRenderMesh = pRenderMesh;
    m_pBspTree = pBspTree;
    m_WorldTM = worldTM;
    m_InverseWorldTM = worldTM.GetInverted();
    m_BBoxWS.Reset();
    m_BBoxLS.Reset();
    m_nFlags = flags;

    if (m_pRenderMesh)
    {
        pRenderMesh->GetBBox(m_BBoxLS.min, m_BBoxLS.max);
        m_BBoxWS.SetTransformedAABB(worldTM, m_BBoxLS);
    }
}

bool CClipVolume::IsPointInsideClipVolume(const Vec3& point) const
{
    FUNCTION_PROFILER_3DENGINE;

    if (!m_pRenderMesh || !m_pBspTree || !m_BBoxWS.IsContainPoint(point))
    {
        return false;
    }

    Vec3 pt = m_InverseWorldTM.TransformPoint(point);
    return m_BBoxLS.IsContainPoint(pt) && m_pBspTree->IsInside(pt);
}

void CClipVolume::RegisterRenderNode(IRenderNode* pRenderNode)
{
    if (m_lstRenderNodes.Find(pRenderNode) < 0)
    {
        m_lstRenderNodes.Add(pRenderNode);

        if (pRenderNode->m_pRNTmpData)
        {
            pRenderNode->m_pRNTmpData->userData.m_pClipVolume = this;
        }
    }
}
void CClipVolume::UnregisterRenderNode(IRenderNode* pRenderNode)
{
    if (m_lstRenderNodes.Delete(pRenderNode) && pRenderNode->m_pRNTmpData)
    {
        pRenderNode->m_pRNTmpData->userData.m_pClipVolume = NULL;
    }
}

void CClipVolume::GetMemoryUsage(class ICrySizer* pSizer) const
{
    pSizer->AddObject(this, sizeof(*this));
}
