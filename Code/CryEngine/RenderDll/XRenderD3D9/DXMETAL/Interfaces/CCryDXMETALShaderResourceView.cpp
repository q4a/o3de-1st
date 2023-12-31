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

// Description : Definition of the DXGL wrapper for ID3D11ShaderResourceView

#include "RenderDll_precompiled.h"
#include "CCryDXMETALShaderResourceView.hpp"
#include "CCryDXMETALDevice.hpp"
#include "CCryDXMETALResource.hpp"
#include "../Implementation/GLResource.hpp"
#include "../Implementation/MetalDevice.hpp"


CCryDXGLShaderResourceView::CCryDXGLShaderResourceView(CCryDXGLResource* pResource, const D3D11_SHADER_RESOURCE_VIEW_DESC& kDesc, CCryDXGLDevice* pDevice)
    : CCryDXGLView(pResource, pDevice)
    , m_kDesc(kDesc)
{
    DXGL_INITIALIZE_INTERFACE(D3D11ShaderResourceView)
}

CCryDXGLShaderResourceView::~CCryDXGLShaderResourceView()
{
}

bool CCryDXGLShaderResourceView::Initialize(NCryMetal::CDevice* pDevice)
{
    D3D11_RESOURCE_DIMENSION eDimension;
    m_spResource->GetType(&eDimension);
    m_spGLView = NCryMetal::CreateShaderResourceView(m_spResource->GetGLResource(), eDimension, m_kDesc, pDevice);
    return m_spGLView != NULL;
}

NCryMetal::SShaderResourceView* CCryDXGLShaderResourceView::GetGLView()
{
    return m_spGLView;
}


////////////////////////////////////////////////////////////////////////////////
// Implementation of ID3D11ShaderResourceView
////////////////////////////////////////////////////////////////////////////////

void CCryDXGLShaderResourceView::GetDesc(D3D11_SHADER_RESOURCE_VIEW_DESC* pDesc)
{
    *pDesc = m_kDesc;
}