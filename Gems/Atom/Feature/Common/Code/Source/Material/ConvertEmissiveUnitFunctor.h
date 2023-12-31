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

#include <Atom/RPI.Reflect/Material/MaterialFunctor.h>
#include <Atom/RPI.Reflect/Material/MaterialPropertyDescriptor.h>
#include <Atom/Feature/CoreLights/PhotometricValue.h>

namespace AZ
{
    namespace Render
    {
        //! The functor can be used to convert between different emissive light unit
        //! Only support Ev100 and Lux
        class ConvertEmissiveUnitFunctor final
            : public AZ::RPI::MaterialFunctor
        {
            friend class ConvertEmissiveUnitFunctorSourceData;
        public:
            AZ_RTTI(ConvertEmissiveUnitFunctor, "{F272CFAB-FD71-4E78-AA47-D0D2E88CE30C}", AZ::RPI::MaterialFunctor);

            static void Reflect(AZ::ReflectContext* context);

            void Process(RuntimeContext& context) override;
            void Process(EditorContext& context) override;
        private:

            AZ::RPI::MaterialPropertyIndex m_intensityPropertyIndex;
            AZ::RPI::MaterialPropertyIndex m_lightUnitPropertyIndex;
            AZ::RHI::ShaderInputConstantIndex m_shaderInputIndex;

            uint32_t m_ev100Index;
            uint32_t m_nitIndex;

            float m_ev100Min;
            float m_ev100Max;
            float m_nitMin;
            float m_nitMax;
        };
    }
}
