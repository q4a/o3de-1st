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

#include <Mesh/EditorMeshComponent.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/Entity/EditorEntityInfoBus.h>
#include <AzToolsFramework/API/EntityCompositionRequestBus.h>

#include <AtomLyIntegration/CommonFeatures/Material/MaterialComponentConstants.h>

namespace AZ
{
    namespace Render
    {
        void EditorMeshComponent::Reflect(AZ::ReflectContext* context)
        {
            BaseClass::Reflect(context);

            if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<EditorMeshComponent, BaseClass>()
                    ->Version(1, ConvertToEditorRenderComponentAdapter<1>);

                // This shouldn't be registered here, but is required to make a vector from EditorMeshComponentTypeId. This can be removed when one of the following happens:
                // - The generic type for AZStd::vector<AZ::Uuid> is registered in a more generic place
                // - EditorLevelComponentAPIComponent has a version of AddComponentsOfType that takes a single Uuid instead of a vector
                serializeContext->RegisterGenericType<AZStd::vector<AZ::Uuid>>();

                if (AZ::EditContext* editContext = serializeContext->GetEditContext())
                {
                    editContext->Class<EditorMeshComponent>(
                        "Mesh", "The mesh component is the primary method of adding visual geometry to entities")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                            ->Attribute(AZ::Edit::Attributes::Category, "Atom")
                            ->Attribute(AZ::Edit::Attributes::Icon, "Editor/Icons/Components/Component_Placeholder.svg")
                            ->Attribute(AZ::Edit::Attributes::ViewportIcon, "editor/icons/components/viewport/component_placeholder.png")
                            ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Game", 0x232b318c))
                            ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                            ->Attribute(AZ::Edit::Attributes::HelpPageURL, "https://docs.aws.amazon.com/lumberyard/latest/userguide/component-mesh.html")
                            ->Attribute(AZ::Edit::Attributes::PrimaryAssetType, AZ::AzTypeInfo<RPI::ModelAsset>::Uuid())
                        ->UIElement(AZ::Edit::UIHandlers::Button, "Add Material Component", "Add Material Component")
                            ->Attribute(AZ::Edit::Attributes::NameLabelOverride, "")
                            ->Attribute(AZ::Edit::Attributes::ButtonText, "Add Material Component")
                            ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorMeshComponent::AddEditorMaterialComponent)
                            ->Attribute(AZ::Edit::Attributes::Visibility, &EditorMeshComponent::GetEditorMaterialComponentVisibility)
                        ;

                    editContext->Class<MeshComponentController>(
                        "MeshComponentController", "")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                            ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->DataElement(AZ::Edit::UIHandlers::Default, &MeshComponentController::m_configuration, "Configuration", "")
                            ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                        ;

                    editContext->Class<MeshComponentConfig>(
                        "MeshComponentConfig", "")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                            ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->DataElement(AZ::Edit::UIHandlers::Default, &MeshComponentConfig::m_modelAsset, "Mesh Asset", "Mesh asset reference")
                        ->DataElement(AZ::Edit::UIHandlers::Default, &MeshComponentConfig::m_sortKey, "Sort Key", "Transparent meshes are drawn by sort key then depth. Used this to force certain transparent meshes to draw before or after others.")
                        ->DataElement(AZ::Edit::UIHandlers::ComboBox, &MeshComponentConfig::m_lodOverride, "Lod Override", "Allows the rendered LOD to be overridden instead of being calculated automatically.")
                            ->Attribute(AZ::Edit::Attributes::EnumValues, &MeshComponentConfig::GetLodOverrideValues)
                            ->Attribute(AZ::Edit::Attributes::Visibility, &MeshComponentConfig::IsAssetSet)
                        ->ClassElement(AZ::Edit::ClassElements::Group, "Reflections")
                            ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                            ->DataElement(AZ::Edit::UIHandlers::CheckBox, &MeshComponentConfig::m_excludeFromReflectionCubeMaps, "Exclude from reflection cubemaps", "Mesh will not be visible in baked reflection probe cubemaps")
                                ->Attribute(AZ::Edit::Attributes::ChangeNotify, Edit::PropertyRefreshLevels::ValuesOnly)
                        ;
                }
            }

