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

#include <AtomToolsFramework/Viewport/RenderViewportWidget.h>
#include <Atom/RPI.Public/ViewportContext.h>
#include <Atom/RPI.Public/ViewportContextBus.h>
#include <Atom/RPI.Public/View.h>
#include <AzFramework/Input/Devices/Mouse/InputDeviceMouse.h>
#include <AzFramework/Viewport/ViewportControllerList.h>
#include <AzFramework/Viewport/ViewportScreen.h>
#include <AzCore/Math/MathUtils.h>
#include <Atom/RHI/RHISystemInterface.h>
#include <Atom/Bootstrap/BootstrapRequestBus.h>

#include <QCursor>
#include <QBoxLayout>
#include <QWindow>
#include <QMouseEvent>

namespace AtomToolsFramework
{
    RenderViewportWidget::RenderViewportWidget(AzFramework::ViewportId id, QWidget* parent)
        : QWidget(parent)
        , AzFramework::InputChannelEventListener(AzFramework::InputChannelEventListener::GetPriorityDefault())
    {
        auto viewportContextManager = AZ::Interface<AZ::RPI::ViewportContextRequestsInterface>::Get();
        AZ_Assert(viewportContextManager, "Attempted to construct RenderViewportWidget without ViewportContextManager");

        // Before we do anything else, we must create a ViewportContext which will give us a ViewportId if we didn't manually specify one.
        AZ::RPI::ViewportContextRequestsInterface::CreationParameters params;
        params.device = AZ::RHI::RHISystemInterface::Get()->GetDevice();
        params.windowHandle = reinterpret_cast<AzFramework::NativeWindowHandle>(winId());
        params.id = id;
        m_viewportContext = viewportContextManager->CreateViewportContext(AZ::Name(), params);

        SetControllerList(AZStd::make_shared<AzFramework::ViewportControllerList>());

        AZ::Name cameraName = AZ::Name(AZStd::string::format("Viewport %i Default Camera", m_viewportContext->GetId()));
        m_defaultCamera = AZ::RPI::View::CreateView(cameraName, AZ::RPI::View::UsageFlags::UsageCamera);
        AZ::Interface<AZ::RPI::ViewportContextRequestsInterface>::Get()->PushView(m_viewportContext->GetName(), m_defaultCamera);

        AzToolsFramework::ViewportInteraction::ViewportInteractionRequestBus::Handler::BusConnect(GetId());
        AzFramework::InputChannelEventListener::Connect();
        AZ::TickBus::Handler::BusConnect();

        setUpdatesEnabled(false);
        setFocusPolicy(Qt::FocusPolicy::WheelFocus);
        setMouseTracking(true);

        // Render at a fixed 60hz for now
        m_renderTimer.start();
        startTimer(1000 / 60, Qt::PreciseTimer);
    }

    void RenderViewportWidget::LockRenderTargetSize(uint32_t width, uint32_t height)
    {
        setFixedSize(aznumeric_cast<int>(width), aznumeric_cast<int>(height));
    }

    void RenderViewportWidget::UnlockRenderTargetSize()
    {
        setSizePolicy(QSizePolicy::Policy::Preferred, QSizePolicy::Policy::Preferred);
    }

    AZ::RPI::ViewportContextPtr RenderViewportWidget::GetViewportContext()
    {
        return m_viewportContext;
    }

    AZ::RPI::ConstViewportContextPtr RenderViewportWidget::GetViewportContext() const
    {
        return m_viewportContext;
    }

    void RenderViewportWidget::SetScene(AzFramework::Scene* scene, bool useDefaultRenderPipeline)
    {
        if (scene == nullptr)
        {
            m_viewportContext->SetRenderScene(nullptr);
            return;
        }
        AZ::RPI::ScenePtr atomScene;
        auto initializeScene = [&](AZ::Render::Bootstrap::Request* bootstrapRequests)
        {
            atomScene = bootstrapRequests->GetOrCreateAtomSceneFromAzScene(scene);
            if (useDefaultRenderPipeline)
            {
                // atomScene may already have a default render pipeline installed.
                // If so, this will be a no-op.
                bootstrapRequests->EnsureDefaultRenderPipelineInstalledForScene(atomScene, m_viewportContext);
            }
        };
        AZ::Render::Bootstrap::RequestBus::Broadcast(initializeScene);
        m_viewportContext->SetRenderScene(atomScene);
        if (auto auxGeomFP = atomScene->GetFeatureProcessor<AZ::RPI::AuxGeomFeatureProcessorInterface>())
        {
            m_auxGeom = auxGeomFP->GetOrCreateDrawQueueForView(m_defaultCamera.get());
        }
    }

