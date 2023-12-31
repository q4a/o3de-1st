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

#include <Atom/RHI/ScopeProducer.h>
#include <Atom/RPI.Public/Pass/RenderPass.h>
#include <Atom/RPI.Public/Buffer/Buffer.h>
#include <Atom/RHI/RayTracingBufferPools.h>
#include <Atom/RHI/RayTracingPipelineState.h>
#include <Atom/RHI/RayTracingShaderTable.h>
#include <Atom/RPI.Public/RPIUtils.h>
#include <Atom/RPI.Public/Shader/ShaderResourceGroup.h>
#include <DiffuseProbeGrid/DiffuseProbeGrid.h>

namespace AZ
{
    namespace Render
    {
        //! Ray tracing shader that generates probe radiance values.
        class DiffuseProbeGridRayTracingPass final
            : public RPI::RenderPass
        {
        public:
            AZ_RPI_PASS(DiffuseProbeGridRayTracingPass);

            AZ_RTTI(DiffuseProbeGridRayTracingPass, "{CB0DF817-3D07-4AC7-8574-F5EE529B8DCA}", RPI::RenderPass);
            AZ_CLASS_ALLOCATOR(DiffuseProbeGridRayTracingPass, SystemAllocator, 0);

            virtual ~DiffuseProbeGridRayTracingPass() override;

            //! Creates a DiffuseProbeGridRayTracingPass
            static RPI::Ptr<DiffuseProbeGridRayTracingPass> Create(const RPI::PassDescriptor& descriptor);

        private:
            explicit DiffuseProbeGridRayTracingPass(const RPI::PassDescriptor& descriptor);

            void CreateRayTracingPipelineState();
            void CreateShaderTableScope();

            // Scope producer functions
            void SetupFrameGraphDependencies(RHI::FrameGraphInterface frameGraph, const RPI::PassScopeProducer& producer) override;
            void CompileResources(const RHI::FrameGraphCompileContext& context, const RPI::PassScopeProducer& producer) override;
            void BuildCommandList(const RHI::FrameGraphExecuteContext& context, const RPI::PassScopeProducer& producer) override;

            // Pass overrides
            void FrameBeginInternal(FramePrepareParams params) override;

            // revision number of the ray tracing TLAS when the shader table was built
            uint32_t m_rayTracingRevision = 0;

            // ray tracing shader and pipeline state
            Data::Instance<RPI::Shader> m_rayTracingShader;
            RHI::Ptr<RHI::RayTracingPipelineState> m_rayTracingPipelineState;

            // ray tracing shader table
            RHI::Ptr<RHI::RayTracingShaderTable> m_rayTracingShaderTable;
            RHI::ScopeProducer* m_rayTracingScopeProducerShaderTable = nullptr;

            // ray tracing global shader resource group asset and pipeline state
            Data::Asset<RPI::ShaderResourceGroupAsset> m_globalSrgAsset;
            RHI::ConstPtr<RHI::PipelineState> m_globalPipelineState;

            // local SRGs associated with each mesh for the hit shaders
            AZStd::vector<AZ::Data::Instance<RPI::ShaderResourceGroup>> m_localSrgs;

            bool m_initialized = false;
        };
    }   // namespace RPI
}   // namespace AZ
