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

#include <AzCore/Component/EntityId.h>
#include <AzCore/EBus/EBus.h>
#include <AzFramework/Entity/EntityContextBus.h>
#include <AzFramework/Entity/EntityDebugDisplayBus.h>
#include <AzFramework/Viewport/CameraState.h>
#include <AzFramework/Viewport/ViewportId.h>
#include <AzToolsFramework/Entity/EditorEntityContextBus.h>
#include <AzToolsFramework/Viewport/ViewportTypes.h>

class QPoint;

namespace AzToolsFramework
{
    namespace ViewportInteraction
    {
        /// Result of handling mouse interaction.
        enum class MouseInteractionResult
        {
            Manipulator,    ///< The manipulator manager handled the interaction.
            Viewport,       ///< The viewport handled the interaction.
            None            ///< The interaction was not handled.
        };

        /// Interface for handling mouse viewport events.
        class MouseViewportRequests
        {
        public:
            /// @cond
            virtual ~MouseViewportRequests() = default;
            /// @endcond

            /// Implement this function to handle a particular mouse event.
            virtual bool HandleMouseInteraction(
                const MouseInteractionEvent& /*mouseInteraction*/) { return false; }
        };
        
        /// Interface for internal handling mouse viewport events.
        class InternalMouseViewportRequests
        {
        public:
            /// @cond
            virtual ~InternalMouseViewportRequests() = default;
            /// @endcond

            /// Implement this function to have the viewport handle this mouse event.
            virtual bool InternalHandleMouseViewportInteraction(
                const MouseInteractionEvent& /*mouseInteraction*/) { return false; }

            /// Implement this function to have manipulators handle this mouse event.
            virtual bool InternalHandleMouseManipulatorInteraction(
                const MouseInteractionEvent& /*mouseInteraction*/) { return false; }

            /// Helper to call both viewport and manipulator handle mouse events
            /// @note Manipulators always attempt to intercept the event first.
            MouseInteractionResult InternalHandleAllMouseInteractions(const MouseInteractionEvent& mouseInteraction);
        };

        inline MouseInteractionResult InternalMouseViewportRequests::InternalHandleAllMouseInteractions(
            const MouseInteractionEvent& mouseInteraction)
        {
            if (InternalHandleMouseManipulatorInteraction(mouseInteraction))
            {
                return MouseInteractionResult::Manipulator;
            }
            else if (InternalHandleMouseViewportInteraction(mouseInteraction))
            {
                return MouseInteractionResult::Viewport;
            }
            else
            {
                return MouseInteractionResult::None;
            }
        }

        /// Interface for viewport selection behaviors.
        class ViewportDisplayNotifications
        {
        public:
            /// @cond
            virtual ~ViewportDisplayNotifications() = default;
            /// @endcond

            /// Display drawing in world space.
            /// \ref DisplayViewportSelection is called from \ref EditorInteractionSystemComponent::DisplayViewport.
            /// DisplayViewport exists on the \ref AzFramework::ViewportDebugDisplayEventBus and is called from \ref CRenderViewport.
            /// \ref DisplayViewportSelection is called after \ref CalculateVisibleEntityDatas on the \ref EditorVisibleEntityDataCache,
            /// this ensures usage of the entity cache will be up to date (do not implement \ref AzFramework::ViewportDebugDisplayEventBus
            /// directly if wishing to use the \ref EditorVisibleEntityDataCache).
            virtual void DisplayViewportSelection(
                const AzFramework::ViewportInfo& /*viewportInfo*/,
                AzFramework::DebugDisplayRequests& /*debugDisplay*/) {}
            /// Display drawing in screen space.
            /// \ref DisplayViewportSelection2d is called after \ref DisplayViewportSelection when the viewport has been
            /// configured to be orthographic in \ref CRenderViewport. All screen space drawing can be performed here.
            virtual void DisplayViewportSelection2d(
                const AzFramework::ViewportInfo& /*viewportInfo*/,
                AzFramework::DebugDisplayRequests& /*debugDisplay*/) {}
        };

        /// Interface for internal handling mouse viewport events and display notifications.
        /// Implement this for types wishing to provide viewport functionality and
        /// set it by using \ref EditorInteractionSystemViewportSelectionRequestBus.
        class InternalViewportSelectionRequests
            : public ViewportDisplayNotifications
            , public InternalMouseViewportRequests
        {
        };

        /// Interface for handling mouse viewport events and display notifications.
        /// Use this interface for composition types used by InternalViewportSelectionRequests.
        class ViewportSelectionRequests
            : public ViewportDisplayNotifications
            , public MouseViewportRequests
        {
        };

        /// The EBusTraits for ViewportInteractionRequests.
        class ViewportEBusTraits
            : public AZ::EBusTraits
        {
        public:
            using BusIdType = AzFramework::ViewportId; ///< ViewportId - used to address requests to this EBus.
            static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
            static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        };

        /// A ray projection, originating from a point and extending in a direction specified as a normal.
        struct ProjectedViewportRay
        {
            AZ::Vector3 origin;
            AZ::Vector3 direction;
        };

