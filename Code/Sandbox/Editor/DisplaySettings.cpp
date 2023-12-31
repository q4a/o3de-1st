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

// Description : Display Settings implementation.

#include "EditorDefs.h"

#include "DisplaySettings.h"

// Qt
#include <QSettings>

// Editor
#include "Settings.h"
#include "Material/MaterialManager.h"



//////////////////////////////////////////////////////////////////////////
CDisplaySettings::CDisplaySettings()
{
    m_flags = SETTINGS_NOLABELS | SETTINGS_NOCOLLISION;
    m_objectHideMask = 0;

    // All flags besides GI enabled by default
    m_renderFlags = RENDER_FLAG_LAST_ONE - 1;
    m_renderFlags &= ~(RENDER_FLAG_BBOX);
    m_renderFlags &= ~(RENDER_FLAG_GI);
    m_debugFlags = 0;
    m_labelsDistance = 100;
}

//////////////////////////////////////////////////////////////////////////
CDisplaySettings::~CDisplaySettings()
{
}

void CDisplaySettings::SaveRegistry()
{
    SaveValue("Settings", "ObjectHideMask", m_objectHideMask);
    SaveValue("Settings", "RenderFlags", m_renderFlags);
    SaveValue("Settings", "DisplayFlags", m_flags & SETTINGS_SERIALIZABLE_FLAGS_MASK);
    SaveValue("Settings", "DebugFlags", m_debugFlags);
    SaveValue("Settings", "LabelsDistance", m_labelsDistance);
}

void CDisplaySettings::LoadRegistry()
{
    LoadValue("Settings", "ObjectHideMask", m_objectHideMask);
    LoadValue("Settings", "RenderFlags", m_renderFlags);
    LoadValue("Settings", "DisplayFlags", m_flags);
    m_flags &= SETTINGS_SERIALIZABLE_FLAGS_MASK;
    LoadValue("Settings", "DebugFlags", m_debugFlags);
    int temp = m_labelsDistance;
    LoadValue("Settings", "LabelsDistance", temp);
    m_labelsDistance = temp;

    gSettings.objectHideMask = m_objectHideMask;
}

void CDisplaySettings::SetObjectHideMask(int hideMask)
{
    int prevMask = m_objectHideMask;
    m_objectHideMask = hideMask;

    gSettings.objectHideMask = m_objectHideMask;

    GetIEditor()->Notify(eNotify_OnDisplayRenderUpdate);
};

//////////////////////////////////////////////////////////////////////////
void CDisplaySettings::PostInitApply()
{
    SetRenderFlags(m_renderFlags);
    SetDebugFlags(m_debugFlags);
}
//////////////////////////////////////////////////////////////////////////
void CDisplaySettings::SetRenderFlags(int flags)
{
    int prev = m_renderFlags;
    m_renderFlags = flags;

    if (!GetIEditor()->Get3DEngine())
    {
        return;
    }
}

//////////////////////////////////////////////////////////////////////////
void CDisplaySettings::SetDebugFlags(int flags)
{
    m_debugFlags = flags;

    //
    //SetCVar( "ai_DebugDraw",m_debugFlags&DBG_AI_DEBUGDRAW );
    //SetCVarInt( "r_TexLog",(m_debugFlags&DBG_TEXTURE_MEMINFO) ? 2:0 );
    //SetCVarInt( "MemStats",(m_debugFlags&DBG_MEMSTATS) ? 1000:0 );
    //SetCVarInt( "p_draw_helpers",(m_debugFlags&DBG_PHYSICS_DEBUGDRAW) ? 5634:0 );

    //SetCVar( "r_ProfileShaders",(m_debugFlags&DBG_RENDERER_PROFILESHADERS) );

    SetCVarInt("r_MeasureOverdraw", (m_debugFlags & DBG_RENDERER_OVERDRAW) ? 1 : 0);   //Eric@conffx display Overdraw in Particle editor preview window
    //SetCVarInt( "r_Stats",(m_debugFlags&DBG_RENDERER_RESOURCES) ? 4:0 );
    //SetCVarInt( "sys_enable_budgetmonitoring",(m_debugFlags&DBG_BUDGET_MONITORING) ? 4:0 );

    //SetCVarInt( "Profile",(m_debugFlags&DBG_FRAMEPROFILE) ? 1:0 );

    if (CMaterialManager* pMaterialManager = GetIEditor()->GetMaterialManager())
    {
        int mask = pMaterialManager->GetHighlightMask();
        if (m_debugFlags & DBG_HIGHLIGHT_BREAKABLE)
        {
            mask |= eHighlight_Breakable;
        }
        else
        {
            mask &= ~eHighlight_Breakable;
        }

        if (m_debugFlags & DBG_HIGHLIGHT_MISSING_SURFACE_TYPE)
        {
            mask |= eHighlight_NoSurfaceType;
        }
        else
        {
            mask &= ~eHighlight_NoSurfaceType;
        }

        pMaterialManager->SetHighlightMask(mask);
    }
}

