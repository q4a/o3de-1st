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

#include <Atom/RHI/CommandList.h>
#include <Atom/RHI/DrawItem.h>
#include <Atom/RHI/ScopeProducer.h>
#include <Atom/RHI.Reflect/ShaderResourceGroupLayoutDescriptor.h>

#include <Atom/RPI.Public/Pass/ComputePass.h>
#include <Atom/RPI.Public/Shader/Shader.h>
#include <Atom/RPI.Public/Shader/ShaderResourceGroup.h>

namespace AZ
{
    namespace Render
    {
        static const char* const EyeAdaptationPassTemplateName = "EyeAdaptationTemplate";
        static const char* const EyeAdaptationDataInputOutputSlotName = "EyeAdaptationDataInputOutput";

        //! The eye adaptation pass.
        //! This pass apply auto exposure control by like eye adaptation for input framebuffer color.
        class EyeAdaptationPass final
            : public RPI::ComputePass
        {
            AZ_RPI_PASS(EyeAdaptationPass);

        public:
            AZ_RTTI(EyeAdaptationPass, "{CC66CFD9-3266-4FD7-A5A8-ACA3753BDF4A}", RPI::ComputePass);
            AZ_CLASS_ALLOCATOR(EyeAdaptationPass, SystemAllocator, 0);
            ~EyeAdaptationPass() = default;

            // Creates a EyeAdaptationPass
            static RPI::Ptr<EyeAdaptationPass> Create(const RPI::PassDescriptor& descriptor);

            // Check if we should enable of disable this pass
            void UpdateEnable();

        protected:
            EyeAdaptationPass(const RPI::PassDescriptor& descriptor);
            void InitBuffer();
            void UpdateInputBufferIndices();

            // A StructuredBuffer for exposure calculation on the GPU.
            struct ExposureCalculationData
            {
                float   m_exposureValue = 1.0f;
            };

            void BuildAttachmentsInternal() override;

            void FrameBeginInternal(FramePrepareParams params) override;

            AZ::Data::Instance<RPI::Buffer> m_buffer;

            // SRG binding indices...
            AZ::RHI::ShaderInputBufferIndex m_exposureControlBufferInputIndex;
        };
    }   // namespace Render
}   // namespace AZ
