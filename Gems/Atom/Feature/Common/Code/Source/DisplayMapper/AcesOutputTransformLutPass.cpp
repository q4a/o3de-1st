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

#include <Atom/Feature/DisplayMapper/AcesOutputTransformLutPass.h>
#include <Atom/Feature/ACES/AcesDisplayMapperFeatureProcessor.h>

#include <Atom/RHI/FrameGraphInterface.h>
#include <Atom/RHI/FrameGraphAttachmentInterface.h>

#include <Atom/RPI.Public/RenderPipeline.h>
#include <Atom/RPI.Public/Scene.h>
#include <Atom/RPI.Public/RPIUtils.h>
#include <Atom/RPI.Reflect/Pass/PassTemplate.h>
#include <Atom/RPI.Reflect/Shader/ShaderAsset.h>

namespace AZ
{
    namespace Render
    {
        RPI::Ptr<AcesOutputTransformLutPass> AcesOutputTransformLutPass::Create(const RPI::PassDescriptor& descriptor)
        {
            RPI::Ptr<AcesOutputTransformLutPass> pass = aznew AcesOutputTransformLutPass(descriptor);
            return pass;
        }

        AcesOutputTransformLutPass::AcesOutputTransformLutPass(const RPI::PassDescriptor& descriptor)
            : DisplayMapperFullScreenPass(descriptor)
        {
        }

        AcesOutputTransformLutPass::~AcesOutputTransformLutPass()
        {
            ReleaseLutImage();
        }

        void AcesOutputTransformLutPass::Init()
        {
            DisplayMapperFullScreenPass::Init();

            AZ_Assert(m_shaderResourceGroup != nullptr, "AcesOutputTransformLutPass %s has a null shader resource group when calling Init.", GetPathName().GetCStr());

            if (m_shaderResourceGroup != nullptr)
            {
                m_shaderInputLutImageIndex = m_shaderResourceGroup->FindShaderInputImageIndex(Name{ "m_lut" });
                m_shaderInputColorImageIndex = m_shaderResourceGroup->FindShaderInputImageIndex(Name{ "m_color" });
                m_shaderInputShaperBiasIndex = m_shaderResourceGroup->FindShaderInputConstantIndex(Name{ "m_shaperBias" });
                m_shaderInputShaperScaleIndex = m_shaderResourceGroup->FindShaderInputConstantIndex(Name{ "m_shaperScale" });
            }
        }

        void AcesOutputTransformLutPass::SetupFrameGraphDependencies(RHI::FrameGraphInterface frameGraph, [[maybe_unused]] const RPI::PassScopeProducer& producer)
        {
            DeclareAttachmentsToFrameGraph(frameGraph);
            DeclarePassDependenciesToFrameGraph(frameGraph);

            if (m_displayMapperLut.m_lutImage == nullptr)
            {
                AcquireLutImage();
            }

            AZ_Assert(m_displayMapperLut.m_lutImage != nullptr, "AcesOutputTransformLutPass unable to acquire LUT image");

            AZ::RHI::AttachmentId imageAttachmentId = AZ::RHI::AttachmentId("DisplayMapperLutImageAttachmentId");
            // Check and assert that the image attachment id exists (it should've been imported by the write lut pass)
            auto attachment = frameGraph.GetAttachmentDatabase().IsAttachmentValid(imageAttachmentId);
            AZ_Assert(attachment, "AcesOutputTransformLutPass invalid attachment \"DisplayMapperLutImageAttachmentId\"");

            RHI::ImageScopeAttachmentDescriptor desc;
            desc.m_attachmentId = imageAttachmentId;
            desc.m_imageViewDescriptor = m_displayMapperLut.m_lutImageViewDescriptor;
            desc.m_loadStoreAction.m_loadAction = AZ::RHI::AttachmentLoadAction::DontCare;

            frameGraph.UseShaderAttachment(desc, RHI::ScopeAttachmentAccess::Read);

            frameGraph.SetEstimatedItemCount(1);
        }

        void AcesOutputTransformLutPass::CompileResources(const RHI::FrameGraphCompileContext& context, [[maybe_unused]] const RPI::PassScopeProducer& producer)
        {
            AZ_Assert(m_shaderResourceGroup != nullptr, "AcesOutputTransformLutPass %s has a null shader resource group when calling FrameBeginInternal.", GetPathName().GetCStr());

            if (m_shaderResourceGroup != nullptr)
            {
                if (m_displayMapperLut.m_lutImageView != nullptr)
                {
                    m_shaderResourceGroup->SetImageView(m_shaderInputLutImageIndex, m_displayMapperLut.m_lutImageView.get());
                }

                m_shaderResourceGroup->SetConstant(m_shaderInputShaperBiasIndex, m_shaperParams.bias);
                m_shaderResourceGroup->SetConstant(m_shaderInputShaperScaleIndex, m_shaperParams.scale);
            }

            BindPassSrg(context, m_shaderResourceGroup);
            m_shaderResourceGroup->Compile();
        }

        void AcesOutputTransformLutPass::AcquireLutImage()
        {
            auto displayMapper = m_pipeline->GetScene()->GetFeatureProcessor<AZ::Render::AcesDisplayMapperFeatureProcessor>();
            displayMapper->GetDisplayMapperLut(m_displayMapperLut);
        }

        void AcesOutputTransformLutPass::ReleaseLutImage()
        {
            m_displayMapperLut.m_lutImage.reset();
            m_displayMapperLut.m_lutImageView.reset();
            m_displayMapperLut = {};
        }

        void AcesOutputTransformLutPass::SetShaperParams(const ShaperParams& shaperParams)
        {
            m_shaperParams = shaperParams;
        }
    }   // namespace Render
}   // namespace AZ
