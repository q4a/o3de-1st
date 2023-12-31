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

#include <Atom/Feature/SkinnedMesh/SkinnedMeshVertexStreams.h>
#include <Atom/RHI.Reflect/ShaderSemantic.h>
#include <Atom/RHI.Reflect/Format.h>
#include <AzCore/Name/Name.h>

namespace AZ
{

    namespace Render
    {
        class SkinnedMeshVertexStreamProperties
            : public SkinnedMeshVertexStreamPropertyInterface
        {
        public:
            AZ_RTTI(AZ::Render::SkinnedMeshVertexStreamProperties, "{8912239E-8412-4B9E-BDE6-AE6BA67A207C}", AZ::Render::SkinnedMeshVertexStreamPropertyInterface);

            SkinnedMeshVertexStreamProperties();

            const SkinnedMeshVertexStreamInfo& GetInputStreamInfo(SkinnedMeshInputVertexStreams stream) const override;
            const SkinnedMeshVertexStreamInfo& GetStaticStreamInfo(SkinnedMeshStaticVertexStreams stream) const override;
            const SkinnedMeshOutputVertexStreamInfo& GetOutputStreamInfo(SkinnedMeshOutputVertexStreams stream) const override;

            Data::Asset<RPI::ResourcePoolAsset> GetInputStreamResourcePool() const override;
            Data::Asset<RPI::ResourcePoolAsset> GetStaticStreamResourcePool() const override;
            Data::Asset<RPI::ResourcePoolAsset> GetOutputStreamResourcePool() const override;

            uint32_t GetMaxSupportedVertexCount() const override;
        private:
            AZStd::array<SkinnedMeshVertexStreamInfo, static_cast<uint8_t>(SkinnedMeshInputVertexStreams::NumVertexStreams)> m_inputStreamInfo;
            AZStd::array<SkinnedMeshVertexStreamInfo, static_cast<uint8_t>(SkinnedMeshStaticVertexStreams::NumVertexStreams)> m_staticStreamInfo;
            AZStd::array<SkinnedMeshOutputVertexStreamInfo, static_cast<uint8_t>(SkinnedMeshOutputVertexStreams::NumVertexStreams)> m_outputStreamInfo;
            
            Data::Asset<RPI::ResourcePoolAsset> m_inputStreamResourcePool;
            Data::Asset<RPI::ResourcePoolAsset> m_staticStreamResourcePool;
            Data::Asset<RPI::ResourcePoolAsset> m_outputStreamResourcePool;
        };
    } // namespace Render
} // namespace AZ
