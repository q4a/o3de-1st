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

#include <Decals/DecalTextureArrayFeatureProcessor.h>
#include <AzCore/Debug/EventTrace.h>
#include <Atom/RHI/Factory.h>
#include <Atom/RPI.Public/RPISystemInterface.h>
#include <Atom/RPI.Public/Material/Material.h>
#include <Atom/RPI.Public/Scene.h>
#include <Atom/RPI.Public/View.h>
#include <AzCore/Math/Quaternion.h>
#include <AtomCore/std/containers/array_view.h>
#include <Atom/RPI.Public/Image/ImageSystemInterface.h>
#include <Atom/RPI.Reflect/Image/StreamingImageAssetHandler.h>
#include <AtomCore/Instance/InstanceDatabase.h>
#include <Atom/RPI.Reflect/Asset/AssetUtils.h>

namespace AZ
{
    namespace Render
    {
        namespace
        {
            static AZ::RHI::Size GetTextureSizeFromMaterialAsset(const AZ::RPI::MaterialAsset* materialAsset)
            {
                for (const auto& elem : materialAsset->GetPropertyValues())
                {
                    if (elem.Is<Data::Asset<RPI::ImageAsset>>())
                    {
                        const auto& imageBinding = elem.GetValue<Data::Asset<RPI::ImageAsset>>();
                        const auto& assetId = imageBinding.GetId();
                        if (assetId.IsValid())
                        {
                            return imageBinding->GetImageDescriptor().m_size;
                        }
                    }
                }
                AZ_Assert(false, "GetSizeFromMaterial() unable to find an image in the given material.");
                return {};
            }

            static AZ::Data::Asset<AZ::RPI::MaterialAsset> QueueMaterialAssetLoad(const AZ::Data::AssetId material)
            {
                auto asset = AZ::Data::AssetManager::Instance().GetAsset<AZ::RPI::MaterialAsset>(material, AZ::Data::AssetLoadBehavior::QueueLoad);
                return asset;
            }
        }

        void DecalTextureArrayFeatureProcessor::Reflect(ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext
                    ->Class<DecalTextureArrayFeatureProcessor, RPI::FeatureProcessor>()
                    ->Version(0);
            }
        }

        void DecalTextureArrayFeatureProcessor::Activate()
        {
            m_streamingImageHandlerPreviousSetting = SetStreamingImageHandlerToLoadAllMips();

            GpuBufferHandler::Descriptor desc;
            desc.m_bufferName = "DecalBuffer";
            desc.m_bufferSrgName = "m_decals";
            desc.m_elementCountSrgName = "m_decalCount";
            desc.m_elementSize = sizeof(DecalData);
            desc.m_srgLayout = RPI::RPISystemInterface::Get()->GetViewSrgAsset()->GetLayout();

            m_decalBufferHandler = GpuBufferHandler(desc);

            CacheShaderIndices();
        }

        void DecalTextureArrayFeatureProcessor::Deactivate()
        {
            AZ::Data::AssetBus::MultiHandler::BusDisconnect();

            RestoreStreamingImageHandlerSettings(m_streamingImageHandlerPreviousSetting);

            m_decalData.Clear();
            m_decalBufferHandler.Release();
            m_materialAssets.clear();
        }

        DecalTextureArrayFeatureProcessor::DecalHandle DecalTextureArrayFeatureProcessor::AcquireDecal()
        {
            const uint16_t id = m_decalData.GetFreeSlotIndex();

            if (id == IndexedDataVector<DecalData>::NoFreeSlot)
            {
                return DecalHandle(DecalHandle::NullIndex);
            }
            else
            {
                m_deviceBufferNeedsUpdate = true;
                DecalData& decalData = m_decalData.GetData(id);
                decalData.m_textureArrayIndex = DecalData::UnusedIndex;
                return DecalHandle(id);
            }
        }

        bool DecalTextureArrayFeatureProcessor::ReleaseDecal(const DecalHandle decal)
        {
            if (decal.IsValid())
            {
                if (m_materialLoadTracker.IsAssetLoading(decal))
                {
                    m_materialLoadTracker.RemoveHandle(decal);
                }

                DecalLocation decalLocation;
                decalLocation.textureArrayIndex = m_decalData.GetData(decal.GetIndex()).m_textureArrayIndex;
                decalLocation.textureIndex = m_decalData.GetData(decal.GetIndex()).m_textureIndex;
                RemoveDecalFromTextureArrays(decalLocation);

                m_decalData.RemoveIndex(decal.GetIndex());
                m_deviceBufferNeedsUpdate = true;
                return true;
            }
            return false;
        }

