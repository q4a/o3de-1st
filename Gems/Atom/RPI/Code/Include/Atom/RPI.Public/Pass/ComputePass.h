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

#include <AzCore/Memory/SystemAllocator.h>

#include <Atom/RHI/DispatchItem.h>

#include <Atom/RPI.Public/Base.h>
#include <Atom/RPI.Public/Pass/RenderPass.h>
#include <Atom/RPI.Public/Shader/Shader.h>
#include <Atom/RPI.Public/Shader/ShaderReloadNotificationBus.h>

namespace AZ
{
    namespace RPI
    {
        class ShaderResourceGroup;

        //! A ComputePass is a leaf pass (pass with no children) that is used for GPU compute
        class ComputePass
            : public RenderPass
            , private ShaderReloadNotificationBus::Handler
        {
            AZ_RPI_PASS(ComputePass);

        public:
            AZ_RTTI(ComputePass, "{61464A74-BD35-4954-AB27-492644EA6C2A}", RenderPass);
            AZ_CLASS_ALLOCATOR(ComputePass, SystemAllocator, 0);
            virtual ~ComputePass();

            //! Creates a ComputePass
            static Ptr<ComputePass> Create(const PassDescriptor& descriptor);

            //! Sets the target total number of threads to dispatch in each dimension
            void SetTargetThreadCounts(uint32_t targetThreadCountX, uint32_t targetThreadCountY, uint32_t targetThreadCountZ);

            //! Returns the shader resource group.
            Data::Instance<ShaderResourceGroup> GetShaderResourceGroup();

        protected:
            ComputePass(const PassDescriptor& descriptor);

            // Pass behavior overrides...
            void FrameBeginInternal(FramePrepareParams params) override;

            // Scope producer functions...
            virtual void SetupFrameGraphDependencies(RHI::FrameGraphInterface frameGraph, const PassScopeProducer& producer);
            virtual void CompileResources(const RHI::FrameGraphCompileContext& context, const PassScopeProducer& producer);
            virtual void BuildCommandList(const RHI::FrameGraphExecuteContext& context, const PassScopeProducer& producer);

            // Calculates the group counts for the dispatch item using the target image dimensions
            // and the number of threads per group (group size)
            void MatchDimensionsToOutput();

            // The compute shader that will be used by the pass
            Data::Instance<Shader> m_shader = nullptr;

            // Default draw SRG for using the shader option system's variant fallback key
            Data::Instance<RPI::ShaderResourceGroup> m_drawSrg = nullptr;

            // The draw item submitted by this pass
            RHI::DispatchItem m_dispatchItem;

            // Whether or not to make the pass a full-screen compute pass. If set to true, the dispatch group counts will
            // be automatically calculated from the size of the first output attachment and the group size dimensions.
            bool m_isFullscreenPass = false;

            // ShaderReloadNotificationBus::Handler overrides...
            void OnShaderReinitialized(const Shader& shader) override;
            void OnShaderAssetReinitialized(const Data::Asset<ShaderAsset>& shaderAsset) override;
            void OnShaderVariantReinitialized(const Shader& shader, const ShaderVariantId& shaderVariantId, ShaderVariantStableId shaderVariantStableId) override;

            void LoadShader();
            PassDescriptor m_passDescriptor;

        };
    }   // namespace RPI
}   // namespace AZ
