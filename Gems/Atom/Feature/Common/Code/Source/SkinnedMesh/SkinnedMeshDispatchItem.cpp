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

#include <SkinnedMesh/SkinnedMeshDispatchItem.h>
#include <SkinnedMesh/SkinnedMeshOutputStreamManager.h>
#include <SkinnedMesh/SkinnedMeshComputePass.h>

#include <Atom/RPI.Public/Shader/ShaderResourceGroup.h>
#include <Atom/RPI.Public/Shader/Shader.h>
#include <Atom/RPI.Public/Model/ModelLod.h>
#include <Atom/RPI.Public/Buffer/Buffer.h>

#include <Atom/RHI/Factory.h>
#include <Atom/RHI/BufferView.h>

#include <limits>

namespace AZ
{
    namespace Render
    {
        SkinnedMeshDispatchItem::SkinnedMeshDispatchItem(
            AZStd::intrusive_ptr<SkinnedMeshInputBuffers> inputBuffers,
            const AZStd::vector<uint32_t>& outputBufferOffsetsInBytes,
            size_t lodIndex,
            Data::Instance<RPI::Buffer> boneTransforms,
            const SkinnedMeshShaderOptions& shaderOptions,
            RPI::Ptr<SkinnedMeshComputePass> skinnedMeshComputePass,
            uint32_t morphTargetDeltaOffsetInBytes,
            float morphTargetDeltaIntegerEncoding)
            : m_inputBuffers(inputBuffers)
            , m_outputBufferOffsetsInBytes(outputBufferOffsetsInBytes)
            , m_lodIndex(lodIndex)
            , m_boneTransforms(AZStd::move(boneTransforms))
            , m_shaderOptions(shaderOptions)
            , m_morphTargetDeltaOffsetInBytes(morphTargetDeltaOffsetInBytes)
            , m_morphTargetDeltaIntegerEncoding(morphTargetDeltaIntegerEncoding)
        {
            m_skinningShader = skinnedMeshComputePass->GetShader();

            // Shader options are generally set per-skinned mesh instance, but morph targets may only exist on some lods. Override the option for applying morph targets here
            if (m_morphTargetDeltaOffsetInBytes != MorphTargetConstants::s_invalidDeltaOffset)
            {
                m_shaderOptions.m_applyMorphTargets = true;
            }

            // CreateShaderOptionGroup will also connect to the SkinnedMeshShaderOptionNotificationBus
            m_shaderOptionGroup = skinnedMeshComputePass->CreateShaderOptionGroup(m_shaderOptions, *this);
        }

        SkinnedMeshDispatchItem::~SkinnedMeshDispatchItem()
        {
            SkinnedMeshShaderOptionNotificationBus::Handler::BusDisconnect();
        }

