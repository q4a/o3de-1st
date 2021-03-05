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

#include <AzCore/Asset/AssetJsonSerializer.h>
#include <AzCore/Asset/AssetManagerComponent.h>
#include <AzCore/Asset/AssetManagerBus.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Serialization/Json/RegistrationContext.h>
#include <AzCore/Slice/SliceAssetHandler.h>
#include <AzCore/Slice/SliceComponent.h>
#include <AzCore/Math/Crc.h>

namespace AZ
{
    //=========================================================================
    // AssetDatabaseComponent
    // [6/25/2012]
    //=========================================================================
    AssetManagerComponent::AssetManagerComponent()
    {
    }

    //=========================================================================
    // Activate
    // [6/25/2012]
    //=========================================================================
    void AssetManagerComponent::Activate()
    {
        Data::AssetManager::Descriptor desc;
        Data::AssetManager::Create(desc);
        SystemTickBus::Handler::BusConnect();
    }

    //=========================================================================
    // Deactivate
    // [6/25/2012]
    //=========================================================================
    void AssetManagerComponent::Deactivate()
    {
        Data::AssetManager::Instance().DispatchEvents(); // clear any waiting assets.

        SystemTickBus::Handler::BusDisconnect();
        Data::AssetManager::Destroy();
    }

    //=========================================================================
    // OnTick
    // [6/25/2012]
    //=========================================================================
    void AssetManagerComponent::OnSystemTick()
    {
        Data::AssetManager::Instance().DispatchEvents();
    }

    //=========================================================================
    // GetProvidedServices
    //=========================================================================
    void AssetManagerComponent::GetProvidedServices(ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("AssetDatabaseService"));
    }

    //=========================================================================
    // GetIncompatibleServices
    //=========================================================================
    void AssetManagerComponent::GetIncompatibleServices(ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("AssetDatabaseService"));
    }

    //=========================================================================
    // GetRequiredServices
    //=========================================================================
    void AssetManagerComponent::GetRequiredServices(ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC_CE("DataStreamingService"));
        required.push_back(AZ_CRC_CE("JobsService"));
    }

    //=========================================================================
    // Reflect
    //=========================================================================
    void AssetManagerComponent::Reflect(ReflectContext* context)
    {
        Data::AssetId::Reflect(context);
        Data::AssetData::Reflect(context);

        if (SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(context))
        {
            serializeContext->RegisterGenericType<Data::Asset<Data::AssetData>>();

            serializeContext->Class<AssetManagerComponent, AZ::Component>()
                ->Version(1)
                ;

            if (EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<AssetManagerComponent>(
                    "Asset Database", "Asset database system functionality")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::Category, "Engine")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("System", 0xc94d118b))
                    ;
            }
        }

        if (BehaviorContext* behaviorContext = azrtti_cast<BehaviorContext*>(context))
        {
            behaviorContext->EBus<Data::AssetCatalogRequestBus>("AssetCatalogRequestBus")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                ->Attribute(AZ::Script::Attributes::Category, "Asset")
                ->Attribute(AZ::Script::Attributes::Module, "asset")
                ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
                ->Event("GetAssetPathById", &Data::AssetCatalogRequests::GetAssetPathById)
                ->Event("GetAssetIdByPath", &Data::AssetCatalogRequests::GetAssetIdByPath)
                ->Event("GetAssetTypeByDisplayName", &Data::AssetCatalogRequests::GetAssetTypeByDisplayName)
                ;
        }

        if (JsonRegistrationContext* jsonContext = azrtti_cast<JsonRegistrationContext*>(context))
        {
            jsonContext->Serializer<AZ::Data::AssetJsonSerializer>()->HandlesType<AZ::Data::Asset>();
        }
    }
}