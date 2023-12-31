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

#pragma once

#include <Atom/RPI.Reflect/Image/StreamingImagePoolAsset.h>
#include <Atom/RPI.Reflect/AssetCreator.h>

namespace AZ
{
    namespace RPI
    {
        //! Constructs an instance of an StreamingImagePoolAsset.
        //! (Note this class generally follows the builder design pattern, but is called a "creator" rather 
        //! than a "builder" to avoid confusion with the AssetBuilderSDK).
        class StreamingImagePoolAssetCreator
            : public AssetCreator<StreamingImagePoolAsset>
        {
        public:
            StreamingImagePoolAssetCreator() = default;

            //! Begins construction of a new streaming image pool asset instance. Resets the builder to a fresh state.
            //! assetId the unique id to use when creating the asset.
            void Begin(const Data::AssetId& assetId);

            //! Assigns the descriptor used to initialize the RHI streaming image pool.
            void SetPoolDescriptor(AZStd::unique_ptr<RHI::StreamingImagePoolDescriptor>&& descriptor);

            //! Assigns the controller asset which will perform streaming priority management on the pool.
            void SetControllerAsset(const Data::Asset<StreamingImageControllerAsset>& controllerAsset);

            //! Assigns the name of the pool
            void SetPoolName(AZStd::string_view poolName);
            
            //! Finalizes and assigns ownership of the asset to result, if successful. 
            //! Otherwise false is returned and result is left untouched.
            bool End(Data::Asset<StreamingImagePoolAsset>& result);
        };
    }
}
