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

#include <Atom/RPI.Reflect/Model/ModelLodAsset.h>
#include <Atom/RPI.Public/Model/Model.h>
#include <Atom/RPI.Public/Shader/Shader.h>
#include <AtomCore/Instance/InstanceData.h>
#include <AzCore/std/smart_ptr/intrusive_base.h>
#include <AzCore/Interface/Interface.h>

namespace AZ
{
    namespace Render
    {
        //! Represents unskinned vertex streams from a source model that can be shared between each skinned mesh instance
        //! that are used as input to the skinning shader
        enum class SkinnedMeshInputVertexStreams : uint8_t
        {
            Position,
            Normal,
            Tangent,
            BiTangent,
            BlendIndices,
            BlendWeights,
            NumVertexStreams
        };

        //! Represents skinned vertex streams that are written to by the skinning shader
        enum class SkinnedMeshOutputVertexStreams : uint8_t
        {
            Position,
            Normal,
            Tangent,
            BiTangent,
            NumVertexStreams
        };

        //! Represents unskinned vertex streams from a source model that can be shared between each skinned mesh instance
        //! but are only used as input to the static mesh pipeline, not as input for the skinning shader
        enum class SkinnedMeshStaticVertexStreams : uint8_t
        {
            UV_0,
            NumVertexStreams
        };

        struct SkinnedMeshVertexStreamInfo
        {
            //! The format used for the buffer
            RHI::Format m_elementFormat;
            //! The size in bytes of each element in the stream
            uint32_t m_elementSize;
            //! The name used for the buffer view
            Name m_bufferName;
            //! The name used to get the ShaderInputBufferIndex for a specific shader variation of the skinning shader (not used by output streams)
            Name m_shaderResourceGroupName;
            //! The shader semantic used for the target skinned model (not used by streams that are input-only)
            RHI::ShaderSemantic m_semantic;
        };

        struct SkinnedMeshOutputVertexStreamInfo
        {
            //! The format used for the buffer
            RHI::Format m_elementFormat;
            //! The size in bytes of each element in the stream
            uint32_t m_elementSize;
            //! The name used for the buffer view
            Name m_bufferName;
            //! The name used to get the ShaderInputBufferIndex for a specific shader variation of the skinning shader
            Name m_shaderResourceGroupName;
            //! The shader semantic used for the target skinned model
            RHI::ShaderSemantic m_semantic;
            //! The input stream that corresponds with this output stream
            SkinnedMeshInputVertexStreams m_correspondingInputVertexStream;
        };

        //! Interface for getting properties associated with a specific type of skinned mesh vertex stream
        class SkinnedMeshVertexStreamPropertyInterface
        {
        public:
            AZ_RTTI(AZ::Render::SkinnedMeshVertexStreamPropertyInterface, "{9F80B8DE-96C4-4199-92E4-D8E37E3A3195}");

            SkinnedMeshVertexStreamPropertyInterface()
            {
                Interface<SkinnedMeshVertexStreamPropertyInterface>::Register(this);
            }

            virtual ~SkinnedMeshVertexStreamPropertyInterface()
            {
                Interface<SkinnedMeshVertexStreamPropertyInterface>::Unregister(this);
            }

            static SkinnedMeshVertexStreamPropertyInterface* Get()
            {
                return Interface<SkinnedMeshVertexStreamPropertyInterface>::Get();
            }

            // Note that you have to delete these for safety reasons, you will trip a static_assert if you do not
            AZ_DISABLE_COPY_MOVE(SkinnedMeshVertexStreamPropertyInterface);
            virtual const SkinnedMeshVertexStreamInfo& GetInputStreamInfo(SkinnedMeshInputVertexStreams stream) const = 0;
            virtual const SkinnedMeshVertexStreamInfo& GetStaticStreamInfo(SkinnedMeshStaticVertexStreams stream) const = 0;
            virtual const SkinnedMeshOutputVertexStreamInfo& GetOutputStreamInfo(SkinnedMeshOutputVertexStreams stream) const = 0;

            virtual Data::Asset<RPI::ResourcePoolAsset> GetInputStreamResourcePool() const = 0;
            virtual Data::Asset<RPI::ResourcePoolAsset> GetStaticStreamResourcePool() const = 0;
            virtual Data::Asset<RPI::ResourcePoolAsset> GetOutputStreamResourcePool() const = 0;
            
            virtual uint32_t GetMaxSupportedVertexCount() const = 0;
        };
    }
}
