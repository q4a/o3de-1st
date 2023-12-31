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

#include <CoreLights/PolygonLightDelegate.h>
#include <Atom/RPI.Public/Scene.h>
#include <AtomLyIntegration/CommonFeatures/CoreLights/AreaLightComponentConfig.h>

namespace AZ
{
    namespace Render
    {
        PolygonLightDelegate::PolygonLightDelegate(LmbrCentral::PolygonPrismShapeComponentRequests* shapeBus, EntityId entityId, bool isVisible)
            : LightDelegateBase<PolygonLightFeatureProcessorInterface>(entityId, isVisible)
            , m_shapeBus(shapeBus)
        {
            InitBase(entityId);
            shapeBus->SetHeight(0.0f);
        }

        void PolygonLightDelegate::SetLightEmitsBothDirections(bool lightEmitsBothDirections)
        {
            if (GetLightHandle().IsValid())
            {
                GetFeatureProcessor()->SetLightEmitsBothDirections(GetLightHandle(), lightEmitsBothDirections);
            }
        }

        float PolygonLightDelegate::CalculateAttenuationRadius(float lightThreshold) const
        {
            // Calculate the radius at which the irradiance will be equal to cutoffIntensity.
            float intensity = GetPhotometricValue().GetCombinedIntensity(PhotometricUnit::Lumen);
            return sqrt(intensity / lightThreshold);
        }

        void PolygonLightDelegate::HandleShapeChanged()
        {
            if (GetLightHandle().IsValid())
            {
                GetFeatureProcessor()->SetPosition(GetLightHandle(), GetTransform().GetTranslation());

                AZStd::vector<Vector2> vertices = m_shapeBus->GetPolygonPrism()->m_vertexContainer.GetVertices();

                Transform transform = GetTransform();
                transform.SetScale(Vector3(transform.GetScale().GetMaxElement())); // Poly Prism only supports uniform scale, so use max element.

                AZStd::vector<Vector3> transformedVertices;
                transformedVertices.reserve(vertices.size());
                for (Vector2 vertex : vertices)
                {
                    transformedVertices.push_back(transform.TransformPoint(Vector3(vertex.GetX(), vertex.GetY(), 0.0f)));
                }
                GetFeatureProcessor()->SetPolygonPoints(GetLightHandle(), transformedVertices.data(), transformedVertices.size(), GetTransform().GetBasisZ());
            }
        }

        float PolygonLightDelegate::GetSurfaceArea() const
        {
            // Surprisingly simple to calculate area of arbitrary polygon assuming no self-intersection. See https://en.wikipedia.org/wiki/Shoelace_formula
            float twiceArea = 0.0f;
            AZStd::vector<Vector2> vertices = m_shapeBus->GetPolygonPrism()->m_vertexContainer.GetVertices();
            for (size_t i = 0; i < vertices.size(); ++i)
            {
                size_t j = (i + 1) % vertices.size();
                twiceArea += vertices.at(i).GetX() * vertices.at(j).GetY();
                twiceArea -= vertices.at(i).GetY() * vertices.at(j).GetX();
            }
            float scale = GetTransform().GetScale().GetMaxElement();
            return GetAbs(twiceArea * 0.5f * scale * scale);
        }

        void PolygonLightDelegate::DrawDebugDisplay(const Transform& transform, const Color& color, AzFramework::DebugDisplayRequests& debugDisplay, bool isSelected) const
        {
            if (isSelected)
            {
                debugDisplay.SetColor(color);

                // Draw a Polygon for the attenuation radius
                debugDisplay.DrawWireSphere(transform.GetTranslation(), CalculateAttenuationRadius(AreaLightComponentConfig::CutoffIntensity));
                debugDisplay.DrawArrow(transform.GetTranslation(), transform.GetTranslation() + transform.GetBasisZ());
            }
        }

    } // namespace Render
} // namespace AZ
