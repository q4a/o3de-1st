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

#include <Atom/RPI.Public/Pass/FullscreenTrianglePass.h>
#include <Atom/RPI.Public/Shader/Shader.h>
#include <Atom/RPI.Public/Shader/ShaderResourceGroup.h>

#include <Atom/Feature/ACES/AcesDisplayMapperFeatureProcessor.h>
#include <Atom/Feature/DisplayMapper/DisplayMapperFullScreenPass.h>

namespace AZ
{
    namespace Render
    {
        /**
         *  The Aces output transform pass.
         *  This pass impelements the ACES color pipeline, which includes Reference Rendering Transform(RRT)
         *  and Output Device Transform(ODT).
         */
        class AcesOutputTransformPass final
            : public DisplayMapperFullScreenPass
        {
            AZ_RPI_PASS(AcesOutputTransformPass);

        public:
            AZ_RTTI(AcesOutputTransformPass, "{705F8A80-CAF2-4A9C-BF40-2141ABD70BDC}", DisplayMapperFullScreenPass);
            AZ_CLASS_ALLOCATOR(AcesOutputTransformPass, SystemAllocator, 0);

            //! Creates a DisplayMapperPass
            static RPI::Ptr<AcesOutputTransformPass> Create(const RPI::PassDescriptor& descriptor);

            void SetDisplayBufferFormat(RHI::Format format);

        private:
            explicit AcesOutputTransformPass(const RPI::PassDescriptor& descriptor);
            void Init() override;

            // Scope producer functions...
            void CompileResources(const RHI::FrameGraphCompileContext& context, const RPI::PassScopeProducer& producer) override;

            RHI::ShaderInputConstantIndex m_shaderInputColorMatIndex;
            RHI::ShaderInputConstantIndex m_shaderInputCinemaLimitsIndex;
            RHI::ShaderInputConstantIndex m_shaderInputAcesSplineParamsIndex;
            RHI::ShaderInputConstantIndex m_shaderInputFlagsIndex;
            RHI::ShaderInputConstantIndex m_shaderInputOutputModeIndex;
            RHI::ShaderInputConstantIndex m_shaderInputSurroundGammaIndex;
            RHI::ShaderInputConstantIndex m_shaderInputGammaIndex;

            AZ::Render::DisplayMapperParameters m_displayMapperParameters = {};

            RHI::Format m_displayBufferFormat = RHI::Format::Unknown;
        };
    }   // namespace Render
}   // namespace AZ