        bool SkinnedMeshDispatchItem::Init()
        {
            if (!m_skinningShader)
            {
                AZ_Error("SkinnedMeshDispatchItem", false, "Cannot initialize a SkinnedMeshDispatchItem with a null shader");
                return false;
            }

            // Get the shader variant and instance SRG
            m_shaderOptionGroup.SetUnspecifiedToDefaultValues();
            RPI::ShaderVariantSearchResult shaderVariantSearchResult = m_skinningShader->FindVariantStableId(m_shaderOptionGroup.GetShaderVariantId());
            const RPI::ShaderVariant& shaderVariant = m_skinningShader->GetVariant(shaderVariantSearchResult.GetStableId());

            RHI::PipelineStateDescriptorForDispatch pipelineStateDescriptor;
            shaderVariant.ConfigurePipelineState(pipelineStateDescriptor);

            auto perInstanceSrgAsset = m_skinningShader->FindShaderResourceGroupAsset(AZ::Name{ "InstanceSrg" });
            if (!perInstanceSrgAsset.GetId().IsValid())
            {
                AZ_Error("SkinnedMeshDispatchItem", false, "Failed to get shader resource group asset");
                return false;
            }
            else if (!perInstanceSrgAsset.IsReady())
            {
                AZ_Error("SkinnedMeshDispatchItem", false, "Shader resource group asset is not loaded");
                return false;
            }

            m_instanceSrg = RPI::ShaderResourceGroup::Create(perInstanceSrgAsset);
            if (!m_instanceSrg)
            {
                AZ_Error("SkinnedMeshDispatchItem", false, "Failed to create shader resource group for skinned mesh");
                return false;
            }

            // If the shader variation is not fully baked, set the fallback key to use a runtime branch for the shader options
            if (!shaderVariantSearchResult.IsFullyBaked() && m_instanceSrg->HasShaderVariantKeyFallbackEntry())
            {
                m_instanceSrg->SetShaderVariantKeyFallbackValue(m_shaderOptionGroup.GetShaderVariantKeyFallbackValue());
            }

            m_inputBuffers->SetBufferViewsOnShaderResourceGroup(m_lodIndex, m_instanceSrg);

            // Set the SRG indices
            RHI::ShaderInputBufferIndex actorInstanceBoneTransformsIndex;
            if (m_shaderOptions.m_skinningMethod == SkinningMethod::LinearSkinning)
            {
                actorInstanceBoneTransformsIndex = m_instanceSrg->FindShaderInputBufferIndex(Name{ "m_boneTransformsLinear" });
                if (!actorInstanceBoneTransformsIndex.IsValid())
                {
                    AZ_Error("SkinnedMeshDispatchItem", false, "Failed to find shader input index for m_boneTransformsLinear in the skinning compute shader per-instance SRG.");
                    return false;
                }
            }
            else if(m_shaderOptions.m_skinningMethod == SkinningMethod::DualQuaternion)
            {
                actorInstanceBoneTransformsIndex = m_instanceSrg->FindShaderInputBufferIndex(Name{ "m_boneTransformsDualQuaternion" });
                if (!actorInstanceBoneTransformsIndex.IsValid())
                {
                    AZ_Error("SkinnedMeshDispatchItem", false, "Failed to find shader input index for m_boneTransformsDualQuaternion in the skinning compute shader per-instance SRG.");
                    return false;
                }
            }
            else
            {
                AZ_Assert(false, "Invalid skinning method for SkinnedMeshDispatchItem.");
            }

            AZ_Assert(aznumeric_cast<uint8_t>(m_outputBufferOffsetsInBytes.size()) == static_cast<uint8_t>(SkinnedMeshOutputVertexStreams::NumVertexStreams), "Not enough offsets were given to the SkinnedMeshDispatchItem");
            for (uint8_t outputStream = 0; outputStream < static_cast<uint8_t>(SkinnedMeshOutputVertexStreams::NumVertexStreams); outputStream++)
            {
                // Set the buffer offsets
                const SkinnedMeshOutputVertexStreamInfo& outputStreamInfo = SkinnedMeshVertexStreamPropertyInterface::Get()->GetOutputStreamInfo(static_cast<SkinnedMeshOutputVertexStreams>(outputStream));
                {
                    RHI::ShaderInputConstantIndex outputOffsetIndex = m_instanceSrg->FindShaderInputConstantIndex(outputStreamInfo.m_shaderResourceGroupName);
                    if (!outputOffsetIndex.IsValid())
                    {
                        AZ_Error("SkinnedMeshDispatchItem", false, "Failed to find shader input index for %s in the skinning compute shader per-instance SRG.", outputStreamInfo.m_shaderResourceGroupName.GetCStr());
                        return false;
                    }

                    // The shader has a view of with 4 bytes per element
                    // Divide the byte offset here so it doesn't need to be done in the shader
                    m_instanceSrg->SetConstant(outputOffsetIndex, m_outputBufferOffsetsInBytes[outputStream] / 4);
                }
            }

            m_instanceSrg->SetBuffer(actorInstanceBoneTransformsIndex, m_boneTransforms);

            // Set the morph target related srg constants
            RHI::ShaderInputConstantIndex morphOffsetIndex = m_instanceSrg->FindShaderInputConstantIndex(Name{ "m_morphTargetDeltaOffset" });
            // The buffer is using 32-bit integers, so divide the offset by 4 here so it doesn't have to be done in the shader
            m_instanceSrg->SetConstant(morphOffsetIndex, m_morphTargetDeltaOffsetInBytes / 4);
            RHI::ShaderInputConstantIndex morphDeltaIntegerEncodingIndex = m_instanceSrg->FindShaderInputConstantIndex(Name{ "m_morphTargetDeltaIntegerEncoding" });
            m_instanceSrg->SetConstant(morphDeltaIntegerEncodingIndex, m_morphTargetDeltaIntegerEncoding);
            
            // Set the vertex count
            const uint32_t vertexCount = m_inputBuffers->GetVertexCount(m_lodIndex);

            RHI::ShaderInputConstantIndex numVerticesIndex;
            numVerticesIndex = m_instanceSrg->FindShaderInputConstantIndex(Name{ "m_numVertices" });
            AZ_Error("SkinnedMeshInputBuffers", numVerticesIndex.IsValid(), "Failed to find shader input index for m_numVerticies in the skinning compute shader per-instance SRG.");
            m_instanceSrg->SetConstant(numVerticesIndex, vertexCount);
            
            uint32_t xThreads = 0;
            uint32_t yThreads = 0;
            CalculateSkinnedMeshTotalThreadsPerDimension(vertexCount, xThreads, yThreads);

            // Set the total number of threads in the x dimension, so the shader can calculate the vertex index from the thread ids
            RHI::ShaderInputConstantIndex totalNumberOfThreadsXIndex;
            totalNumberOfThreadsXIndex = m_instanceSrg->FindShaderInputConstantIndex(Name{ "m_totalNumberOfThreadsX" });
            AZ_Error("SkinnedMeshInputBuffers", totalNumberOfThreadsXIndex.IsValid(), "Failed to find shader input index for m_totalNumberOfThreadsX in the skinning compute shader per-instance SRG.");
            m_instanceSrg->SetConstant(totalNumberOfThreadsXIndex, xThreads);

            m_instanceSrg->Compile();
            m_dispatchItem.m_uniqueShaderResourceGroup = m_instanceSrg->GetRHIShaderResourceGroup();
            m_dispatchItem.m_pipelineState = m_skinningShader->AcquirePipelineState(pipelineStateDescriptor);
            
            const auto& numThreads = m_skinningShader->GetAsset()->GetAttribute(RHI::ShaderStage::Compute, AZ::Name{ "numthreads" });
            auto& arguments = m_dispatchItem.m_arguments.m_direct;
            if (numThreads)
            {
                const auto& args = *numThreads;
                arguments.m_threadsPerGroupX = args[0].type() == azrtti_typeid<int>() ? AZStd::any_cast<int>(args[0]) : 1;
                arguments.m_threadsPerGroupY = args[1].type() == azrtti_typeid<int>() ? AZStd::any_cast<int>(args[1]) : 1;
                arguments.m_threadsPerGroupZ = args[2].type() == azrtti_typeid<int>() ? AZStd::any_cast<int>(args[2]) : 1;
            }

            arguments.m_totalNumberOfThreadsX = xThreads;
            arguments.m_totalNumberOfThreadsY = yThreads;
            arguments.m_totalNumberOfThreadsZ = 1;

            return true;
        }