    AZ::RPI::ViewPtr RenderViewportWidget::GetDefaultCamera()
    {
        return m_defaultCamera;
    }

    AZ::RPI::ConstViewPtr RenderViewportWidget::GetDefaultCamera() const
    {
        return m_defaultCamera;
    }

    static bool IsMouseButtonEvent(const AzFramework::InputChannel& inputChannel)
    {
        const auto& mouseButtons = AzFramework::InputDeviceMouse::Button::All;
        return AZStd::find(mouseButtons.begin(), mouseButtons.end(), inputChannel.GetInputChannelId()) != mouseButtons.end();
    }

    static bool IsMouseMoveEvent(const AzFramework::InputChannel& inputChannel)
    {
        return inputChannel.GetInputChannelId() == AzFramework::InputDeviceMouse::SystemCursorPosition;
    }

    bool RenderViewportWidget::CanInputGrantFocus(const AzFramework::InputChannel& inputChannel) const
    {
        // Only take focus from a mouse event if the cursor is currently within the viewport
        if (!m_mouseOver)
        {
            return false;
        }

        // Only mouse button down events (clicks) can grant focus
        if (inputChannel.GetState() != AzFramework::InputChannel::State::Began)
        {
            return false;
        }

        // Only mouse button events can grant focus
        return IsMouseButtonEvent(inputChannel);
    }

    bool RenderViewportWidget::OnInputChannelEventFiltered(const AzFramework::InputChannel& inputChannel)
    {
        // Grab keyboard focus if we've been clicked on.
        // Qt normally handles this for us, but we're filtering native events before they get
        // synthesized into QMouseEvents.
        if (!hasFocus() && CanInputGrantFocus(inputChannel))
        {
            setFocus();
        }

        // Don't consume new input events if we don't currently have focus.
        // We do forward Ended and Updated events, as they may be relevant to our current state
        // (e.g. a key gets released after we lose focus, it shouldn't remain "stuck").
        if (!hasFocus() && inputChannel.GetState() == AzFramework::InputChannel::State::Began)
        {
            return false;
        }

        // If we receive a mouse button event from outside of our viewport, ignore it even if we have focus.
        if (!m_mouseOver && inputChannel.GetState() == AzFramework::InputChannel::State::Began && IsMouseButtonEvent(inputChannel))
        {
            return false;
        }

        // Don't forward system cursor position updates, we'll do that ourselves for in-window movements once the result of
        // ViewportCursorScreenPosition is guaranteed to be correct (see mouseMoveEvent).
        if (IsMouseMoveEvent(inputChannel))
        {
            return false;
        }

        return m_controllerList->HandleInputChannelEvent(GetId(), inputChannel);
    }

    void RenderViewportWidget::OnTick([[maybe_unused]]float deltaTime, AZ::ScriptTimePoint time)
    {
        m_time = time;
    }

    void RenderViewportWidget::resizeEvent([[maybe_unused]] QResizeEvent* event)
    {
        // We need to wait until the window is activated, so the underlying surface
        // has been created and has the correct size.
        if (windowHandle()->isActive())
        {
            SendWindowResizeEvent();
        }
        else
        {
            m_windowResizedEvent = true;
        }
    }

    bool RenderViewportWidget::event(QEvent* event)
    {
        // Check if we have a pending resize event.
        // At this point the surface has been created and has
        // the proper dimensions.
        if (event->type() == QEvent::WindowActivate && m_windowResizedEvent)
        {
            SendWindowResizeEvent();
        }
        return QWidget::event(event);
    }

    void RenderViewportWidget::enterEvent([[maybe_unused]] QEvent* event)
    {
        m_mouseOver = true;
    }

