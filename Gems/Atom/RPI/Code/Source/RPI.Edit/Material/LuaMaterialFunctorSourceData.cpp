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

#include <Atom/RPI.Edit/Material/LuaMaterialFunctorSourceData.h>
#include <Atom/RPI.Reflect/Material/LuaMaterialFunctor.h>
#include <Atom/RPI.Reflect/Material/MaterialPropertiesLayout.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Script/ScriptAsset.h>
#include <Atom/RPI.Edit/Common/AssetUtils.h>

namespace AZ
{
    namespace RPI
    {
        void LuaMaterialFunctorSourceData::Reflect(AZ::ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext->Class<LuaMaterialFunctorSourceData>()
                    ->Version(3)
                    ->Field("file", &LuaMaterialFunctorSourceData::m_luaSourceFile)
                    ->Field("propertyNamePrefix", &LuaMaterialFunctorSourceData::m_propertyNamePrefix)
                    ->Field("srgNamePrefix", &LuaMaterialFunctorSourceData::m_srgNamePrefix)
                    ->Field("optionsNamePrefix", &LuaMaterialFunctorSourceData::m_optionsNamePrefix)
                    //[GFX TODO][ATOM-6011] Add support for inline script. Needs a custom "multiline string" json serializer.
                    //->Field("script", &LuaMaterialFunctorSourceData::m_luaScript)
                    ;
            }
        }

        AZStd::vector<LuaMaterialFunctorSourceData::AssetDependency> LuaMaterialFunctorSourceData::GetAssetDependencies() const
        {
            if (!m_luaSourceFile.empty())
            {
                AssetDependency dependency;
                dependency.m_jobKey = "Lua Compile";
                dependency.m_sourceFilePath = m_luaSourceFile;

                return AZStd::vector<AssetDependency>{dependency};
            }
            else
            {
                return {};
            }
        }

        Outcome<AZStd::vector<Name>, void> LuaMaterialFunctorSourceData::GetNameListFromLuaScript(AZ::ScriptContext& scriptContext, const char* luaFunctionName) const
        {
            AZStd::vector<Name> result;

            AZ::ScriptDataContext call;
            if (scriptContext.Call(luaFunctionName, call, false))
            {
                if (!call.CallExecute())
                {
                    AZ_Error("LuaMaterialFunctorSourceData", false, "Failed calling %s().", luaFunctionName);
                    return Failure();
                }

                if (1 != call.GetNumResults() || !call.IsTable(0))
                {
                    AZ_Error("LuaMaterialFunctorSourceData", false, "%s() must return a table.", luaFunctionName);
                    return Failure();
                }

                AZ::ScriptDataContext table;
                if (!call.InspectTable(0, table))
                {
                    AZ_Error("LuaMaterialFunctorSourceData", false, "Failed to inspect table returned by %s().", luaFunctionName);
                    return Failure();
                }

                const char* fieldName;
                int fieldIndex;
                int elementIndex;

                bool foundPropertyError = false;

                while (table.InspectNextElement(elementIndex, fieldName, fieldIndex))
                {
                    if (fieldIndex != -1)
                    {
                        if (!table.IsString(elementIndex))
                        {
                            AZ_Error("LuaMaterialFunctorSourceData", false, "%s() returned invalid table: element[%d] is not a string", luaFunctionName, fieldIndex);
                            foundPropertyError = true;
                            continue;
                        }

                        const char* materialPropertyName = nullptr;
                        if (!table.ReadValue(elementIndex, materialPropertyName))
                        {
                            AZ_Error("LuaMaterialFunctorSourceData", false, "%s() returned invalid table: element[%d] is invalid", luaFunctionName, fieldIndex);
                            foundPropertyError = true;
                            continue;
                        }

                        result.push_back(Name{materialPropertyName});
                    }
                }

                if (foundPropertyError)
                {
                    return Failure();
                }
            }

            return Success(result);
        }