        DecalTextureArrayFeatureProcessor::DecalHandle DecalTextureArrayFeatureProcessor::CloneDecal(const DecalHandle sourceDecal)
        {
            AZ_Assert(sourceDecal.IsValid(), "Invalid DecalHandle passed to DecalTextureArrayFeatureProcessor::CloneDecal().");

            const DecalHandle decal = AcquireDecal();
            if (decal.IsValid())
            {
                m_decalData.GetData(decal.GetIndex()) = m_decalData.GetData(sourceDecal.GetIndex());
                const auto materialAsset = GetMaterialUsedByDecal(sourceDecal);
                m_materialToTextureArrayLookupTable.at(materialAsset).m_useCount++;
                m_deviceBufferNeedsUpdate = true;
            }
            return decal;
        }

        void DecalTextureArrayFeatureProcessor::Simulate(const RPI::FeatureProcessor::SimulatePacket& packet)
        {
            AZ_PROFILE_FUNCTION(Debug::ProfileCategory::AzRender)
            AZ_UNUSED(packet);

            if (m_deviceBufferNeedsUpdate)
            {
                bool success = m_decalBufferHandler.UpdateBuffer(m_decalData.GetDataVector());
                AZ_Error(FeatureProcessorName, success, "Unable to update buffer during Simulate().");
                m_deviceBufferNeedsUpdate = false;
            }
        }

        void DecalTextureArrayFeatureProcessor::Render(const RPI::FeatureProcessor::RenderPacket& packet)
        {
            // Note that decals are rendered as part of the forward shading pipeline. We only need to bind the decal buffers/textures in here.
            AZ_PROFILE_FUNCTION(Debug::ProfileCategory::AzRender)

            for (const RPI::ViewPtr& view : packet.m_views)
            {
                m_decalBufferHandler.UpdateSrg(view->GetShaderResourceGroup().get());
                SetPackedTexturesToSrg(view);
            }
        }

        void DecalTextureArrayFeatureProcessor::SetDecalData(const DecalHandle handle, const DecalData& data)
        {
            if (handle.IsValid())
            {
                m_decalData.GetData(handle.GetIndex()) = data;
                m_deviceBufferNeedsUpdate = true;
            }
            else
            {
                AZ_Warning("DecalTextureArrayFeatureProcessor", false, "Invalid handle passed to DecalTextureArrayFeatureProcessor::SetDecalData().");
            }
        }

        const Data::Instance<RPI::Buffer> DecalTextureArrayFeatureProcessor::GetDecalBuffer() const
        {
            return m_decalBufferHandler.GetBuffer();
        }

        uint32_t DecalTextureArrayFeatureProcessor::GetDecalCount() const
        {
            return m_decalBufferHandler.GetElementCount();
        }

        void DecalTextureArrayFeatureProcessor::SetDecalPosition(const DecalHandle handle, const AZ::Vector3& position)
        {
            if (handle.IsValid())
            {
                AZStd::array<float, 3>& writePos = m_decalData.GetData(handle.GetIndex()).m_position;
                position.StoreToFloat3(writePos.data());
                m_deviceBufferNeedsUpdate = true;
            }
            else
            {
                AZ_Warning("DecalTextureArrayFeatureProcessor", false, "Invalid handle passed to DecalTextureArrayFeatureProcessor::SetDecalPosition().");
            }
        }

        void DecalTextureArrayFeatureProcessor::SetDecalOrientation(DecalHandle handle, const AZ::Quaternion& orientation)
        {
            if (handle.IsValid())
            {
                orientation.StoreToFloat4(m_decalData.GetData(handle.GetIndex()).m_quaternion.data());
                m_deviceBufferNeedsUpdate = true;
            }
            else
            {
                AZ_Warning("DecalTextureArrayFeatureProcessor", false, "Invalid handle passed to DecalTextureArrayFeatureProcessor::SetDecalOrientation().");
            }
        }

