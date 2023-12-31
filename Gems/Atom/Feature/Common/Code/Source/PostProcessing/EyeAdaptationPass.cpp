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

#include <PostProcessing/EyeAdaptationPass.h>

#include <Atom/RHI/Factory.h>
#include <Atom/RHI/FrameGraphAttachmentInterface.h>
#include <Atom/RHI/FrameGraphInterface.h>
#include <Atom/RHI/PipelineState.h>

#include <Atom/RPI.Public/Base.h>
#include <Atom/RPI.Public/Pass/PassUtils.h>
#include <Atom/RPI.Public/RenderPipeline.h>
#include <Atom/RHI/RHISystemInterface.h>
#include <Atom/RPI.Public/RPIUtils.h>
#include <Atom/RPI.Public/Scene.h>
#include <Atom/RPI.Public/View.h>
#include <Atom/RPI.Reflect/Pass/PassTemplate.h>
#include <Atom/RPI.Reflect/Shader/ShaderAsset.h>

#include <PostProcess/PostProcessFeatureProcessor.h>
#include <PostProcess/ExposureControl/ExposureControlSettings.h>

namespace AZ
{
    namespace Render
    {
        static const char* const EyeAdaptationBufferBaseName = "EyeAdaptationBuffer";

        RPI::Ptr<EyeAdaptationPass> EyeAdaptationPass::Create(const RPI::PassDescriptor& descriptor)
        {
            RPI::Ptr<EyeAdaptationPass> pass = aznew EyeAdaptationPass(descriptor);
            return pass;
        }

        EyeAdaptationPass::EyeAdaptationPass(const RPI::PassDescriptor& descriptor)
            : RPI::ComputePass(descriptor)
        {
        }
        
        void EyeAdaptationPass::InitBuffer()
        {
            AZStd::string bufferName = AZStd::string::format("%s_%p", EyeAdaptationBufferBaseName, this);

            ExposureCalculationData defaultData;
            RPI::CommonBufferDescriptor desc;
            desc.m_poolType = RPI::CommonBufferPoolType::ReadWrite;
            desc.m_bufferName = bufferName;
            desc.m_byteCount = sizeof(ExposureCalculationData);
            desc.m_elementSize = aznumeric_cast<uint32_t>(desc.m_byteCount);
            desc.m_bufferData = &defaultData;

            m_buffer = RPI::BufferSystemInterface::Get()->CreateBufferFromCommonPool(desc);
        }

        void EyeAdaptationPass::UpdateEnable()
        {
            if (m_pipeline == nullptr)
            {
                SetEnabled(false);
                return;
            }

            AZ_Assert(m_pipeline->GetScene(), "Scene shouldn't nullptr");

            UpdateInputBufferIndices();

            AZ::RPI::Scene* scene = GetScene();
            bool enabled = false;

            if (scene)
            {
                PostProcessFeatureProcessor* fp = scene->GetFeatureProcessor<PostProcessFeatureProcessor>();
                AZ::RPI::ViewPtr view = GetView();
                if (fp)
                {
                    PostProcessSettings* postProcessSettings = fp->GetLevelSettingsFromView(view);
                    if (postProcessSettings)
                    {
                        ExposureControlSettings* settings = postProcessSettings->GetExposureControlSettings();
                        if (settings)
                        {
                            enabled = true;
                        }
                    }
                }
            }

            const bool lastEnabled = IsEnabled();
            SetEnabled(enabled);

            if (IsEnabled() && !lastEnabled)
            {
                // Need rebuilt this pass's attachment as any connections. So queue parent pass.
                GetParent()->QueueForBuildAttachments();
            }
        }

        void EyeAdaptationPass::UpdateInputBufferIndices()
        {
            if (m_exposureControlBufferInputIndex.IsNull())
            {
                m_exposureControlBufferInputIndex = GetView()->GetShaderResourceGroup()->FindShaderInputBufferIndex(Name("m_exposureControl"));
            }
        }

        void EyeAdaptationPass::BuildAttachmentsInternal()
        {
            if (m_pipeline == nullptr)
            {
                return;
            }

            if (!m_buffer)
            {
                InitBuffer();
            }

            AttachBufferToSlot(EyeAdaptationDataInputOutputSlotName, m_buffer);
        }

        void EyeAdaptationPass::FrameBeginInternal(FramePrepareParams params)
        {
            AZ::RPI::ComputePass::FrameBeginInternal(params);

            AZ::RPI::Scene* scene = GetScene();
            if (scene)
            {
                PostProcessFeatureProcessor* fp = scene->GetFeatureProcessor<PostProcessFeatureProcessor>();
                if (fp)
                {
                    AZ::RPI::ViewPtr view = GetView();
                    PostProcessSettings* postProcessSettings = fp->GetLevelSettingsFromView(view);
                    if (postProcessSettings)
                    {
                        ExposureControlSettings* settings = postProcessSettings->GetExposureControlSettings();
                        if (settings)
                        {
                            settings->UpdateBuffer();
                            view->GetShaderResourceGroup()->SetBufferView(m_exposureControlBufferInputIndex, settings->GetBufferView());
                        }
                    }
                }
            }
        }
    }   // namespace Render
}   // namespace AZ
