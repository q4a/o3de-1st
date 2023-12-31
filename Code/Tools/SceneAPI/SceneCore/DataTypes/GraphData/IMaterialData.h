#pragma once

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

#ifndef AZINCLUDE_TOOLS_SCENECORE_DATATYPES_IMATERIALDATA_H_
#define AZINCLUDE_TOOLS_SCENECORE_DATATYPES_IMATERIALDATA_H_

#include <AzCore/std/string/string.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/Math/Vector3.h>
#include <SceneAPI/SceneCore/DataTypes/IGraphObject.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace DataTypes
        {
            class IMaterialData
                : public IGraphObject
            {
            public:
                AZ_RTTI(IMaterialData, "{4C0E818F-CEE8-48A0-AC3D-AC926811BFE4}", IGraphObject);

                enum class TextureMapType
                {
                    Diffuse,
                    Specular,
                    Bump,
                    Normal
                };

                ~IMaterialData() override = default;

                void GetDebugOutput(AZ::SceneAPI::Utilities::DebugOutput& output) const override
                {
                    output.Write("MaterialName", GetMaterialName());
                    output.Write("UniqueId", GetUniqueId());
                    output.Write("IsNoDraw", IsNoDraw());
                    output.Write("DiffuseColor", GetDiffuseColor());
                    output.Write("SpecularColor", GetSpecularColor());
                    output.Write("EmissiveColor", GetEmissiveColor());
                    output.Write("Opacity", GetOpacity());
                    output.Write("Shininess", GetShininess());
                }

                virtual const AZStd::string& GetMaterialName() const = 0;
                virtual const AZStd::string& GetTexture(TextureMapType mapType) const = 0;
                virtual bool IsNoDraw() const = 0;

                virtual const AZ::Vector3& GetDiffuseColor() const = 0;
                virtual const AZ::Vector3& GetSpecularColor() const = 0;
                virtual const AZ::Vector3& GetEmissiveColor() const = 0;
                virtual float GetOpacity() const = 0;
                virtual float GetShininess() const = 0;
                                
                virtual uint64_t GetUniqueId() const = 0;
            };
        }  //namespace DataTypes
    }  //namespace SceneAPI
}  //namespace AZ

#endif // AZINCLUDE_TOOLS_SCENECORE_DATATYPES_IMATERIALDATA_H_
