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
//! All possible property types.
#ifndef CRYINCLUDE_EDITOR_UTIL_PROPERTYDEFINITION_H
#define CRYINCLUDE_EDITOR_UTIL_PROPERTYDEFINITION_H
#pragma once

#include "Include/EditorCoreAPI.h"
#include "Variable.h"

enum PropertyType
{
    ePropertyInvalid = 0,
    ePropertyTable = 1,
    ePropertyBool = 2,
    ePropertyInt,
    ePropertyFloat,
    ePropertyVector2,
    ePropertyVector,
    ePropertyVector4,
    ePropertyString,
    ePropertyColor,
    ePropertyAngle,
    ePropertyFloatCurve,
    ePropertyColorCurve,
    ePropertyFile,
    ePropertyTexture,
    ePropertyAnimation,
    ePropertyModel,
    ePropertySelection,
    ePropertyList,
    ePropertyShader,
    ePropertyMaterial,
    ePropertyEquip,
    ePropertyReverbPreset,
    ePropertyLocalString,
    ePropertyDeprecated0, // formerly ePropertyCustomAction
    ePropertyGameToken,
    ePropertySequence,
    ePropertyMissionObj,
    ePropertyUser,
    ePropertySequenceId,
    ePropertyLightAnimation,
    ePropertyFlare,
    ePropertyParticleName,
    ePropertyGeomCache,
    ePropertyAudioTrigger,
    ePropertyAudioSwitch,
    ePropertyAudioSwitchState,
    ePropertyAudioRTPC,
    ePropertyAudioEnvironment,
    ePropertyAudioPreloadRequest,
    ePropertyFlowCustomData,
    ePropertyUiElement,
    ePropertyMotion
};

namespace Prop
{
    struct EDITOR_CORE_API Description
    {
        Description();
        Description(IVariable* pVar);

        PropertyType m_type;
        int m_numImages;
        AZ_PUSH_DISABLE_DLL_EXPORT_MEMBER_WARNING
        IVarEnumListPtr m_enumList;
        AZ_POP_DISABLE_DLL_EXPORT_MEMBER_WARNING
        float m_rangeMin;
        float m_rangeMax;
        float m_step;
        bool m_bHardMin;
        bool m_bHardMax;
        QString m_name;
        float m_valueMultiplier;
        CUIEnumsDatabase_SEnum* m_pEnumDBItem;
    };

    EDITOR_CORE_API const char* GetName(int dataType);

    EDITOR_CORE_API PropertyType GetType(int dataType);
    EDITOR_CORE_API PropertyType GetType(const IVariable* var);
    EDITOR_CORE_API PropertyType GetType(const char* type);

    EDITOR_CORE_API int GetNumImages(int dataType);
    EDITOR_CORE_API int GetNumImages(const IVariable* var);
    EDITOR_CORE_API int GetNumImages(const char* type);

    EDITOR_CORE_API const char* GetPropertyTypeToResourceType(PropertyType type);
}

#endif // CRYINCLUDE_EDITOR_UTIL_PROPERTYDEFINITION_H
