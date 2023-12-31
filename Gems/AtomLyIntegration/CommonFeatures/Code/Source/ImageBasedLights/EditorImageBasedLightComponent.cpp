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

#include <ImageBasedLights/EditorImageBasedLightComponent.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzCore/Asset/AssetManager.h>
#include <AzCore/RTTI/BehaviorContext.h>

namespace AZ
{
    namespace Render
    {
        void EditorImageBasedLightComponent::Reflect(AZ::ReflectContext* context)
        {
            BaseClass::Reflect(context);

            if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<EditorImageBasedLightComponent, BaseClass>()
                    ->Version(1, ConvertToEditorRenderComponentAdapter<1>);

                if (AZ::EditContext* editContext = serializeContext->GetEditContext())
                {
                    editContext->Class<EditorImageBasedLightComponent>(
                        "Global Skylight (IBL)", "Adds image based illumination to the scene")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                            ->Attribute(AZ::Edit::Attributes::Category, "Atom")
                            ->Attribute(AZ::Edit::Attributes::Icon, "Editor/Icons/Components/Component_Placeholder.svg")
                            ->Attribute(AZ::Edit::Attributes::ViewportIcon, "editor/icons/components/viewport/component_placeholder.png")
                            ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Game", 0x232b318c))
                            ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                            ->Attribute(AZ::Edit::Attributes::HelpPageURL, "https://docs.aws.amazon.com/lumberyard/latest/userguide/component-imagebasedlight.html")
                        ;

                    editContext->Class<ImageBasedLightComponentController>(
                        "ImageBasedLightComponentController", "")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                            ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->DataElement(AZ::Edit::UIHandlers::Default, &ImageBasedLightComponentController::m_configuration, "Configuration", "")
                            ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                        ;

                    editContext->Class<ImageBasedLightComponentConfig>(
                        "ImageBasedLightComponentConfig", "")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                            ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->DataElement(AZ::Edit::UIHandlers::Default, &ImageBasedLightComponentConfig::m_diffuseImageAsset, "Diffuse Image", "Cubemap image asset for determining diffuse lighting")
                        ->DataElement(AZ::Edit::UIHandlers::Default, &ImageBasedLightComponentConfig::m_specularImageAsset, "Specular Image", "Cubemap image asset for determining specular lighting")
                        ->DataElement(AZ::Edit::UIHandlers::Slider, &ImageBasedLightComponentConfig::m_exposure, "Exposure", "Exposure in stops")
                            ->Attribute(AZ::Edit::Attributes::SoftMin, -5.0f)
                            ->Attribute(AZ::Edit::Attributes::SoftMax, 5.0f)
                            ->Attribute(AZ::Edit::Attributes::Min, -20.0f)
                            ->Attribute(AZ::Edit::Attributes::Max, 20.0f)
                        ;
                }
            }

            if (auto behaviorContext = azrtti_cast<BehaviorContext*>(context))
            {
                behaviorContext->Class<EditorImageBasedLightComponent>()->RequestBus("ImageBasedLightComponentRequestBus");

                behaviorContext->ConstantProperty("EditorImageBasedLightComponentTypeId", BehaviorConstant(Uuid(EditorImageBasedLightComponentTypeId)))
                    ->Attribute(AZ::Script::Attributes::Module, "render")
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation);
            }
        }

        EditorImageBasedLightComponent::EditorImageBasedLightComponent(const ImageBasedLightComponentConfig& config)
            : BaseClass(config)
        {
        }

    } // namespace Render
} // namespace AZ