            if (BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
            {
                behaviorContext->ConstantProperty("EditorMeshComponentTypeId", BehaviorConstant(Uuid(EditorMeshComponentTypeId)))
                    ->Attribute(AZ::Script::Attributes::Module, "render")
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation);

                behaviorContext->Class<EditorMeshComponent>()->RequestBus("RenderMeshComponentRequestBus");
            }
        }

        EditorMeshComponent::EditorMeshComponent(const MeshComponentConfig& config)
            : BaseClass(config)
        {
        }

        void EditorMeshComponent::Activate()
        {
            BaseClass::Activate();
            AzToolsFramework::EditorComponentSelectionRequestsBus::Handler::BusConnect(GetEntityId());
            AzFramework::EntityDebugDisplayEventBus::Handler::BusConnect(GetEntityId());
            MeshComponentNotificationBus::Handler::BusConnect(GetEntityId());
        }

        void EditorMeshComponent::Deactivate()
        {
            MeshComponentNotificationBus::Handler::BusDisconnect();
            AzFramework::EntityDebugDisplayEventBus::Handler::BusDisconnect();
            AzToolsFramework::EditorComponentSelectionRequestsBus::Handler::BusDisconnect();
            BaseClass::Deactivate();
        }

        void EditorMeshComponent::SetPrimaryAsset(const AZ::Data::AssetId& assetId)
        {
            m_controller.SetModelAssetId(assetId);
        }

        AZ::Aabb EditorMeshComponent::GetEditorSelectionBoundsViewport(
            [[maybe_unused]] const AzFramework::ViewportInfo& viewportInfo)
        {
            return m_controller.GetWorldBounds();
        }

        bool EditorMeshComponent::EditorSelectionIntersectRayViewport(
            [[maybe_unused]] const AzFramework::ViewportInfo& viewportInfo, const AZ::Vector3& src,
            const AZ::Vector3& dir, float& distance)
        {
            if (!m_controller.GetModel())
            {
                return false;
            }

            AZ::Transform transform = AZ::Transform::CreateIdentity();
            AZ::TransformBus::EventResult(transform, GetEntityId(), &AZ::TransformBus::Events::GetWorldTM);

            return m_controller.GetModel()->RayIntersection(transform, src, dir, distance);
        }

        bool EditorMeshComponent::SupportsEditorRayIntersect()
        {
            return true;
        }

        void EditorMeshComponent::DisplayEntityViewport(
            const AzFramework::ViewportInfo&, AzFramework::DebugDisplayRequests& debugDisplay)
        {
            if (!IsSelected())
            {
                return;
            }

            const AZ::Aabb localAabb = m_controller.GetLocalBounds();
            if (!localAabb.IsValid())
            {
                return;
            }

            AZ::Transform worldTM;
            AZ::TransformBus::EventResult(worldTM, GetEntityId(), &AZ::TransformBus::Events::GetWorldTM);

            debugDisplay.PushMatrix(worldTM);

            debugDisplay.DrawWireBox(localAabb.GetMin(), localAabb.GetMax());

            debugDisplay.PopMatrix();
        }

        AZ::Crc32 EditorMeshComponent::AddEditorMaterialComponent()
        {
            const AZStd::vector<AZ::EntityId> entityList = { GetEntityId() };
            const AZ::ComponentTypeList componentsToAdd = { AZ::Uuid(AZ::Render::EditorMaterialComponentTypeId) };

            AzToolsFramework::EntityCompositionRequests::AddComponentsOutcome outcome =
                AZ::Failure(AZStd::string("Failed to add AZ::Render::EditorMaterialComponentTypeId"));
            AzToolsFramework::EntityCompositionRequestBus::BroadcastResult(outcome, &AzToolsFramework::EntityCompositionRequests::AddComponentsToEntities, entityList, componentsToAdd);
            return Edit::PropertyRefreshLevels::EntireTree;
        }

        bool EditorMeshComponent::HasEditorMaterialComponent() const
        {
            return GetEntity() && GetEntity()->FindComponent(AZ::Uuid(AZ::Render::EditorMaterialComponentTypeId)) != nullptr;
        }

        AZ::u32 EditorMeshComponent::GetEditorMaterialComponentVisibility() const
        {
            return HasEditorMaterialComponent() ? AZ::Edit::PropertyVisibility::Hide : AZ::Edit::PropertyVisibility::Show;
        }

        void EditorMeshComponent::OnModelReady(const Data::Asset<RPI::ModelAsset>& /*modelAsset*/, const Data::Instance<RPI::Model>& /*model*/)
        {
            // Refresh the tree when the model loads to update UI based on the model.
            AzToolsFramework::ToolsApplicationEvents::Bus::Broadcast(
                &AzToolsFramework::ToolsApplicationEvents::InvalidatePropertyDisplay,
                AzToolsFramework::Refresh_EntireTree);
        }

    } // namespace Render
} // namespace AZ