//////////////////////////////////////////////////////////////////////////
void CDisplaySettings::SetCVar(const char* cvar, bool value)
{
    ICVar* var = GetIEditor()->GetSystem()->GetIConsole()->GetCVar(cvar);
    if (var)
    {
        var->Set((value) ? 1 : 0);
    }
    else
    {
        CLogFile::FormatLine("Console Variable %s not declared", cvar);
    }
}

//////////////////////////////////////////////////////////////////////////
void CDisplaySettings::SetCVarInt(const char* cvar, int value)
{
    ICVar* var = GetIEditor()->GetSystem()->GetIConsole()->GetCVar(cvar);
    if (var)
    {
        var->Set(value);
    }
    else
    {
        CLogFile::FormatLine("Console Variable %s not declared", cvar);
    }
}

//////////////////////////////////////////////////////////////////////////
void CDisplaySettings::DisplayLabels(bool bEnable)
{
    if (bEnable)
    {
        m_flags &= ~SETTINGS_NOLABELS;
    }
    else
    {
        m_flags |= SETTINGS_NOLABELS;
    }
}

//////////////////////////////////////////////////////////////////////////
void CDisplaySettings::DisplayTracks(bool bEnable)
{
    if (bEnable)
    {
        m_flags &= ~SETTINGS_HIDE_TRACKS;
    }
    else
    {
        m_flags |= SETTINGS_HIDE_TRACKS;
    }
}

//////////////////////////////////////////////////////////////////////////
void CDisplaySettings::DisplayLinks(bool bEnable)
{
    if (bEnable)
    {
        m_flags &= ~SETTINGS_HIDE_LINKS;
    }
    else
    {
        m_flags |= SETTINGS_HIDE_LINKS;
    }
}

//////////////////////////////////////////////////////////////////////////
void CDisplaySettings::DisplayHelpers(bool bEnable)
{
    if (bEnable)
    {
        m_flags &= ~SETTINGS_HIDE_HELPERS;
    }
    else
    {
        m_flags |= SETTINGS_HIDE_HELPERS;
    }
}

//////////////////////////////////////////////////////////////////////////
void CDisplaySettings::DisplayDimensionFigures(bool bEnable)
{
    if (bEnable)
    {
        m_flags |= SETTINGS_SHOW_DIMENSIONFIGURES;
    }
    else
    {
        m_flags &= ~SETTINGS_SHOW_DIMENSIONFIGURES;
    }
}

//////////////////////////////////////////////////////////////////////////
void CDisplaySettings::SetHighlightBreakable(bool bHighlight)
{
    if (bHighlight)
    {
        m_debugFlags |= DBG_HIGHLIGHT_BREAKABLE;
    }
    else
    {
        m_debugFlags &= ~DBG_HIGHLIGHT_BREAKABLE;
    }
}

//////////////////////////////////////////////////////////////////////////
void CDisplaySettings::SetHighlightMissingSurfaceType(bool bHighlight)
{
    if (bHighlight)
    {
        m_debugFlags |= DBG_HIGHLIGHT_MISSING_SURFACE_TYPE;
    }
    else
    {
        m_debugFlags &= ~DBG_HIGHLIGHT_MISSING_SURFACE_TYPE;
    }
}

//////////////////////////////////////////////////////////////////////////
void CDisplaySettings::SaveValue(const char* sSection, const char* sKey, int value)
{
    QSettings settings;
    settings.setValue(QStringLiteral("%1/%2").arg(sSection, sKey), value);
}

//////////////////////////////////////////////////////////////////////////
void CDisplaySettings::LoadValue(const char* sSection, const char* sKey, int& value)
{
    QSettings settings;
    value = settings.value(QStringLiteral("%1/%2").arg(sSection, sKey), value).toInt();
}

//////////////////////////////////////////////////////////////////////////
void CDisplaySettings::LoadValue(const char* sSection, const char* sKey, bool& value)
{
    QSettings settings;
    value = settings.value(QStringLiteral("%1/%2").arg(sSection, sKey), value).toInt();
}
