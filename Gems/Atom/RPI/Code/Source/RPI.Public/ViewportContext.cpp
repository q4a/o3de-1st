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

#include <Atom/RPI.Public/ViewportContext.h>
#include <Atom/RPI.Public/RenderPipeline.h>
#include <Atom/RPI.Public/ViewportContextBus.h>
#include <Atom/RPI.Public/ViewportContextManager.h>
#include <Atom/RPI.Public/View.h>

namespace AZ
{
    namespace RPI
    {
        ViewportContext::ViewportContext(ViewportContextManager* manager, AzFramework::ViewportId id, const AZ::Name& name, RHI::Device& device, AzFramework::NativeWindowHandle nativeWindow, ScenePtr renderScene)
            : m_rootScene(renderScene)
            , m_id(id)
            , m_windowContext(AZStd::make_shared<WindowContext>())
            , m_manager(manager)
            , m_name(name)
        {
            m_windowContext->Initialize(device, nativeWindow);
            AzFramework::WindowRequestBus::EventResult(
                m_viewportSize,
                nativeWindow,
                &AzFramework::WindowRequestBus::Events::GetClientAreaSize);
            AzFramework::WindowNotificationBus::Handler::BusConnect(nativeWindow);
        }

        ViewportContext::~ViewportContext()
        {
            m_manager->UnregisterViewportContext(m_id);
            m_windowContext->Shutdown();
        }

        AzFramework::ViewportId ViewportContext::GetId() const
        {
            return m_id;
        }

        AzFramework::NativeWindowHandle ViewportContext::GetWindowHandle() const
        {
            return m_windowContext->GetWindowHandle();
        }

        WindowContextSharedPtr ViewportContext::GetWindowContext()
        {
            return m_windowContext;
        }

        ScenePtr ViewportContext::GetRenderScene()
        {
            return m_rootScene;
        }

        void ViewportContext::SetRenderScene(ScenePtr scene)
        {
            if (m_rootScene != scene)
            {
                if (m_rootScene)
                {
                    SceneNotificationBus::Handler::BusDisconnect(m_rootScene->GetId());
                }
                else
                {
                    // If the scene was empty, we should save the default view from this scene as default view for the context.
                    auto renderPipeline = scene->FindRenderPipelineForWindow(m_windowContext->GetWindowHandle());
                    if (renderPipeline)
                    {
                        if (AZ::RPI::ViewPtr pipelineView = renderPipeline->GetDefaultView(); pipelineView)
                        {
                            SetDefaultView(pipelineView);
                        }
                    }
                }

                m_rootScene = scene;
                if (m_rootScene)
                {
                    SceneNotificationBus::Handler::BusConnect(m_rootScene->GetId());
                }
                m_currentPipeline.reset();
                UpdatePipelineView();
            }
        }

        void ViewportContext::RenderTick()
        {
            if (m_currentPipeline)
            {
                m_currentPipeline->AddToRenderTickOnce();
            }
        }

        AZ::Name ViewportContext::GetName() const
        {
            return m_name;
        }

        ViewPtr ViewportContext::GetDefaultView()
        {
            return m_defaultView;
        }

        ConstViewPtr ViewportContext::GetDefaultView() const
        {
            return m_defaultView;
        }

        AzFramework::WindowSize ViewportContext::GetViewportSize() const
        {
            return m_viewportSize;
        }

        void ViewportContext::ConnectSizeChangedHandler(SizeChangedEvent::Handler& handler)
        {
            handler.Connect(m_sizeChangedEvent);
        }

        const AZ::Matrix4x4& ViewportContext::GetCameraViewMatrix() const
        {
            return GetDefaultView()->GetWorldToViewMatrix();
        }

        void ViewportContext::SetCameraViewMatrix(const AZ::Matrix4x4& matrix)
        {
            GetDefaultView()->SetWorldToViewMatrix(matrix);
        }

        const AZ::Matrix4x4& ViewportContext::GetCameraProjectionMatrix() const
        {
            return GetDefaultView()->GetViewToClipMatrix();
        }

        void ViewportContext::SetCameraProjectionMatrix(const AZ::Matrix4x4& matrix)
        {
            GetDefaultView()->SetViewToClipMatrix(matrix);
        }

        AZ::Transform ViewportContext::GetCameraTransform() const
        {
            const Matrix4x4& worldToViewMatrix = GetDefaultView()->GetWorldToViewMatrix();
            return AZ::Transform::CreateFromQuaternionAndTranslation(
                Quaternion::CreateFromMatrix4x4(worldToViewMatrix),
                worldToViewMatrix.GetTranslation()
            );
        }

        void ViewportContext::SetCameraTransform(const AZ::Transform& transform)
        {
            GetDefaultView()->SetCameraTransform(AZ::Matrix3x4::CreateFromTransform(transform.GetOrthogonalized()));
        }

        void ViewportContext::SetDefaultView(ViewPtr view)
        {
            if (m_defaultView != view)
            {
                m_defaultView = view;
                UpdatePipelineView();
            }
        }

        void ViewportContext::UpdatePipelineView()
        {
            if (!m_defaultView || !m_rootScene)
            {
                return;
            }

            if (!m_currentPipeline)
            {
                m_currentPipeline = m_rootScene ? m_rootScene->FindRenderPipelineForWindow(m_windowContext->GetWindowHandle()) : nullptr;
            }

            if (auto pipeline = GetCurrentPipeline())
            {
                pipeline->SetDefaultView(m_defaultView);
            }
        }

        RenderPipelinePtr ViewportContext::GetCurrentPipeline()
        {
            return m_currentPipeline;
        }

        void ViewportContext::OnRenderPipelineAdded([[maybe_unused]]RenderPipelinePtr pipeline)
        {
            // If the pipeline is registered to our window, reset our current pipeline and do a lookup
            // Currently, Scene just stores pipelines sequentially in a vector, but we'll attempt to be safe
            // in the event prioritization is added later
            if (pipeline->GetWindowHandle() == m_windowContext->GetWindowHandle())
            {
                m_currentPipeline.reset();
                UpdatePipelineView();
            }
        }

        void ViewportContext::OnRenderPipelineRemoved([[maybe_unused]]RenderPipeline* pipeline)
        {
            if (m_currentPipeline.get() == pipeline)
            {
                m_currentPipeline.reset();
                UpdatePipelineView();
            }
        }

        void ViewportContext::OnWindowResized(uint32_t width, uint32_t height)
        {
            if (m_viewportSize.m_width != width || m_viewportSize.m_height != height)
            {
                m_viewportSize.m_width = width;
                m_viewportSize.m_height = height;
                m_sizeChangedEvent.Signal(m_viewportSize);
            }
        }
    } // namespace RPI
} // namespace AZ