    void RenderViewportWidget::leaveEvent([[maybe_unused]] QEvent* event)
    {
        m_mouseOver = false;
    }

    void RenderViewportWidget::timerEvent(QTimerEvent*)
    {
        m_controllerList->UpdateViewport(GetId(), AzFramework::FloatSeconds(m_renderTimer.restart() / 1000.f), m_time);
        m_viewportContext->RenderTick();
    }

    void RenderViewportWidget::mouseMoveEvent(QMouseEvent* event)
    {
        m_mousePosition = event->localPos();

        // Now that we've looked a viewport local mouse position,
        // we can go ahead and broadcast the system cursor input event to the controllers.
        // This allows any controllers not listening to pure mosue deltas to consistently
        // look up the mouse position in viewport screen coordinates.
        const AzFramework::InputDevice* mouseInputDevice = nullptr;
        if (AzFramework::InputDeviceRequestBus::EventResult(mouseInputDevice, AzFramework::InputDeviceMouse::Id, &AzFramework::InputDeviceRequests::GetInputDevice);
            mouseInputDevice != nullptr)
        {
            AzFramework::InputChannel syntheticInput(AzFramework::InputDeviceMouse::SystemCursorPosition, *mouseInputDevice);
            m_controllerList->HandleInputChannelEvent(GetId(), syntheticInput);
        }
    }

    void RenderViewportWidget::SendWindowResizeEvent()
    {
        // Scale the size by the DPI of the platform to
        // get the proper size in pixels.
        const QSize uiWindowSize = size();
        const qreal deficePixelRatio = devicePixelRatioF();
        const QSize windowSize = uiWindowSize * deficePixelRatio;

        AzFramework::NativeWindowHandle windowId = reinterpret_cast<AzFramework::NativeWindowHandle>(winId());
        AzFramework::WindowNotificationBus::Event(windowId, &AzFramework::WindowNotifications::OnWindowResized, windowSize.width(), windowSize.height());
        m_windowResizedEvent = false;
    }

    AZ::Name RenderViewportWidget::GetCurrentContextName() const
    {
        return m_viewportContext->GetName();
    }

    void RenderViewportWidget::SetCurrentContextName(const AZ::Name& contextName)
    {
        auto viewportContextManager = AZ::Interface<AZ::RPI::ViewportContextRequestsInterface>::Get();
        viewportContextManager->RenameViewportContext(m_viewportContext, contextName);
    }

    AzFramework::ViewportId RenderViewportWidget::GetId() const
    {
        return m_viewportContext->GetId();
    }

    AzFramework::ViewportControllerListPtr RenderViewportWidget::GetControllerList()
    {
        return m_controllerList;
    }

    AzFramework::ConstViewportControllerListPtr RenderViewportWidget::GetControllerList() const
    {
        return m_controllerList;
    }

    void RenderViewportWidget::SetControllerList(AzFramework::ViewportControllerListPtr controllerList)
    {
        if (m_controllerList)
        {
            m_controllerList->UnregisterViewportContext(GetId());
        }
        m_controllerList = controllerList;
        if (m_controllerList)
        {
            m_controllerList->RegisterViewportContext(GetId());
        }
    }

    AzFramework::CameraState RenderViewportWidget::GetCameraState()
    {
        AZ::RPI::ViewPtr currentView = m_viewportContext->GetDefaultView();
        if (currentView == nullptr)
        {
            return {};
        }

        // Build camera state from Atom camera transforms
        AzFramework::CameraState cameraState = AzFramework::CreateCameraFromWorldFromViewMatrix(
            currentView->GetViewToWorldMatrix(),
            AZ::Vector2{aznumeric_cast<float>(width()), aznumeric_cast<float>(height())}
        );
        AzFramework::SetCameraClippingVolumeFromPerspectiveFovMatrixRH(cameraState, currentView->GetViewToClipMatrix());

        // Convert from Z-up
        AZStd::swap(cameraState.m_forward, cameraState.m_up);
        cameraState.m_forward = -cameraState.m_forward;

        return cameraState;
    }

    bool RenderViewportWidget::GridSnappingEnabled()
    {
        return false;
    }

    float RenderViewportWidget::GridSize()
    {
        return 0.0f;
    }

