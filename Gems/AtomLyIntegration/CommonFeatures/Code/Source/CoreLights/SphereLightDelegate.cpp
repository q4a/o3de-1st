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

#include <CoreLights/SphereLightDelegate.h>
#include <Atom/RPI.Public/Scene.h>
#include <Atom/Feature/CoreLights/PointLightFeatureProcessorInterface.h>
#include <AtomLyIntegration/CommonFeatures/CoreLights/AreaLightComponentConfig.h>

namespace AZ
{
    namespace Render
    {
        SphereLightDelegate::SphereLightDelegate(LmbrCentral::SphereShapeComponentRequests* shapeBus, EntityId entityId, bool isVisible)
            : LightDelegateBase<PointLightFeatureProcessorInterface>(entityId, isVisible)
            , m_shapeBus(shapeBus)
        {
            InitBase(entityId);
        }

        float SphereLightDelegate::CalculateAttenuationRadius(float lightThreshold) const
        {
            // Calculate the radius at which the irradiance will be equal to cutoffIntensity.
            float intensity = GetPhotometricValue().GetCombinedIntensity(PhotometricUnit::Lumen);
            return sqrt(intensity / lightThreshold);
        }
        
        void SphereLightDelegate::HandleShapeChanged()
        {
            if (GetLightHandle().IsValid())
            {
                GetFeatureProcessor()->SetPosition(GetLightHandle(), GetTransform().GetTranslation());
                GetFeatureProcessor()->SetBulbRadius(GetLightHandle(), GetRadius());
            }
        }

        float SphereLightDelegate::GetSurfaceArea() const
        {
            float radius = GetRadius();
            return 4.0f * Constants::Pi * radius * radius;
        }

        float SphereLightDelegate::GetRadius() const
        {
            return m_shapeBus->GetRadius() * GetTransform().GetScale().GetMaxElement();
        }

        void SphereLightDelegate::DrawDebugDisplay(const Transform& transform, const Color& color, AzFramework::DebugDisplayRequests& debugDisplay, bool isSelected) const
        {
            if (isSelected)
            {
                debugDisplay.SetColor(color);

                // Draw a sphere for the attenuation radius
                debugDisplay.DrawWireSphere(transform.GetTranslation(), CalculateAttenuationRadius(AreaLightComponentConfig::CutoffIntensity));
            }
        }
    } // namespace Render
} // namespace AZ
