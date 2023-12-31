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

#include <Atom/RPI.Reflect/Image/AttachmentImageAssetCreator.h>

#include <AzCore/Asset/AssetManager.h>

namespace AZ
{
    namespace RPI
    {
        void AttachmentImageAssetCreator::Begin(const Data::AssetId& assetId)
        {
            BeginCommon(assetId);
        }

        void AttachmentImageAssetCreator::SetImageDescriptor(const RHI::ImageDescriptor& descriptor)
        {
            if (ValidateIsReady())
            {
                m_asset->m_imageDescriptor = descriptor;
            }
        }

        void AttachmentImageAssetCreator::SetImageViewDescriptor(const RHI::ImageViewDescriptor& descriptor)
        {
            if (ValidateIsReady())
            {
                m_asset->m_imageViewDescriptor = descriptor;
            }
        }

        void AttachmentImageAssetCreator::SetPoolAsset(const Data::Asset<ResourcePoolAsset>& poolAsset)
        {
            if (ValidateIsReady())
            {
                m_asset->m_poolAsset = poolAsset;
            }
        }

        void AttachmentImageAssetCreator::SetOptimizedClearValue(const RHI::ClearValue& clearValue)
        {
            if (ValidateIsReady())
            {
                m_asset->m_isClearValueValid = true;
                m_asset->m_optimizedClearValue = clearValue;
            }
        }

        bool AttachmentImageAssetCreator::End(Data::Asset<AttachmentImageAsset>& result)
        {
            if (!ValidateIsReady())
            {
                return false;
            }

            // Validate assetId instead of validate AssetData* exist. This is mainly because the Asset<T> 's serializer only need serialize asset id instead of asset data
            // Instead of create a complete Asset<T>, we could use new Asset(assetId, type, hint) to create a placeholder to save the asset id for serialization.
            if (!m_asset->m_poolAsset.GetId().IsValid())
            {
                ReportError("You must assign a pool asset before calling End().");
                return false;
            }

            m_asset->SetReady();
            return EndCommon(result);
        }
        
        void AttachmentImageAssetCreator::SetAssetHint(AZStd::string_view hint)
        {
            m_asset.SetHint(hint);
        }
    }
}
