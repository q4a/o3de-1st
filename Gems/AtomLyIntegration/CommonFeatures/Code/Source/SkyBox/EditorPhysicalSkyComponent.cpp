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

#include <SkyBox/EditorPhysicalSkyComponent.h>
#include <AzCore/RTTI/BehaviorContext.h>

namespace AZ
{
    namespace Render
    {
        void EditorPhysicalSkyComponent::Reflect(AZ::ReflectContext* context)
        {
            BaseClass::Reflect(context);

            if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<EditorPhysicalSkyComponent, BaseClass>()
                    ->Version(1, ConvertToEditorRenderComponentAdapter<1>);

                if (AZ::EditContext* editContext = serializeContext->GetEditContext())
                {
                    editContext->Class<EditorPhysicalSkyComponent>(
                        "Physical Sky", "Physical Sky render the background of your scene with physical simulation")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                            ->Attribute(AZ::Edit::Attributes::Category, "Atom")
                            ->Attribute(AZ::Edit::Attributes::Icon, "Editor/Icons/Components/Component_Placeholder.svg")
                            ->Attribute(AZ::Edit::Attributes::ViewportIcon, "editor/icons/components/viewport/component_placeholder.png")
                            ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Game", 0x232b318c))
                            ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                            ->Attribute(AZ::Edit::Attributes::HelpPageURL, "")
                        ;

                    editContext->Class<PhysicalSkyComponentController>(
                        "PhysicalSkyComponentController", "")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                            ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->DataElement(AZ::Edit::UIHandlers::Default, &PhysicalSkyComponentController::m_configuration, "Configuration", "")
                            ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                        ;

                    editContext->Class<PhysicalSkyComponentConfig>(
                        "PhysicalSkyComponentConfig", "")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                            ->DataElement(Edit::UIHandlers::ComboBox, &PhysicalSkyComponentConfig::m_intensityMode, "Intensity Mode", "Specifying the light unit")
                                ->EnumAttribute(PhotometricUnit::Ev100Luminance, "Ev100")
                                ->EnumAttribute(PhotometricUnit::Nit, "Nit")
                            ->DataElement(AZ::Edit::UIHandlers::Slider, &PhysicalSkyComponentConfig::m_skyIntensity, "Sky Intensity", "Brightness of the sky")
                                ->Attribute(AZ::Edit::Attributes::Min, &PhysicalSkyComponentConfig::GetSkyIntensityMin)
                                ->Attribute(AZ::Edit::Attributes::Max, &PhysicalSkyComponentConfig::GetSkyIntensityMax)
                                ->Attribute(Edit::Attributes::Suffix, &PhysicalSkyComponentConfig::GetIntensitySuffix)
                            ->DataElement(AZ::Edit::UIHandlers::Slider, &PhysicalSkyComponentConfig::m_sunIntensity, "Sun Intensity", "Brightness of the sun")
                                ->Attribute(AZ::Edit::Attributes::Min, &PhysicalSkyComponentConfig::GetSunIntensityMin)
                                ->Attribute(AZ::Edit::Attributes::Max, &PhysicalSkyComponentConfig::GetSunIntensityMax)
                                ->Attribute(Edit::Attributes::Suffix, &PhysicalSkyComponentConfig::GetIntensitySuffix)
                            ->DataElement(AZ::Edit::UIHandlers::Slider, &PhysicalSkyComponentConfig::m_sunRadiusFactor, "Sun Radius Factor", "A factor for Physical sun radius in millions of km. 1 unit is 695,508 km")
                                ->Attribute(AZ::Edit::Attributes::Min, 0.1f)
                                ->Attribute(AZ::Edit::Attributes::Max, 2.f)
                            ->DataElement(AZ::Edit::UIHandlers::Slider, &PhysicalSkyComponentConfig::m_turbidity, "Turbidity", "A measure of the aerosol content in the air. Default is 1.")
                                ->Attribute(AZ::Edit::Attributes::Min, 1)
                                ->Attribute(AZ::Edit::Attributes::Max, 10)
                                ->Attribute(AZ::Edit::Attributes::Step, 1)
                        ;
                }
            }

            if (auto behaviorContext = azrtti_cast<BehaviorContext*>(context))
            {
                behaviorContext->Class<EditorPhysicalSkyComponent>()->RequestBus("PhysicalSkyRequestBus");

                behaviorContext->ConstantProperty("EditorPhysicalSkyComponentTypeId", BehaviorConstant(Uuid(EditorPhysicalSkyComponentTypeId)))
                    ->Attribute(AZ::Script::Attributes::Module, "render")
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation);
            }
        }

        EditorPhysicalSkyComponent::EditorPhysicalSkyComponent(const PhysicalSkyComponentConfig& config)
            : BaseClass(config)
        {
        }

        u32 EditorPhysicalSkyComponent::OnConfigurationChanged()
        {
            // If the intensity mode changes in the editor, convert the photometric value and update the intensity
            if (m_controller.m_configuration.m_intensityMode != m_controller.m_skyPhotometricValue.GetType())
            {
                m_controller.m_skyPhotometricValue.ConvertToPhotometricUnit(m_controller.m_configuration.m_intensityMode);
                m_controller.m_configuration.m_skyIntensity = m_controller.m_skyPhotometricValue.GetIntensity();


                m_controller.m_sunPhotometricValue.ConvertToPhotometricUnit(m_controller.m_configuration.m_intensityMode);
                m_controller.m_configuration.m_sunIntensity = m_controller.m_sunPhotometricValue.GetIntensity();
            }

            BaseClass::OnConfigurationChanged();
            return Edit::PropertyRefreshLevels::AttributesAndValues;
        }
    }
}
