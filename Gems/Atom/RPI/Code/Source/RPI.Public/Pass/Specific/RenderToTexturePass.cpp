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

#include <Atom/RHI/FrameScheduler.h>

#include <Atom/RPI.Public/Pass/PassSystemInterface.h>
#include <Atom/RPI.Public/Pass/PassUtils.h>
#include <Atom/RPI.Public/Pass/Specific/RenderToTexturePass.h>


namespace AZ
{
    namespace RPI
    {
        Ptr<RenderToTexturePass> RenderToTexturePass::Create(const PassDescriptor& descriptor)
        {
            Ptr<RenderToTexturePass> pass = aznew RenderToTexturePass(descriptor);
            return pass;
        }

        Ptr<ParentPass> RenderToTexturePass::Recreate() const
        {
            PassDescriptor desc = GetPassDescriptor();
            Ptr<ParentPass> pass = aznew RenderToTexturePass(desc);
            return pass;
        }

        RenderToTexturePass::RenderToTexturePass(const PassDescriptor& descriptor)
            : ParentPass(descriptor)
        {
            // Save the pass data for easier access
            const RPI::RenderToTexturePassData* passData = RPI::PassUtils::GetPassData<RPI::RenderToTexturePassData>(descriptor);
            if (passData)
            {
                m_passData = *passData;
                OnUpdateOutputSize();
            }
        }

        RenderToTexturePass::~RenderToTexturePass()
        {
        }
        
        void RenderToTexturePass::BuildAttachmentsInternal()
        {
            m_outputAttachment = aznew PassAttachment();
            m_outputAttachment->m_name = "RenderTarget";
            m_outputAttachment->ComputePathName(GetPathName());

            RHI::ImageDescriptor outputImageDesc;
            outputImageDesc.m_bindFlags = RHI::ImageBindFlags::Color | RHI::ImageBindFlags::ShaderRead | RHI::ImageBindFlags::CopyWrite;
            outputImageDesc.m_size.m_width = m_passData.m_width;
            outputImageDesc.m_size.m_height = m_passData.m_height;
            outputImageDesc.m_format = m_passData.m_format;
            m_outputAttachment->m_descriptor = outputImageDesc;

            m_ownedAttachments.push_back(m_outputAttachment);

            PassAttachmentBinding outputBinding;
            outputBinding.m_name = "Output";
            outputBinding.m_slotType = PassSlotType::Output;
            outputBinding.m_attachment = m_outputAttachment;
            outputBinding.m_scopeAttachmentUsage = RHI::ScopeAttachmentUsage::RenderTarget;

            m_attachmentBindings.push_back(outputBinding);
            
            Base::BuildAttachmentsInternal();
        }

        void RenderToTexturePass::FrameBeginInternal(FramePrepareParams params)
        {
            params.m_scissorState = m_scissor;
            params.m_viewportState = m_viewport;

            Base::FrameBeginInternal(params);

            // for read back output
            if (m_readback)
            {
                m_readback->FrameBegin(params);
                if (m_readback->IsFinished())
                {
                    // Done reading. Remove the reference
                    m_readback = nullptr;
                }
            }
        }

        void RenderToTexturePass::ResizeOutput(uint32_t width, uint32_t height)
        {
            m_passData.m_width = width;
            m_passData.m_height = height;
            OnUpdateOutputSize();
            QueueForBuildAttachments();
        }

        void RenderToTexturePass::OnUpdateOutputSize()
        {
            // update scissor and viewport when output size changed
            m_scissor.m_minX = 0;
            m_scissor.m_maxX = m_passData.m_width;
            m_scissor.m_minY = 0;
            m_scissor.m_maxY = m_passData.m_height;
            
            m_viewport.m_minX = 0;
            m_viewport.m_maxX = aznumeric_cast<float>(m_passData.m_width);
            m_viewport.m_minX = 0;
            m_viewport.m_maxY = aznumeric_cast<float>(m_passData.m_height);
            m_viewport.m_minZ = 0;
            m_viewport.m_maxZ = 1;
        }

        void RenderToTexturePass::ReadbackOutput(AZStd::shared_ptr<AttachmentReadback> readback)
        {
            if (m_outputAttachment)
            {
                m_readback = readback;
                AZStd::string readbackName = AZStd::string::format("%s_%s", m_outputAttachment->GetAttachmentId().GetCStr(), GetName().GetCStr());
                m_readback->ReadPassAttachment(m_outputAttachment.get(), AZ::Name(readbackName));
            }
        }

    }   // namespace RPI
}   // namespace AZ
