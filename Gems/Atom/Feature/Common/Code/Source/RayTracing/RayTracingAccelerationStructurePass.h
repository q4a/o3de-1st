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
#include <Atom/RPI.Public/Pass/Pass.h>
#include <Atom/RPI.Public/Buffer/Buffer.h>
#include <Atom/RHI/RayTracingBufferPools.h>

namespace AZ
{
    namespace Render
    {
        //! This pass builds the RayTracing acceleration structures for a scene
        class RayTracingAccelerationStructurePass final
            : public RPI::Pass
            , public RHI::ScopeProducer
        {
        public:
            AZ_RPI_PASS(RayTracingAccelerationStructurePass);

            AZ_RTTI(RayTracingAccelerationStructurePass, "{6BAA1755-D7D2-497F-BCDB-CA28B42728DC}", Pass);
            AZ_CLASS_ALLOCATOR(RayTracingAccelerationStructurePass, SystemAllocator, 0);

            //! Creates a RayTracingAccelerationStructurePass
            static RPI::Ptr<RayTracingAccelerationStructurePass> Create(const RPI::PassDescriptor& descriptor);

            ~RayTracingAccelerationStructurePass() = default;

        private:
            explicit RayTracingAccelerationStructurePass(const RPI::PassDescriptor& descriptor);

            // Scope producer functions
            void SetupFrameGraphDependencies(RHI::FrameGraphInterface frameGraph) override;
            void BuildCommandList(const RHI::FrameGraphExecuteContext& context) override;

            // Pass overrides
            void BuildAttachmentsInternal() override;
            void FrameBeginInternal(FramePrepareParams params) override;

            // buffer view descriptor for the TLAS
            RHI::BufferViewDescriptor m_tlasBufferViewDescriptor;

            // revision number of the ray tracing data when the TLAS was built
            uint32_t m_rayTracingRevision = 0;
        };
    }   // namespace RPI
}   // namespace AZ
