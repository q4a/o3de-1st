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

#include <AzCore/Component/Component.h>
#include <AzCore/Component/TransformBus.h>
#include <AtomLyIntegration/CommonFeatures/PostProcess/ShapeWeightModifier/ShapeWeightModifierComponentConfig.h>
#include <AtomLyIntegration/CommonFeatures/PostProcess/PostFxWeightRequestBus.h>

namespace AZ
{
    namespace Render
    {
        class ShapeWeightModifierComponentController final
            : public PostFxWeightRequestBus::Handler
        {
        public:
            friend class EditorShapeWeightModifierComponent;

            AZ_TYPE_INFO(AZ::Render::ShapeWeightModifierComponentController, "{5EF82EB8-8A7F-4B6C-BD40-8BABA1ABE0E5}");
            static void Reflect(AZ::ReflectContext* context);

            ShapeWeightModifierComponentController() = default;
            ShapeWeightModifierComponentController(const ShapeWeightModifierComponentConfig& config);

            static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services);
            static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services);
            static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& services);

            void Activate(EntityId entityId);
            void Deactivate();
            void SetConfiguration(const ShapeWeightModifierComponentConfig& config);
            const ShapeWeightModifierComponentConfig& GetConfiguration() const { return m_configuration; }

            //! PostFxWeightRequestBus::Handler Override
            virtual float GetWeightAtPosition(const AZ::Vector3& influencerPosition) const override;
        private:
            float GetRatio(float maxRange, float minRange, float distance) const;
            AZ_DISABLE_COPY(ShapeWeightModifierComponentController);

            ShapeWeightModifierComponentConfig m_configuration;
            EntityId m_entityId;
        };
    }
}
