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

#include "FrameCaptureSystemComponent.h"

#include <Atom/RPI.Public/Pass/PassSystemInterface.h>
#include <Atom/RPI.Public/Pass/PassFilter.h>
#include <Atom/RPI.Public/Pass/RenderPass.h>
#include <Atom/RPI.Public/Pass/Specific/SwapChainPass.h>

#include <Atom/Utils/DdsFile.h>
#include <Atom/Utils/PpmFile.h>

#include <AtomCore/Serialization/Json/JsonUtils.h>

#include <AzCore/IO/SystemFile.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Script/ScriptContextAttributes.h>
#include <AzCore/Serialization/SerializeContext.h>

#include <AzFramework/IO/LocalFileIO.h>
#include <AzFramework/StringFunc/StringFunc.h>

#include <AzCore/Preprocessor/EnumReflectUtils.h>

namespace AZ
{
    namespace Render
    {
        AZ_ENUM_DEFINE_REFLECT_UTILITIES(FrameCaptureResult);

        class FrameCaptureNotificationBusHandler final
            : public FrameCaptureNotificationBus::Handler
            , public AZ::BehaviorEBusHandler
        {
        public:
            AZ_EBUS_BEHAVIOR_BINDER(FrameCaptureNotificationBusHandler, "{68D1D94C-7055-4D32-8E22-BEEEBA0940C4}", AZ::SystemAllocator, OnCaptureFinished);

            void OnCaptureFinished(FrameCaptureResult result, const AZStd::string& info) override
            {
                Call(FN_OnCaptureFinished, result, info);
            }

            static void Reflect(AZ::ReflectContext* context)
            {
                if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
                {
                    FrameCaptureResultReflect(*serializeContext);
                }

                if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
                {
                    //[GFX_TODO][ATOM-13424] Replace this with a utility in AZ_ENUM_DEFINE_REFLECT_UTILITIES
                    behaviorContext->EnumProperty<static_cast<int>(FrameCaptureResult::None)>("FrameCaptureResult_None")
                        ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                        ->Attribute(AZ::Script::Attributes::Module, "atom");
                    behaviorContext->EnumProperty<static_cast<int>(FrameCaptureResult::Success)>("FrameCaptureResult_Success")
                        ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                        ->Attribute(AZ::Script::Attributes::Module, "atom");
                    behaviorContext->EnumProperty<static_cast<int>(FrameCaptureResult::FileWriteError)>("FrameCaptureResult_FileWriteError")
                        ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                        ->Attribute(AZ::Script::Attributes::Module, "atom");
                    behaviorContext->EnumProperty<static_cast<int>(FrameCaptureResult::InvalidArgument)>("FrameCaptureResult_InvalidArgument")
                        ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                        ->Attribute(AZ::Script::Attributes::Module, "atom");
                    behaviorContext->EnumProperty<static_cast<int>(FrameCaptureResult::UnsupportedFormat)>("FrameCaptureResult_UnsupportedFormat")
                        ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                        ->Attribute(AZ::Script::Attributes::Module, "atom");
                    behaviorContext->EnumProperty<static_cast<int>(FrameCaptureResult::InternalError)>("FrameCaptureResult_InternalError")
                        ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                        ->Attribute(AZ::Script::Attributes::Module, "atom");

                    behaviorContext->EBus<FrameCaptureNotificationBus>("FrameCaptureNotificationBus")
                        ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                        ->Attribute(AZ::Script::Attributes::Module, "atom")
                        ->Handler<FrameCaptureNotificationBusHandler>()
                        ;
                }
            }
        };

        void FrameCaptureSystemComponent::Reflect(AZ::ReflectContext* context)
        {
            if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<FrameCaptureSystemComponent, AZ::Component>()
                    ->Version(1)
                    ;
            }

            if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
            {
                behaviorContext->EBus<FrameCaptureRequestBus>("FrameCaptureRequestBus")
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                    ->Attribute(AZ::Script::Attributes::Module, "atom")
                    ->Event("CaptureScreenshot", &FrameCaptureRequestBus::Events::CaptureScreenshot)
                    ->Event("CaptureScreenshotWithPreview", &FrameCaptureRequestBus::Events::CaptureScreenshotWithPreview)
                    ->Event("CapturePassAttachment", &FrameCaptureRequestBus::Events::CapturePassAttachment)
                    ;

                FrameCaptureNotificationBusHandler::Reflect(context);
            }
        }