        RPI::LuaMaterialFunctorSourceData::FunctorResult LuaMaterialFunctorSourceData::CreateFunctor(
                const AZStd::string& materialTypeSourceFilePath,
                const MaterialPropertiesLayout* propertiesLayout
            ) const
        {
            using namespace RPI;

            RPI::Ptr<LuaMaterialFunctor> functor = aznew LuaMaterialFunctor;

            functor->m_propertyNamePrefix = m_propertyNamePrefix;
            functor->m_srgNamePrefix = m_srgNamePrefix;
            functor->m_optionsNamePrefix = m_optionsNamePrefix;

            if (!m_luaScript.empty() && !m_luaSourceFile.empty())
            {
                AZ_Error("LuaMaterialFunctor", m_luaSourceFile.empty(), "Lua material functor has both a built-in script and an external script file.");
                return Failure();
            }
            else if (!m_luaScript.empty())
            {
                functor->m_scriptBuffer.assign(m_luaScript.begin(), m_luaScript.end());
            }
            else if (!m_luaSourceFile.empty())
            {
                auto loadOutcome = RPI::AssetUtils::LoadAsset<ScriptAsset>(materialTypeSourceFilePath, m_luaSourceFile);
                if (!loadOutcome)
                {
                    AZ_Error("LuaMaterialFunctorSourceData", false, "Could not load script file '%s'", m_luaSourceFile.c_str());
                    return Failure();
                }

                functor->m_scriptAsset = loadOutcome.GetValue();
            }
            else
            {
                AZ_Error("LuaMaterialFunctor", false, "Lua material functor has no script data.");
                return Failure();
            }

            AZ::ScriptContext scriptContext;

            auto scriptBuffer = functor->GetScriptBuffer();
            if (!scriptContext.Execute(scriptBuffer.data(), functor->GetScriptDescription(), scriptBuffer.size()))
            {
                AZ_Error("LuaMaterialFunctorSourceData", false, "Error initializing script '%s'.", functor->m_scriptAsset.ToString<AZStd::string>().c_str());
                return Failure();
            }

            // [GFX TODO][ATOM-6012]: Figure out how to make shader option dependencies and material property dependencies get automatically reported

            auto materialPropertyDependencies = GetNameListFromLuaScript(scriptContext, "GetMaterialPropertyDependencies");
            auto shaderOptionDependencies = GetNameListFromLuaScript(scriptContext, "GetShaderOptionDependencies");

            if (!materialPropertyDependencies.IsSuccess() || !shaderOptionDependencies.IsSuccess())
            {
                return Failure();
            }
            
            if (materialPropertyDependencies.GetValue().empty())
            {
                AZ_Error("LuaMaterialFunctorSourceData", false, "Material functor must use at least one material property.");
                return Failure();
            }

            m_shaderOptionDependencies = shaderOptionDependencies.GetValue();
            for (auto& shaderOption : m_shaderOptionDependencies)
            {
                shaderOption = Name{m_optionsNamePrefix + shaderOption.GetCStr()};
            }

            for (const Name& materialProperty : materialPropertyDependencies.GetValue())
            {
                MaterialPropertyIndex index = propertiesLayout->FindPropertyIndex(Name{m_propertyNamePrefix + materialProperty.GetCStr()});
                if (!index.IsValid())
                {
                    AZ_Error("LuaMaterialFunctorSourceData", false, "Property '%s' is not found in material type.", materialProperty.GetCStr());
                    return Failure();
                }

                AddMaterialPropertyDependency(functor, index);
            }

            return Success(RPI::Ptr<MaterialFunctor>(functor));
        }

        RPI::LuaMaterialFunctorSourceData::FunctorResult LuaMaterialFunctorSourceData::CreateFunctor(const RuntimeContext& context) const
        {
            return CreateFunctor(
                context.GetMaterialTypeSourceFilePath(),
                context.GetMaterialPropertiesLayout());
        }

        RPI::LuaMaterialFunctorSourceData::FunctorResult LuaMaterialFunctorSourceData::CreateFunctor(const EditorContext& context) const
        {
            return CreateFunctor(
                context.GetMaterialTypeSourceFilePath(),
                context.GetMaterialPropertiesLayout());
        }
    }
}