        /// Requests that can be made to the viewport to query and modify its state.
        class ViewportInteractionRequests
        {
        public:
            /// Return the current camera state for this viewport.
            virtual AzFramework::CameraState GetCameraState() = 0;
            /// Return if grid snapping is enabled.
            virtual bool GridSnappingEnabled() = 0;
            /// Return the grid snapping size.
            virtual float GridSize() = 0;
            /// Does the grid currently want to be displayed.
            virtual bool ShowGrid() = 0;
            /// Return if angle snapping is enabled.
            virtual bool AngleSnappingEnabled() = 0;
            /// Return the angle snapping/step size.
            virtual float AngleStep() = 0;
            /// Transform a point in world space to screen space coordinates.
            virtual QPoint ViewportWorldToScreen(const AZ::Vector3& worldPosition) = 0;
            /// Transform a point in screen space coordinates to a vector in world space based on clip space depth.
            /// Depth specifies a relative camera depth to project in the range of [0.f, 1.f].
            /// Returns the world space position if successful.
            virtual AZStd::optional<AZ::Vector3> ViewportScreenToWorld(const QPoint& screenPosition, float depth) = 0;
            /// Casts a point in screen space to a ray in world space originating from the viewport camera frustum's near plane.
            /// Returns a ray containing the ray's origin and a direction normal, if successful.
            virtual AZStd::optional<ProjectedViewportRay> ViewportScreenToWorldRay(const QPoint& screenPosition) = 0;
            /// Gets the last recorded cursor position in the viewport in screen space coordinates.
            virtual QPoint ViewportCursorScreenPosition() = 0;

        protected:
            ~ViewportInteractionRequests() = default;
        };

        /// Type to inherit to implement ViewportInteractionRequests.
        using ViewportInteractionRequestBus = AZ::EBus<ViewportInteractionRequests, ViewportEBusTraits>;

        //! Requests to freeze the Viewport Input
        //! Added to prevent a bug with the legacy CryEngine Viewport code that would
        //! keep doing raycast tests even when no level is loaded, causing a crash.
        class ViewportFreezeRequests
        {
        public:
            //! Return if Viewport Input is frozen
            virtual bool IsViewportInputFrozen() = 0;
            //! Sets the Viewport Input freeze state
            virtual void FreezeViewportInput(bool freeze) = 0;

        protected:
            ~ViewportFreezeRequests() = default;
        };

        //! Type to inherit to implement ViewportFreezeRequests.
        using ViewportFreezeRequestBus = AZ::EBus<ViewportFreezeRequests, ViewportEBusTraits>;

        /// Viewport requests that are only guaranteed to be serviced by the Main Editor viewport.
        class MainEditorViewportInteractionRequests
        {
        public:
            /// Given a point in screen space, return the picked entity (if any).
            /// Picked EntityId will be returned, InvalidEntityId will be returned on failure.
            virtual AZ::EntityId PickEntity(const QPoint& point) = 0;
            /// Given a point in screen space, return the terrain position in world space.
            virtual AZ::Vector3 PickTerrain(const QPoint& point) = 0;
            /// Return the terrain height given a world position in 2d (xy plane).
            virtual float TerrainHeight(const AZ::Vector2& position) = 0;
            /// Given the current view frustum (viewport) return all visible entities.
            virtual void FindVisibleEntities(AZStd::vector<AZ::EntityId>& visibleEntities) = 0;
            /// Is the user holding a modifier key to move the manipulator space from local to world.
            virtual bool ShowingWorldSpace() = 0;
            /// Return the widget to use as the parent for the viewport context menu.
            virtual QWidget* GetWidgetForViewportContextMenu() = 0;
            /// Set the render context for the viewport.
            virtual void BeginWidgetContext() = 0;
            /// End the render context for the viewport.
            /// Return to previous context before Begin was called.
            virtual void EndWidgetContext() = 0;

        protected:
            ~MainEditorViewportInteractionRequests() = default;
        };

        /// Type to inherit to implement MainEditorViewportInteractionRequests.
        using MainEditorViewportInteractionRequestBus = AZ::EBus<MainEditorViewportInteractionRequests, ViewportEBusTraits>;

        /// A helper to wrap Begin/EndWidgetContext.
        class WidgetContextGuard
        {
        public:
            explicit WidgetContextGuard(const int viewportId)
                : m_viewportId(viewportId)
            {
                MainEditorViewportInteractionRequestBus::Event(
                    viewportId, &MainEditorViewportInteractionRequestBus::Events::BeginWidgetContext);
            }

            ~WidgetContextGuard()
            {
                MainEditorViewportInteractionRequestBus::Event(
                    m_viewportId, &MainEditorViewportInteractionRequestBus::Events::EndWidgetContext);
            }

        private:
            int m_viewportId; ///< The viewport id the widget context is being set on.
        };

    } // namespace ViewportInteraction

    /// Temporary bus to query if the new Viewport Interaction Model mode is enabled or not.
    class NewViewportInteractionModelEnabledRequests
        : public AZ::EBusTraits
    {
    public:
        virtual bool IsNewViewportInteractionModelEnabled() = 0;

    protected:
        ~NewViewportInteractionModelEnabledRequests() = default;
    };

    /// Type to inherit to implement NewViewportInteractionModelEnabledRequests
    using NewViewportInteractionModelEnabledRequestBus = AZ::EBus<NewViewportInteractionModelEnabledRequests>;

    /// Utility function to return EntityContextId.
    inline AzFramework::EntityContextId GetEntityContextId()
    {
        AzFramework::EntityContextId entityContextId;
        EditorEntityContextRequestBus::BroadcastResult(
            entityContextId, &EditorEntityContextRequests::GetEditorEntityContextId);

        return entityContextId;
    }

    /// Utility function to return if the new Viewport Interaction Model
    /// is enabled (wraps NewViewportInteractionModelEnabledRequests).
    inline bool IsNewViewportInteractionModelEnabled()
    {
        bool newViewportInteractionModelEnabled = false;
        NewViewportInteractionModelEnabledRequestBus::BroadcastResult(
            newViewportInteractionModelEnabled,
            &NewViewportInteractionModelEnabledRequests::IsNewViewportInteractionModelEnabled);

        return newViewportInteractionModelEnabled;
    }
} // namespace AzToolsFramework