        void FrameCaptureSystemComponent::Activate()
        {
            FrameCaptureRequestBus::Handler::BusConnect();
        }

        void FrameCaptureSystemComponent::InitReadback()
        {
            if (!m_readback)
            {
                m_readback = AZStd::make_shared<AZ::RPI::AttachmentReadback>(AZ::RHI::ScopeId{ "FrameCapture" });
            }
            m_readback->SetCallback(AZStd::bind(&FrameCaptureSystemComponent::CaptureAttachmentCallback, this, AZStd::placeholders::_1));
        }

        void FrameCaptureSystemComponent::Deactivate()
        {
            m_readback = nullptr;
            FrameCaptureRequestBus::Handler::BusDisconnect();
        }

        AZStd::string FrameCaptureSystemComponent::ResolvePath(const AZStd::string& filePath)
        {
            AZ::IO::FileIOBase* fileIO = AZ::IO::FileIOBase::GetDirectInstance();
            char resolvedPath[AZ_MAX_PATH_LEN] = { 0 };
            fileIO->ResolvePath(filePath.c_str(), resolvedPath, AZ_MAX_PATH_LEN);

            return AZStd::string(resolvedPath);
        }

        bool FrameCaptureSystemComponent::CaptureScreenshotForWindow(const AZStd::string& filePath, AzFramework::NativeWindowHandle windowHandle)
        {
            InitReadback();

            if (m_state != State::Idle)
            {
                AZ_Warning("FrameCaptureSystemComponent", false, "Another capture has not finished yet");
                return false;
            }

            // Find SwapChainPass for the window handle
            RPI::SwapChainPass* pass = AZ::RPI::PassSystemInterface::Get()->FindSwapChainPass(windowHandle);
            if (!pass)
            {
                AZ_Warning("FrameCaptureSystemComponent", false, "Failed to find SwapChainPass for the window");
                return false;
            }

            if (!m_readback->IsReady())
            {
                AZ_Assert(false, "Failed to capture attachment since the readback is not ready");
                return false;
            }

            m_outputFilePath = ResolvePath(filePath);
            m_latestCaptureInfo.clear();
            m_state = State::Pending;
            m_result = FrameCaptureResult::None;
            TickBus::Handler::BusConnect();
            pass->ReadbackSwapChain(m_readback);

            return true;
        }

        bool FrameCaptureSystemComponent::CaptureScreenshot(const AZStd::string& filePath)
        {
            AzFramework::NativeWindowHandle windowHandle = nullptr;
            AzFramework::WindowSystemRequestBus::BroadcastResult(
                windowHandle,
                &AzFramework::WindowSystemRequestBus::Events::GetDefaultWindowHandle);

            if (windowHandle)
            {
                return CaptureScreenshotForWindow(filePath, windowHandle);
            }

            return false;
        }

        bool FrameCaptureSystemComponent::CaptureScreenshotWithPreview(const AZStd::string& outputFilePath)
        {
            InitReadback();

            if (m_state != State::Idle)
            {
                AZ_Warning("FrameCaptureSystemComponent", false, "Another capture has not finished yet");
                return false;
            }

            if (!m_readback->IsReady())
            {
                AZ_Assert(false, "Failed to capture attachment since the readback is not ready");
                return false;
            }

            m_outputFilePath.clear();
            if (!outputFilePath.empty())
            {
                m_outputFilePath = ResolvePath(outputFilePath);
            }
            m_latestCaptureInfo.clear();

            // Find the pass first
            RPI::PassClassFilter<RPI::ImageAttachmentPreviewPass> passFilter;
            AZStd::vector<AZ::RPI::Pass*> foundPasses = AZ::RPI::PassSystemInterface::Get()->FindPasses(passFilter);

            if (foundPasses.size() == 0)
            {
                AZ_Warning("FrameCaptureSystemComponent", false, "Failed to find an ImageAttachmentPreviewPass pass ");
                return false;
            }

            AZ::RPI::ImageAttachmentPreviewPass* previewPass = azrtti_cast<AZ::RPI::ImageAttachmentPreviewPass*>(foundPasses[0]);
            bool result = previewPass->ReadbackOutput(m_readback);
            if (result)
            {
                m_state = State::Pending;
                m_result = FrameCaptureResult::None;
                TickBus::Handler::BusConnect();
            }
            else
            {
                AZ_Warning("FrameCaptureSystemComponent", false, "CaptureScreenshotWithPreview. Failed to readback output from the ImageAttachmentPreviewPass");;
            }
            return result;
        }

