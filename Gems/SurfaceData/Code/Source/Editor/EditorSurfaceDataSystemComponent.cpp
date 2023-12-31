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

#include "SurfaceData_precompiled.h"
#include "EditorSurfaceDataSystemComponent.h"
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzFramework/Asset/GenericAssetHandler.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyEditorAPI.h>


namespace SurfaceData
{
    namespace Details
    {
        AzFramework::GenericAssetHandler<EditorSurfaceTagListAsset>* s_surfaceTagListAssetHandler = nullptr;

        void RegisterAssethandlers()
        {
            s_surfaceTagListAssetHandler = aznew AzFramework::GenericAssetHandler<EditorSurfaceTagListAsset>("Surface Tag Name List", "Other", "surfaceTagNameList");
            s_surfaceTagListAssetHandler->Register();
        }

        void UnregisterAssethandlers()
        {
            if (s_surfaceTagListAssetHandler)
            {
                s_surfaceTagListAssetHandler->Unregister();
                delete s_surfaceTagListAssetHandler;
                s_surfaceTagListAssetHandler = nullptr;
            }
        }
    }

    void EditorSurfaceDataSystemConfig::Reflect(AZ::ReflectContext* context)
    {
        EditorSurfaceTagListAsset::Reflect(context);

        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<EditorSurfaceDataSystemConfig, AZ::ComponentConfig>()
                ->Version(0)
                ;

            AZ::EditContext* edit = serialize->GetEditContext();
            if (edit)
            {
                edit->Class<EditorSurfaceDataSystemConfig>(
                    "Editor Surface Data System Config", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ;
            }
        }
    }

    void EditorSurfaceDataSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        EditorSurfaceDataSystemConfig::Reflect(context);

        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<EditorSurfaceDataSystemComponent, AzToolsFramework::Components::EditorComponentBase>()
                ->Version(0)
                ->Field("Configuration", &EditorSurfaceDataSystemComponent::m_configuration)
                ;

            if (AZ::EditContext* ec = serialize->GetEditContext())
            {
                ec->Class<EditorSurfaceDataSystemComponent>("Editor Surface Data System", "Manages discovery and registration of surface tag list assets")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("System", 0xc94d118b))
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(0, &EditorSurfaceDataSystemComponent::m_configuration, "Configuration", "")
                    ;
            }
        }
    }

    void EditorSurfaceDataSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC("SurfaceDataTagProviderService", 0x21e6b583));
    }

    void EditorSurfaceDataSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC("SurfaceDataTagProviderService", 0x21e6b583));
    }

    void EditorSurfaceDataSystemComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC("SurfaceDataSystemService", 0x1d44d25f));
    }

    void EditorSurfaceDataSystemComponent::Init()
    {
        AzToolsFramework::Components::EditorComponentBase::Init();
    }

    void EditorSurfaceDataSystemComponent::Activate()
    {
        Details::RegisterAssethandlers();
        AzFramework::AssetCatalogEventBus::Handler::BusConnect();
        AzToolsFramework::Components::EditorComponentBase::Activate();
        SurfaceDataTagProviderRequestBus::Handler::BusConnect();
    }

    void EditorSurfaceDataSystemComponent::Deactivate()
    {
        AzFramework::AssetCatalogEventBus::Handler::BusDisconnect();
        AzToolsFramework::Components::EditorComponentBase::Deactivate();
        SurfaceDataTagProviderRequestBus::Handler::BusDisconnect();
        Details::UnregisterAssethandlers();
        AZ::Data::AssetBus::MultiHandler::BusDisconnect();
    }

    void EditorSurfaceDataSystemComponent::GetRegisteredSurfaceTagNames(SurfaceTagNameSet& masks) const
    {
        for (const auto& tagName : Constants::s_allTagNames)
        {
            masks.insert(tagName);
        }

        for (const auto& assetPair : m_surfaceTagNameAssets)
        {
            const auto& asset = assetPair.second;
            if (asset.IsReady())
            {
                const auto& tags = asset.Get()->m_surfaceTagNames;
                masks.insert(tags.begin(), tags.end());
            }
        }
    }

    void EditorSurfaceDataSystemComponent::OnCatalogLoaded(const char* /*catalogFile*/)
    {
        //automatically register all surface tag list assets

        // First run through all the assets and trigger loads on them.
        AZ::Data::AssetCatalogRequestBus::Broadcast(&AZ::Data::AssetCatalogRequestBus::Events::EnumerateAssets,
            nullptr,
            [this](const AZ::Data::AssetId assetId, const AZ::Data::AssetInfo& assetInfo) {
                const auto assetType = azrtti_typeid<EditorSurfaceTagListAsset>();
                if (assetInfo.m_assetType == assetType)
                {
                    m_surfaceTagNameAssets[assetId] = AZ::Data::AssetManager::Instance().GetAsset(assetId, assetType, AZ::Data::AssetLoadBehavior::Default);
                }
            },
            nullptr);

        // After all the loads are triggered, block to make sure they've all completed.
        for (auto& asset : m_surfaceTagNameAssets)
        {
            asset.second.BlockUntilLoadComplete();
        }
    }

    void EditorSurfaceDataSystemComponent::OnCatalogAssetAdded(const AZ::Data::AssetId& assetId)
    {
        AZ::Data::AssetInfo assetInfo;
        AZ::Data::AssetCatalogRequestBus::BroadcastResult(assetInfo, &AZ::Data::AssetCatalogRequests::GetAssetInfoById, assetId);

        const auto assetType = azrtti_typeid<EditorSurfaceTagListAsset>();
        if (assetInfo.m_assetType == assetType)
        {
            AZ::Data::AssetBus::MultiHandler::BusConnect(assetId);
        }
    }

    void EditorSurfaceDataSystemComponent::OnCatalogAssetChanged(const AZ::Data::AssetId& assetId)
    {
        AZ::Data::AssetInfo assetInfo;
        AZ::Data::AssetCatalogRequestBus::BroadcastResult(assetInfo, &AZ::Data::AssetCatalogRequests::GetAssetInfoById, assetId);

        const auto assetType = azrtti_typeid<EditorSurfaceTagListAsset>();
        if (assetInfo.m_assetType == assetType)
        {
            AZ::Data::AssetBus::MultiHandler::BusConnect(assetId);
        }
    }

    void EditorSurfaceDataSystemComponent::OnCatalogAssetRemoved(const AZ::Data::AssetId& assetId, const AZ::Data::AssetInfo& /*assetInfo*/)
    {
        m_surfaceTagNameAssets.erase(assetId);
    }

    void EditorSurfaceDataSystemComponent::OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
        OnAssetReady(asset);
    }

    void EditorSurfaceDataSystemComponent::OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
        AZ::Data::AssetBus::MultiHandler::BusDisconnect(asset.GetId());
        AddAsset(asset);
    }

    void EditorSurfaceDataSystemComponent::AddAsset(AZ::Data::Asset<AZ::Data::AssetData>& asset)
    {
        const auto assetType = azrtti_typeid<EditorSurfaceTagListAsset>();
        if (asset.GetType() == assetType)
        {
            m_surfaceTagNameAssets[asset.GetId()] = asset;
            AzToolsFramework::PropertyEditorGUIMessages::Bus::Broadcast(&AzToolsFramework::PropertyEditorGUIMessages::RequestRefresh, AzToolsFramework::PropertyModificationRefreshLevel::Refresh_AttributesAndValues);
        }
    }
}