        void DecalTextureArrayFeatureProcessor::SetDecalHalfSize(DecalHandle handle, const Vector3& halfSize)
        {
            if (handle.IsValid())
            {
                halfSize.StoreToFloat3(m_decalData.GetData(handle.GetIndex()).m_halfSize.data());
                m_deviceBufferNeedsUpdate = true;
            }
            else
            {
                AZ_Warning("DecalTextureArrayFeatureProcessor", false, "Invalid handle passed to DecalTextureArrayFeatureProcessor::SetDecalHalfSize().");
            }
        }

        void DecalTextureArrayFeatureProcessor::SetDecalAttenuationAngle(const DecalHandle handle, float angleAttenuation)
        {
            if (handle.IsValid())
            {
                m_decalData.GetData(handle.GetIndex()).m_angleAttenuation = angleAttenuation;
                m_deviceBufferNeedsUpdate = true;
            }
            else
            {
                AZ_Warning("DecalTextureArrayFeatureProcessor", handle.IsValid(), "Invalid handle passed to DecalTextureArrayFeatureProcessor::SetDecalAttenuationAngle().");
            }
        }

        void DecalTextureArrayFeatureProcessor::SetDecalOpacity(const DecalHandle handle, float opacity)
        {
            if (handle.IsValid())
            {
                m_decalData.GetData(handle.GetIndex()).m_opacity = opacity;

                m_deviceBufferNeedsUpdate = true;
            }
            else
            {
                AZ_Warning("DecalTextureArrayFeatureProcessor", false, "Invalid handle passed to DecalTextureArrayFeatureProcessor::SetDecalOpacity().");
            }
        }

        void DecalTextureArrayFeatureProcessor::SetDecalSortKey(DecalHandle handle, uint8_t sortKey)
        {
            if (handle.IsValid())
            {
                m_decalData.GetData(handle.GetIndex()).m_sortKey = sortKey;
                m_deviceBufferNeedsUpdate = true;
            }
            else
            {
                AZ_Warning("DecalTextureArrayFeatureProcessor", false, "Invalid handle passed to DecalTextureArrayFeatureProcessor::SetDecalSortKey().");
            }
        }

        void DecalTextureArrayFeatureProcessor::SetDecalTransform(DecalHandle handle, const AZ::Transform& world)
        {
            if (handle.IsValid())
            {
                SetDecalHalfSize(handle, world.GetScale());
                SetDecalPosition(handle, world.GetTranslation());
                SetDecalOrientation(handle, world.GetRotation());

                m_deviceBufferNeedsUpdate = true;
            }
            else
            {
                AZ_Warning("DecalTextureArrayFeatureProcessor", false, "Invalid handle passed to DecalTextureArrayFeatureProcessor::SetDecalTransform().");
            }
        }

        void DecalTextureArrayFeatureProcessor::SetDecalMaterial(const DecalHandle handle, const AZ::Data::AssetId material)
        {
            AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::Renderer);
            if (handle.IsNull())
            {
                AZ_Warning("DecalTextureArrayFeatureProcessor", false, "Invalid handle passed to DecalTextureArrayFeatureProcessor::SetDecalMaterial().");
                return;
            }