        bool FrameCaptureSystemComponent::CapturePassAttachment(const AZStd::vector<AZStd::string>& passHierarchy, const AZStd::string& slot,
            const AZStd::string& outputFilePath)
        {
            InitReadback();

            if (m_state != State::Idle)
            {
                AZ_Warning("FrameCaptureSystemComponent", false, "Another capture has not finished yet");
                return false;
            }

            if (!m_readback->IsReady())
            {
                AZ_Assert(false, "Failed to capture attachment since the readback is not ready");
                return false;
            }

            m_outputFilePath.clear();
            if (!outputFilePath.empty())
            {
                m_outputFilePath = ResolvePath(outputFilePath);
            }
            m_latestCaptureInfo.clear();

            // Find the pass first
            AZ::RPI::PassHierarchyFilter passFilter(passHierarchy);
            AZStd::vector<AZ::RPI::Pass*> foundPasses = AZ::RPI::PassSystemInterface::Get()->FindPasses(passFilter);

            if (foundPasses.size() == 0)
            {
                AZ_Warning("FrameCaptureSystemComponent", false, "Failed to find pass from %s", passFilter.ToString().c_str());
                return false;
            }

            AZ::RPI::RenderPass* renderPass = azrtti_cast<AZ::RPI::RenderPass*>(foundPasses[0]);
            if (renderPass)
            {
                Name slotName = Name(slot);
                AZ::RPI::PassAttachment* attachment = nullptr;
                for (auto& binding : renderPass->GetAttachmentBindings())
                {
                    if (binding.m_name == slotName)
                    {
                        attachment = binding.m_attachment.get();
                        break;
                    }
                }
                if (attachment)
                {
                    m_state = State::Pending;
                    m_result = FrameCaptureResult::None;
                    TickBus::Handler::BusConnect();
                    renderPass->ReadbackAttachment(m_readback, attachment);
                }
                else
                {
                    AZ_Warning("FrameCaptureSystemComponent", false, "Failed to find attachment bound to pass [%s] slot [%s]",
                        renderPass->GetName().GetCStr(), slotName.GetCStr());
                    return false;
                }
            }
            return true;
        }

        bool FrameCaptureSystemComponent::CapturePassAttachmentWithCallback(const AZStd::vector<AZStd::string>& passHierarchy, const AZStd::string& slotName
            , RPI::AttachmentReadback::CallbackFunction callback)
        {
            bool result = CapturePassAttachment(passHierarchy, slotName, "");

            // Append state change to user provided call back
            AZ::RPI::AttachmentReadback::CallbackFunction callbackSetState = [&, callback](const AZ::RPI::AttachmentReadback::ReadbackResult& result)
            {
                callback(result);
                m_state = (result.m_state == AZ::RPI::AttachmentReadback::ReadbackState::Success) ? State::WasSuccess : State::WasFailure;
            };
            m_readback->SetCallback(callbackSetState);

            return result;
        }

        void FrameCaptureSystemComponent::OnTick([[maybe_unused]] float deltaTime, [[maybe_unused]] ScriptTimePoint time)
        {
            if (m_state == State::WasSuccess || m_state == State::WasFailure)
            {
                FrameCaptureNotificationBus::Broadcast(&FrameCaptureNotificationBus::Events::OnCaptureFinished, m_result, m_latestCaptureInfo.c_str());
                m_state = State::Idle;
                m_result = FrameCaptureResult::None;
                TickBus::Handler::BusDisconnect();
            }
            else if (m_state != State::Pending)
            {
                AZ_Assert(false, "TickBus should not be connected when a readback is not Pending. Something is out of sync");
            }
        }

