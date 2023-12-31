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

#include <ScriptCanvas/Core/Graph.h>
#include <ScriptCanvas/Asset/RuntimeAsset.h>
#include <ScriptCanvas/Asset/RuntimeAssetHandler.h>
#include <ScriptCanvas/Execution/RuntimeComponent.h>

#include <ScriptCanvas/Core/ScriptCanvasBus.h>

#include <AzCore/IO/GenericStreams.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/Serialization/Utils.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Component/Entity.h>


namespace ScriptCanvas
{
    //=========================================================================
    // RuntimeAssetHandler
    //=========================================================================
    RuntimeAssetHandler::RuntimeAssetHandler(AZ::SerializeContext* context)
    {
        SetSerializeContext(context);

        AZ::AssetTypeInfoBus::MultiHandler::BusConnect(AZ::AzTypeInfo<RuntimeAsset>::Uuid());
    }

    RuntimeAssetHandler::~RuntimeAssetHandler()
    {
        AZ::AssetTypeInfoBus::MultiHandler::BusDisconnect();
    }

    AZ::Data::AssetType RuntimeAssetHandler::GetAssetType() const
    {
        return AZ::AzTypeInfo<RuntimeAsset>::Uuid();
    }

    const char* RuntimeAssetHandler::GetAssetTypeDisplayName() const
    {
        return "Script Canvas Runtime Graph";
    }

    const char* RuntimeAssetHandler::GetGroup() const
    {
        return "Script";
    }

    const char* RuntimeAssetHandler::GetBrowserIcon() const
    {
        return "Editor/Icons/ScriptCanvas/Viewport/ScriptCanvas.png";
    }

    AZ::Uuid RuntimeAssetHandler::GetComponentTypeId() const
    {
        return azrtti_typeid<RuntimeComponent>();
    }

    void RuntimeAssetHandler::GetAssetTypeExtensions(AZStd::vector<AZStd::string>& extensions)
    {
        const AZ::Uuid& assetType = *AZ::AssetTypeInfoBus::GetCurrentBusId();
        if (assetType == AZ::AzTypeInfo<RuntimeAsset>::Uuid())
        {
            extensions.push_back(RuntimeAsset::GetFileExtension());
        }
    }

    bool RuntimeAssetHandler::CanCreateComponent([[maybe_unused]] const AZ::Data::AssetId& assetId) const
    {
        // This is a runtime component so we shouldn't be making components at edit time for this
        return false;
    }

    AZ::Data::AssetPtr RuntimeAssetHandler::CreateAsset(const AZ::Data::AssetId& id, const AZ::Data::AssetType& type)
    {
        (void)type;
        AZ_Assert(type == AZ::AzTypeInfo<RuntimeAsset>::Uuid(), "This handler deals only with the Script Canvas Runtime Asset type!");

        return aznew RuntimeAsset(id);
    }

    AZ::Data::AssetHandler::LoadResult RuntimeAssetHandler::LoadAssetData(
        const AZ::Data::Asset<AZ::Data::AssetData>& asset,
        AZStd::shared_ptr<AZ::Data::AssetDataStream> stream,
        const AZ::Data::AssetFilterCB& assetLoadFilterCB)
    {
        RuntimeAsset* runtimeAsset = asset.GetAs<RuntimeAsset>();
        AZ_Assert(runtimeAsset, "This should be a Script Canvas runtime asset, as this is the only type we process!");
        if (runtimeAsset && m_serializeContext)
        {
            stream->Seek(0U, AZ::IO::GenericStream::ST_SEEK_BEGIN);
            bool loadSuccess = AZ::Utils::LoadObjectFromStreamInPlace(*stream, runtimeAsset->m_runtimeData, m_serializeContext, AZ::ObjectStream::FilterDescriptor(assetLoadFilterCB));
            return loadSuccess ? AZ::Data::AssetHandler::LoadResult::LoadComplete : AZ::Data::AssetHandler::LoadResult::Error;
        }
        return AZ::Data::AssetHandler::LoadResult::Error;
    }

    bool RuntimeAssetHandler::SaveAssetData(const AZ::Data::Asset<AZ::Data::AssetData>& asset, AZ::IO::GenericStream* stream)
    {
        RuntimeAsset* runtimeAsset = asset.GetAs<RuntimeAsset>();
        AZ_Assert(runtimeAsset, "This should be a Script Canvas runtime asset, as this is the only type we process!");
        if (runtimeAsset && m_serializeContext)
        {
            AZ::ObjectStream* binaryObjStream = AZ::ObjectStream::Create(stream, *m_serializeContext, AZ::ObjectStream::ST_XML);
            bool graphSaved = binaryObjStream->WriteClass(&runtimeAsset->m_runtimeData);
            binaryObjStream->Finalize();
            return graphSaved;
        }

        return false;
    }

    void RuntimeAssetHandler::DestroyAsset(AZ::Data::AssetPtr ptr)
    {
        delete ptr;
    }

    void RuntimeAssetHandler::GetHandledAssetTypes(AZStd::vector<AZ::Data::AssetType>& assetTypes)
    {
        assetTypes.push_back(AZ::AzTypeInfo<RuntimeAsset>::Uuid());
    }

    AZ::SerializeContext* RuntimeAssetHandler::GetSerializeContext() const
    {
        return m_serializeContext;
    }
    
    void RuntimeAssetHandler::SetSerializeContext(AZ::SerializeContext* context)
    {
        m_serializeContext = context;

        if (m_serializeContext == nullptr)
        {
            // use the default app serialize context
            AZ::ComponentApplicationBus::BroadcastResult(m_serializeContext, &AZ::ComponentApplicationBus::Events::GetSerializeContext);
            if (!m_serializeContext)
            {
                AZ_Error("Script Canvas", false, "RuntimeAssetHandler: No serialize context provided! We will not be able to process the Script Canvas Runtime Asset type");
            }
        }
    }
} // namespace AZ
