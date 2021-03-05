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

#include <AzCore/EBus/EBus.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>

class QWidget;

namespace MaterialEditor
{
    //! MaterialEditorWindowRequestBus provides 
    class MaterialEditorWindowRequests
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;

        //! Add dockable widget in main window
        //! @param name title of the dockable window
        //! @param widget docked window content
        //! @param area location of docked window corresponding to Qt::DockWidgetArea
        //! @param orientation orientation of docked window corresponding to Qt::Orientation
        virtual bool AddDockWidget(const AZStd::string& name, QWidget* widget, uint32_t area, uint32_t orientation) = 0;

        //! Destroy dockable widget in main window
        //! @param name title of the dockable window
        virtual void RemoveDockWidget(const AZStd::string& name) = 0;

        //! Show or hide dockable widget in main window
        //! @param name title of the dockable window
        virtual void SetDockWidgetVisible(const AZStd::string& name, bool visible) = 0;

        //! Determine visibility of dockable widget in main window
        //! @param name title of the dockable window
        virtual bool IsDockWidgetVisible(const AZStd::string& name) const = 0;

        //! Get a list of registered docked widget names
        virtual AZStd::vector<AZStd::string> GetDockWidgetNames() const = 0;

        //! Resizes the Material Editor window to achieve a requested size for the viewport render target.
        //! (This indicates the size of the render target, not the desktop-scaled QT widget size).
        virtual void ResizeViewportRenderTarget(uint32_t width, uint32_t height) = 0;

        //! Forces the viewport's render target to use the given resolution, ignoring the size of the viewport widget.
        virtual void LockViewportRenderTargetSize(uint32_t width, uint32_t height) = 0;

        //! Releases the viewport's render target resolution lock, allowing it to match the viewport widget again.
        virtual void UnlockViewportRenderTargetSize() = 0;
    };
    using MaterialEditorWindowRequestBus = AZ::EBus<MaterialEditorWindowRequests>;

} // namespace MaterialEditor