        void FrameCaptureSystemComponent::CaptureAttachmentCallback(const AZ::RPI::AttachmentReadback::ReadbackResult& readbackResult)
        {
            AZ_Assert(m_state == State::Pending, "Unexpected value for m_state");
            AZ_Assert(m_result == FrameCaptureResult::None, "Unexpected value for m_result");

            m_latestCaptureInfo = m_outputFilePath;
            if (readbackResult.m_state == AZ::RPI::AttachmentReadback::ReadbackState::Success)
            {
                if (readbackResult.m_attachmentType == AZ::RHI::AttachmentType::Buffer)
                {
                    // write buffer data to the data file

                    AZ::IO::FileIOStream fileStream(m_outputFilePath.c_str(), AZ::IO::OpenMode::ModeWrite | AZ::IO::OpenMode::ModeCreatePath);
                    if (fileStream.IsOpen())
                    {
                        fileStream.Write(readbackResult.m_dataBuffer->size(), readbackResult.m_dataBuffer->data());
                        m_result = FrameCaptureResult::Success;
                    }
                    else
                    {
                        m_latestCaptureInfo = AZStd::string::format("Failed to open file %s for writing", m_outputFilePath.c_str());
                        m_result = FrameCaptureResult::FileWriteError;
                    }
                }
                else if (readbackResult.m_attachmentType == AZ::RHI::AttachmentType::Image)
                {
                    AZStd::string extension;
                    AzFramework::StringFunc::Path::GetExtension(m_outputFilePath.c_str(), extension, false);
                    AZStd::to_lower(extension.begin(), extension.end());

                    if (extension == "ppm")
                    {
                        if (readbackResult.m_imageDescriptor.m_format == RHI::Format::R8G8B8A8_UNORM || readbackResult.m_imageDescriptor.m_format == RHI::Format::B8G8R8A8_UNORM)
                        {
                            const AZStd::vector<uint8_t> outBuffer = Utils::PpmFile::CreatePpmFromImageBuffer(*readbackResult.m_dataBuffer.get(),
                                readbackResult.m_imageDescriptor.m_size, readbackResult.m_imageDescriptor.m_format);

                            IO::FileIOStream fileStream(m_outputFilePath.c_str(), IO::OpenMode::ModeWrite | IO::OpenMode::ModeCreatePath);

                            if (fileStream.IsOpen())
                            {
                                fileStream.Write(outBuffer.size(), outBuffer.data());
                                fileStream.Close();
                                m_result = FrameCaptureResult::Success;
                            }
                            else
                            {
                                m_latestCaptureInfo = AZStd::string::format("Failed to open file %s for writing", m_outputFilePath.c_str());
                                m_result = FrameCaptureResult::FileWriteError;
                            }
                        }
                        else
                        {
                            m_latestCaptureInfo = AZStd::string::format("Can't save image with format %s to a ppm file",
                                RHI::ToString(readbackResult.m_imageDescriptor.m_format));
                            m_result = FrameCaptureResult::UnsupportedFormat;
                        }
                    }
                    else if (extension == "dds")
                    {
                        // write the read back result of the image attachment to a dds file
                        auto outcome = AZ::DdsFile::WriteFile(m_outputFilePath, { readbackResult.m_imageDescriptor.m_size,
                            readbackResult.m_imageDescriptor.m_format, readbackResult.m_dataBuffer.get() });
                        if (outcome.IsSuccess())
                        {
                            m_result = FrameCaptureResult::Success;
                        }
                        else
                        {
                            m_latestCaptureInfo = outcome.GetError().m_message;
                            m_result = FrameCaptureResult::InternalError;
                        }
                    }
                    else
                    {
                        m_latestCaptureInfo = AZStd::string::format("Only supports saving image to ppm or dds files");
                        m_result = FrameCaptureResult::InvalidArgument;
                    }
                }
            }
            else
            {
                m_latestCaptureInfo = AZStd::string::format("Failed to read back attachment [%s]", readbackResult.m_name.GetCStr());
                m_result = FrameCaptureResult::InternalError;
            }


            if (m_result == FrameCaptureResult::Success)
            {
                m_state = State::WasSuccess;

                // Normalize the path so the slashes will be in the right direction for the local platform allowing easy copy/paste into file browsers.
                AZStd::string normalizedPath = m_outputFilePath;
                AzFramework::StringFunc::Path::Normalize(normalizedPath);
                AZ_Printf("FrameCaptureSystemComponent", "Attachment [%s] was saved to file %s\n", readbackResult.m_name.GetCStr(), normalizedPath.c_str());
            }
            else
            {
                m_state = State::WasFailure;
                AZ_Warning("FrameCaptureSystemComponent", false, "%s", m_latestCaptureInfo.c_str());
            }
        }
    }
}