            if (material.IsValid())
            {
                AZ_Assert(m_decalData.GetData(handle.GetIndex()).m_textureArrayIndex == DecalData::UnusedIndex, "Setting Material on a decal more than once is not currently supported.");

                const auto iter = m_materialToTextureArrayLookupTable.find(material);
                if (iter != m_materialToTextureArrayLookupTable.end())
                {
                    // This material is already loaded and registered with this feature processor
                    iter->second.m_useCount++;
                    SetDecalTextureLocation(handle, iter->second.m_location);
                    return;
                }

                // Material not loaded so queue it up for loading.
                QueueMaterialLoadForDecal(material, handle);
                return;
            }
        }

        void DecalTextureArrayFeatureProcessor::CacheShaderIndices()
        {
            for (int i = 0; i < NumTextureArrays; ++i)
            {
                const RHI::ShaderResourceGroupLayout* viewSrgLayout = RPI::RPISystemInterface::Get()->GetViewSrgAsset()->GetLayout();
                const AZStd::string baseName = "m_decalTextureArray" + AZStd::to_string(i);

                m_decalTextureArrayIndices[i] = viewSrgLayout->FindShaderInputImageIndex(Name(baseName.c_str()));
                AZ_Warning("DecalTextureArrayFeatureProcessor", m_decalTextureArrayIndices[i].IsValid(), "Unable to find %s in decal shader.", baseName.c_str());
            }
        }

        AZStd::optional<AZ::Render::DecalTextureArrayFeatureProcessor::DecalLocation> DecalTextureArrayFeatureProcessor::AddMaterialToTextureArrays(const AZ::RPI::MaterialAsset* materialAsset)
        {
            const RHI::Size textureSize = GetTextureSizeFromMaterialAsset(materialAsset);

            int textureArrayIndex = FindTextureArrayWithSize(textureSize);
            const bool wasExistingTextureArrayFoundForGivenSize = textureArrayIndex != -1;
            if (m_textureArrayList.size() == NumTextureArrays && !wasExistingTextureArrayFoundForGivenSize)
            {
                AZ_Warning("DecalTextureArrayFeatureProcessor", false, "Unable to add decal with size %u %u. There are no more texture arrays left to accept a decal with this size permutation.", textureSize.m_width, textureSize.m_height);
                return AZStd::nullopt;
            }

            int textureIndex;
            if (!wasExistingTextureArrayFoundForGivenSize)
            {
                DecalTextureArray decalTextureArray;
                textureIndex = decalTextureArray.AddMaterial(materialAsset->GetId());
                textureArrayIndex = m_textureArrayList.push_front(AZStd::make_pair(textureSize, decalTextureArray));
            }
            else
            {
                textureIndex = m_textureArrayList[textureArrayIndex].second.AddMaterial(materialAsset->GetId());
            }

            DecalLocation result;
            result.textureArrayIndex = textureArrayIndex;
            result.textureIndex = textureIndex;
            return result;
        }

        const bool DecalTextureArrayFeatureProcessor::SetStreamingImageHandlerToLoadAllMips()
        {
            // All mips need to be available upfront for packing into a texture array
            // [GFX TODO[ATOM-13694] - Investigate if we can remove SetStreamingImageHandlerToLoadAllMips() from the Decal system
            using namespace AZ;
            RPI::StreamingImageAssetHandler* imageAssetHandler = static_cast<RPI::StreamingImageAssetHandler*>(
                Data::AssetManager::Instance().GetHandler(RPI::StreamingImageAsset::RTTI_Type()));

            bool savedLoadMipEnabled = imageAssetHandler->GetLoadMipChainsEnabled();
            imageAssetHandler->SetLoadMipChainsEnabled(true);
            return savedLoadMipEnabled;
        }

        void DecalTextureArrayFeatureProcessor::RestoreStreamingImageHandlerSettings(const bool oldValue)
        {
            using namespace AZ;
            RPI::StreamingImageAssetHandler* imageAssetHandler = static_cast<RPI::StreamingImageAssetHandler*>(
                Data::AssetManager::Instance().GetHandler(RPI::StreamingImageAsset::RTTI_Type()));

            imageAssetHandler->SetLoadMipChainsEnabled(oldValue);
        }

        void DecalTextureArrayFeatureProcessor::OnAssetReady(const Data::Asset<Data::AssetData> asset)
        {
            AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::Renderer);
            const Data::AssetId& assetId = asset->GetId();
            
            const RPI::MaterialAsset* materialAsset = asset.GetAs<AZ::RPI::MaterialAsset>();
            const bool validDecalMaterial = materialAsset && DecalTextureArray::IsValidDecalMaterial(*materialAsset);
            if (validDecalMaterial)
            {
                const auto& decalsThatUseThisMaterial = m_materialLoadTracker.GetHandlesByAsset(asset.GetId());
                const auto& decalLocation = AddMaterialToTextureArrays(materialAsset);

                if (decalLocation)
                {
                    for (const auto& decal : decalsThatUseThisMaterial)
                    {
                        m_materialToTextureArrayLookupTable[assetId].m_useCount++;
                        SetDecalTextureLocation(decal, *decalLocation);
                    }
                    m_materialToTextureArrayLookupTable[assetId].m_location = *decalLocation;
                }
            }
            else
            {
                AZ_Warning("DecalTextureArrayFeatureProcessor", false, "DecalTextureArray::IsValidDecalMaterial() failed, unable to add this material to the decal");
            }

            m_materialLoadTracker.RemoveAllHandlesWithAsset(assetId);
            AZ::Data::AssetBus::MultiHandler::BusDisconnect(assetId);

            if (!m_materialLoadTracker.AreAnyLoadsInFlight())
            {
                PackTexureArrays();
            }
        }

        void DecalTextureArrayFeatureProcessor::SetDecalTextureLocation(const DecalHandle& handle, const DecalLocation location)
        {
            AZ_Assert(handle.IsValid(), "SetDecalTextureLocation called with invalid handle");
            m_decalData.GetData(handle.GetIndex()).m_textureArrayIndex = location.textureArrayIndex;
            m_decalData.GetData(handle.GetIndex()).m_textureIndex = location.textureIndex;
            m_deviceBufferNeedsUpdate = true;
        }

        void DecalTextureArrayFeatureProcessor::SetPackedTexturesToSrg(const RPI::ViewPtr& view)
        {
            int iter = m_textureArrayList.begin();
            while (iter != -1)
            {
                const auto packedTexture = m_textureArrayList[iter].second.GetPackedTexture();
                view->GetShaderResourceGroup()->SetImage(m_decalTextureArrayIndices[iter], packedTexture);
                iter = m_textureArrayList.next(iter);
            }
        }

        int DecalTextureArrayFeatureProcessor::FindTextureArrayWithSize(const RHI::Size& size) const
        {
            int iter = m_textureArrayList.begin();
            while (iter != -1)
            {
                if (m_textureArrayList[iter].first == size)
                {
                    return iter;
                }

                iter = m_textureArrayList.next(iter);
            }
            return -1;
        }

        bool DecalTextureArrayFeatureProcessor::RemoveDecalFromTextureArrays(const DecalLocation decalLocation)
        {
            if (decalLocation.textureArrayIndex != DecalData::UnusedIndex)
            {
                auto& textureArray = m_textureArrayList[decalLocation.textureArrayIndex].second;

                const AZ::Data::AssetId material = textureArray.GetMaterialAssetId(decalLocation.textureIndex);
                auto iter = m_materialToTextureArrayLookupTable.find(material);
                AZ_Assert(iter != m_materialToTextureArrayLookupTable.end(), "Bad state");
                DecalLocationAndUseCount& decalInformation = iter->second;
                decalInformation.m_useCount--;

                if (decalInformation.m_useCount == 0)
                {
                    m_materialToTextureArrayLookupTable.erase(iter);
                    textureArray.RemoveMaterial(decalLocation.textureIndex);
                }

                if (textureArray.NumMaterials() == 0)
                {
                    m_textureArrayList.erase(decalLocation.textureArrayIndex);
                }
            }
            return false;
        }

        void DecalTextureArrayFeatureProcessor::PackTexureArrays()
        {
            int iter = m_textureArrayList.begin();
            while (iter != -1)
            {
                m_textureArrayList[iter].second.Pack();
                iter = m_textureArrayList.next(iter);
            }
        }

        AZ::Data::AssetId DecalTextureArrayFeatureProcessor::GetMaterialUsedByDecal(const DecalHandle handle) const
        {
            AZ::Data::AssetId material;
            if (handle.IsValid())
            {
                const DecalData& decalData = m_decalData.GetData(handle.GetIndex());
                if (decalData.m_textureArrayIndex != DecalData::UnusedIndex)
                {
                    const DecalTextureArray& textureArray = m_textureArrayList[decalData.m_textureArrayIndex].second;
                    material = textureArray.GetMaterialAssetId(decalData.m_textureIndex);
                }
            }
            return material;
        }

        void DecalTextureArrayFeatureProcessor::QueueMaterialLoadForDecal(const AZ::Data::AssetId material, const DecalHandle handle)
        {
            // Note that another decal might have already queued this material for loading
            if (m_materialLoadTracker.IsAssetLoading(material))
            {
                m_materialLoadTracker.TrackAssetLoad(handle, material);
                return;
            }

            const auto materialAsset = QueueMaterialAssetLoad(material);
            m_materialAssets.emplace(material, materialAsset);
            m_materialLoadTracker.TrackAssetLoad(handle, material);

            if (materialAsset.IsLoading())
            {
                AZ::Data::AssetBus::MultiHandler::BusConnect(material);
            }
            else if (materialAsset.IsReady())
            {
                OnAssetReady(materialAsset);
            }
            else
            {
                AZ_Assert(false, "DecalTextureArrayFeatureProcessor::QueueMaterialLoadForDecal is in an unhandled state.");
            }
        }

    } // namespace Render
} // namespace AZ
