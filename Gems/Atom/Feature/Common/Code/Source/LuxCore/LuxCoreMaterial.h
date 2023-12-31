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


#include <Atom_Feature_Traits_Platform.h>
#if AZ_TRAIT_LUXCORE_SUPPORTED

#include <Atom/RPI.Public/Material/Material.h>
#include "LuxCoreTexture.h"

#include <luxcore/luxcore.h>

namespace AZ
{
    namespace Render
    {
        // Manage mapping between Atom PBR material and LuxCore Disney material
        class LuxCoreMaterial final
        {
        public:
            LuxCoreMaterial() = default;
            LuxCoreMaterial(const AZ::Data::Instance<AZ::RPI::Material>& material);
            LuxCoreMaterial(const LuxCoreMaterial &material);
            ~LuxCoreMaterial();

            luxrays::Properties GetLuxCoreMaterialProperties();
            AZ::Data::InstanceId GetMaterialId();
        private:

            static constexpr const char* s_pbrColorGroup = "baseColor";
            static constexpr const char* s_pbrMetallicGroup = "metallic";
            static constexpr const char* s_pbrRoughnessGroup = "roughness";
            static constexpr const char* s_pbrSpecularGroup = "specularF0";
            static constexpr const char* s_pbrNormalGroup = "normal";
            static constexpr const char* s_pbrOpacityGroup = "opacity";

            static constexpr const char* s_pbrColorProperty = "color";
            static constexpr const char* s_pbrFactorProperty = "factor";
            static constexpr const char* s_pbrUseTextureProperty = "useTexture";
            static constexpr const char* s_pbrTextureProperty = "textureMap";

            void ParseProperty(const char* group, AZStd::string propertyName);
            bool ParseTexture(const char* group, AZStd::string propertyName);
            AZ::Name MakePbrPropertyName(const char* groupName, const char* propertyName) const;

            void Init(const AZ::Data::Instance<AZ::RPI::Material>& material);

            AZStd::string m_luxCoreMaterialName;
            luxrays::Properties m_luxCoreMaterial;

            AZ::Data::Instance<AZ::RPI::Material> m_material = nullptr;
        };
    }
}
#endif