    bool RenderViewportWidget::ShowGrid()
    {
        return false;
    }

    bool RenderViewportWidget::AngleSnappingEnabled()
    {
        return false;
    }

    float RenderViewportWidget::AngleStep()
    {
        return 0.0f;
    }

    QPoint RenderViewportWidget::ViewportWorldToScreen(const AZ::Vector3& worldPosition)
    {
        AZ::RPI::ViewPtr currentView = m_viewportContext->GetDefaultView();
        if (currentView == nullptr)
        {
            return QPoint();
        }
        AzFramework::ScreenPoint position = AzFramework::WorldToScreen(
            worldPosition,
            currentView->GetViewToWorldMatrix(),
            currentView->GetViewToClipMatrix(),
            AZ::Vector2{aznumeric_cast<float>(width()), aznumeric_cast<float>(height())}
        );
        return {position.m_x, position.m_y};
    }

    AZStd::optional<AZ::Vector3> RenderViewportWidget::ViewportScreenToWorld(const QPoint& screenPosition, float depth)
    {
        const auto& cameraProjection = m_viewportContext->GetCameraProjectionMatrix();
        const auto& cameraView = m_viewportContext->GetCameraViewMatrix();

        const AZ::Vector4 normalizedScreenPosition {
            screenPosition.x() * 2.f / width() - 1.0f,
            (height() - screenPosition.y()) * 2.f / height() - 1.0f,
            1.f - depth, // [GFX TODO] [ATOM-1501] Currently we always assume reverse depth
            1.f
        };
        AZ::Matrix4x4 worldFromScreen = cameraProjection * cameraView;
        worldFromScreen.InvertFull();

        AZ::Vector4 projectedPosition = worldFromScreen * normalizedScreenPosition;
        if (projectedPosition.GetW() == 0.f)
        {
            return {};
        }
        return projectedPosition.GetAsVector3() / projectedPosition.GetW();
    }

    AZStd::optional<AzToolsFramework::ViewportInteraction::ProjectedViewportRay> RenderViewportWidget::ViewportScreenToWorldRay(const QPoint& screenPosition)
    {
        auto pos0 = ViewportScreenToWorld(screenPosition, 0.f);
        auto pos1 = ViewportScreenToWorld(screenPosition, 1.f);
        if (!pos0.has_value() || !pos1.has_value())
        {
            return {};
        }

        pos0 = m_viewportContext->GetDefaultView()->GetViewToWorldMatrix().GetTranslation();
        AZ::Vector3 rayOrigin = pos0.value();
        AZ::Vector3 rayDirection = pos1.value() - pos0.value();
        rayDirection.Normalize();
        return AzToolsFramework::ViewportInteraction::ProjectedViewportRay{rayOrigin, rayDirection};
    }

    QPoint RenderViewportWidget::ViewportCursorScreenPosition()
    {
        return m_mousePosition.toPoint();
    }

    void RenderViewportWidget::SetWindowTitle(const AZStd::string& title)
    {
        setWindowTitle(QString::fromUtf8(title.c_str()));
    }

    AzFramework::WindowSize RenderViewportWidget::GetClientAreaSize() const
    {
        return AzFramework::WindowSize{aznumeric_cast<uint32_t>(width()), aznumeric_cast<uint32_t>(height())};
    }

    void RenderViewportWidget::ResizeClientArea(AzFramework::WindowSize clientAreaSize)
    {
        const QSize targetSize = QSize{aznumeric_cast<int>(clientAreaSize.m_width), aznumeric_cast<int>(clientAreaSize.m_height)};
        resize(targetSize);
    }

    bool RenderViewportWidget::GetFullScreenState() const
    {
        // The RenderViewportWidget does not currently support full screen.
        return false;
    }

    void RenderViewportWidget::SetFullScreenState([[maybe_unused]]bool fullScreenState)
    {
        // The RenderViewportWidget does not currently support full screen.
    }

    bool RenderViewportWidget::CanToggleFullScreenState() const
    {
        // The RenderViewportWidget does not currently support full screen.
        return false;
    }

    void RenderViewportWidget::ToggleFullScreenState()
    {
        // The RenderViewportWidget does not currently support full screen.
    }
} //namespace AtomToolsFramework