        const RHI::DispatchItem& SkinnedMeshDispatchItem::GetRHIDispatchItem() const
        {
            return m_dispatchItem;
        }

        Data::Instance<RPI::Buffer> SkinnedMeshDispatchItem::GetBoneTransforms() const
        {
            return m_boneTransforms;
        }

        AZStd::array_view<AZ::RHI::Ptr<RHI::BufferView>> SkinnedMeshDispatchItem::GetSourceUnskinnedBufferViews() const
        {
            return m_inputBuffers->GetInputBufferViews(m_lodIndex);
        }

        AZStd::array_view<AZ::RHI::Ptr<RHI::BufferView>> SkinnedMeshDispatchItem::GetTargetSkinnedBufferViews() const
        {
            return m_actorInstanceBufferViews;
        }

        size_t SkinnedMeshDispatchItem::GetVertexCount() const
        {
            return aznumeric_cast<size_t>(m_inputBuffers->GetVertexCount(m_lodIndex));
        }

        void SkinnedMeshDispatchItem::OnShaderReinitialized(const CachedSkinnedMeshShaderOptions* cachedShaderOptions)
        {
            m_shaderOptionGroup = cachedShaderOptions->CreateShaderOptionGroup(m_shaderOptions);

            if (!Init())
            {
                AZ_Error("SkinnedMeshDispatchItem", false, "Failed to re-initialize after the shader was re-loaded.");
            }
        }

        void CalculateSkinnedMeshTotalThreadsPerDimension(uint32_t vertexCount, uint32_t& xThreads, uint32_t& yThreads)
        {
            const uint32_t maxVerticesPerDimension = static_cast<uint32_t>(std::numeric_limits<uint16_t>::max());
            if (vertexCount > maxVerticesPerDimension * maxVerticesPerDimension)
            {
                AZ_Error("CalculateSkinnedMeshTotalThreadsPerDimension", false, "Vertex count '%d' exceeds maximum supported vertices '%d' for skinned meshes. Not all vertices will be rendered.", vertexCount, maxVerticesPerDimension * maxVerticesPerDimension);
                xThreads = maxVerticesPerDimension;
                yThreads = maxVerticesPerDimension;
                return;
            }
            else if (vertexCount == 0)
            {
                AZ_Error("CalculateSkinnedMeshTotalThreadsPerDimension", false, "Cannot skin mesh with 0 vertices.");
                xThreads = 0;
                yThreads = 0;
                return;
            }
            
            // Get the minimum number of threads in the y dimension needed to cover all the vertices in the mesh
            yThreads = vertexCount % maxVerticesPerDimension != 0 ? vertexCount / maxVerticesPerDimension + 1 : vertexCount / maxVerticesPerDimension;
            
            // Divide the total number of threads across y dimensions, rounding the number of xThreads up to cover any remainder
            xThreads = 1 + ((vertexCount - 1) / yThreads);
        }

    } // namespace Render
} // namespace AZ
