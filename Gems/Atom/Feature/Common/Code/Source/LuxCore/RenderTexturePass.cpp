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

#include <Atom/Feature/LuxCore/RenderTexturePass.h>
#include <Atom/Feature/LuxCore/LuxCoreBus.h>

namespace AZ
{
    namespace Render
    {
        RPI::Ptr<RenderTexturePass> RenderTexturePass::Create(const RPI::PassDescriptor& descriptor)
        {
            RPI::Ptr<RenderTexturePass> pass = aznew RenderTexturePass(descriptor);
            return pass;
        }

        RenderTexturePass::RenderTexturePass(const RPI::PassDescriptor& descriptor)
            : FullscreenTrianglePass(descriptor)
        {
            m_textureIndex = m_shaderResourceGroup->FindShaderInputImageIndex(Name("m_sourceTexture"));
        }

        RenderTexturePass::~RenderTexturePass()
        {
        }

        void RenderTexturePass::SetPassSrgImage(AZ::Data::Instance<AZ::RPI::Image> image, RHI::Format format)
        {
            m_attachmentSize = image->GetRHIImage()->GetDescriptor().m_size;
            m_attachmentFormat = format;
            m_shaderResourceGroup->SetImage(m_textureIndex, image);
            QueueForBuildAttachments();
        }

        void RenderTexturePass::BuildAttachmentsInternal()
        {
            UpdataAttachment();
            FullscreenTrianglePass::BuildAttachmentsInternal();
        }

        void RenderTexturePass::FrameBeginInternal(FramePrepareParams params)
        {
            FullscreenTrianglePass::FrameBeginInternal(params);
        }

        void RenderTexturePass::UpdataAttachment()
        {
            // [GFX TODO][ATOM-2470] stop caring about attachment
            RPI::Ptr<RPI::PassAttachment> attachment = m_ownedAttachments.front();
            if (!attachment)
            {
                AZ_Assert(false, "[RenderTexturePass %s] Cannot find any image attachment.", GetPathName().GetCStr());
                return;
            }
            AZ_Assert(attachment->m_descriptor.m_type == RHI::AttachmentType::Image, "[RenderTexturePass %s] requires an image attachment", GetPathName().GetCStr());

            RPI::PassAttachmentBinding& binding = GetOutputBinding(0);
            binding.m_attachment = attachment;

            RHI::ImageDescriptor& imageDescriptor = attachment->m_descriptor.m_image;
            imageDescriptor.m_size = m_attachmentSize;
            imageDescriptor.m_format = m_attachmentFormat;
        }

        RHI::AttachmentId RenderTexturePass::GetRenderTargetId()
        {
            return m_ownedAttachments.front()->GetAttachmentId();
        }

        void RenderTexturePass::InitShaderVariant(bool isNormal)
        {
            auto shaderOption = m_shader->CreateShaderOptionGroup();
            RPI::ShaderOptionValue isNormalOption{ isNormal };
            shaderOption.SetValue(AZ::Name("o_isNormal"), isNormalOption);

            RPI::ShaderVariantSearchResult result = m_shader->FindVariantStableId(shaderOption.GetShaderVariantId());
            m_shaderVariantStableId = result.GetStableId();

            if (!result.IsFullyBaked() && m_drawShaderResourceGroup->HasShaderVariantKeyFallbackEntry())
            {
                m_drawShaderResourceGroup->SetShaderVariantKeyFallbackValue(shaderOption.GetShaderVariantId().m_key);
            }
        }
    }
}
