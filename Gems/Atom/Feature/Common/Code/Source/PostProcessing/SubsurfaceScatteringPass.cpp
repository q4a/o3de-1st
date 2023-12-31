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

#include <PostProcessing/SubsurfaceScatteringPass.h>

#include <Atom/RHI/CommandList.h>
#include <Atom/RHI/Factory.h>
#include <Atom/RHI/FrameScheduler.h>

#include <Atom/RPI.Reflect/Pass/PassTemplate.h>

#include <Atom/RPI.Public/Pass/PassUtils.h>
#include <Atom/RPI.Public/Pass/PassAttachment.h>
#include <Atom/RPI.Public/RPIUtils.h>
#include <Atom/RPI.Public/RenderPipeline.h>
#include <Atom/RPI.Public/View.h>
#include <Atom/RPI.Public/RPISystemInterface.h>
#include <Atom/RPI.Public/Shader/ShaderResourceGroup.h>

namespace AZ
{
    namespace RPI
    {
        Ptr<SubsurfaceScatteringPass> SubsurfaceScatteringPass::Create(const PassDescriptor& descriptor)
        {
            Ptr<SubsurfaceScatteringPass> pass = aznew SubsurfaceScatteringPass(descriptor);
            return pass;
        }

        SubsurfaceScatteringPass::SubsurfaceScatteringPass(const PassDescriptor& descriptor)
            : ComputePass(descriptor)
        {
            bool succeed = false;

            succeed = FindShaderInputConstantIndexByName(m_screenSizeInputIndex, Name{ "m_screenSize" });

            if (!succeed)
            {
                AZ_Error("PassSystem", false, "[SubsurfaceScatteringPass '%s']: Initialization failed");
            }
        }

        bool SubsurfaceScatteringPass::FindShaderInputConstantIndexByName(AZ::RHI::ShaderInputConstantIndex& index, AZ::Name name)
        {
            index = m_shaderResourceGroup->FindShaderInputConstantIndex(name);

            if (index.IsValid())
            {
                return true;
            }
            else
            {
                AZ_Error("PassSystem", false, "[SubsurfaceScatteringPass '%s']: Failed to find shader input constant %s in per pass srg of compute shader.",
                    name.GetCStr());
                return false;
            }
        }

        void SubsurfaceScatteringPass::FrameBeginInternal(FramePrepareParams params)
        {
            RHI::Size targetImageSize;
            if (m_isFullscreenPass)
            {
                PassAttachment* outputAttachment = nullptr;

                if (GetOutputCount() > 0)
                {
                    outputAttachment = GetOutputBinding(0).m_attachment.get();
                }
                else if (GetInputOutputCount() > 0)
                {
                    outputAttachment = GetInputOutputBinding(0).m_attachment.get();
                }

                AZ_Assert(outputAttachment != nullptr, "[ComputePass '%s']: A fullscreen compute pass must have a valid output or input/output.",
                    GetPathName().GetCStr());

                AZ_Assert(outputAttachment->GetAttachmentType() == RHI::AttachmentType::Image,
                    "[ComputePass '%s']: The output of a fullscreen compute pass must be an image.",
                    GetPathName().GetCStr());

                targetImageSize = outputAttachment->m_descriptor.m_image.m_size;
                SetTargetThreadCounts(targetImageSize.m_width, targetImageSize.m_height, targetImageSize.m_depth);
            }

            // Update shader constant
            m_shaderResourceGroup->SetConstant(m_screenSizeInputIndex, AZ::Vector2(static_cast<float>(targetImageSize.m_width), static_cast<float>(targetImageSize.m_height)));

            RenderPass::FrameBeginInternal(params);
        }
    }   // namespace RPI
}   // namespace AZ
