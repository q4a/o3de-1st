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

#include "./UseTextureFunctor.h"
#include <Atom/RPI.Edit/Material/MaterialFunctorSourceData.h>

namespace AZ
{
    namespace Render
    {
        class UseTextureFunctor;

        //! Builds a UseTextureFunctor.
        //! Materials can use this functor to control whether a specific texture property will be sampled. 
        //! Sampling will be disabled if no texture is bound or if the useTexture flag is disabled.
        class UseTextureFunctorSourceData final
            : public RPI::MaterialFunctorSourceData
        {
        public:
            AZ_RTTI(UseTextureFunctorSourceData, "{2CBB80CF-5EEB-4C0F-B628-1FE0729E2D18}", RPI::MaterialFunctorSourceData);

            static void Reflect(ReflectContext* context);

            FunctorResult CreateFunctor(const RuntimeContext& context) const override;
            FunctorResult CreateFunctor(const EditorContext& context) const override;
            AZStd::vector<AZ::Name> GetShaderOptionDependencies() const override;

        private:
            // Material property inputs...
            Name m_texturePropertyName;     //!< Name of a material property for a texture
            Name m_useTexturePropertyName;  //!< Name of a material property for a bool that indicates whether to use the texture

            //! material properties that relate to the texture, which will be enabled only when the texture map is enabled.
            AZStd::vector<Name> m_dependentProperties;

            // Shader option output...
            AZStd::vector<AZ::Name> m_shaderTags; //!< Identifier tags associated to shaders that a material uses
            Name m_useTextureOptionName;        //!< Name of the shader option that controls whether the texture should be sampled
        };

    } // namespace Render
} // namespace AZ
