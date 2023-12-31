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
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <Atom/Feature/Utils/LightingPreset.h>
#include <Atom/Feature/Utils/ModelPreset.h>

namespace MaterialEditor
{
    class MaterialViewportNotifications
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;

        //! Signal that all configs are about to be reloaded
        virtual void OnBeginReloadContent() {}

        //! Signal that all configs were reloaded
        virtual void OnEndReloadContent() {}

        //! Signal that a preset was added
        //! @param preset being added
        virtual void OnLightingPresetAdded([[maybe_unused]] AZ::Render::LightingPresetPtr preset) {}

        //! Signal that a preset was selected
        //! @param preset being selected
        virtual void OnLightingPresetSelected([[maybe_unused]] AZ::Render::LightingPresetPtr preset) {}

        //! Signal that a preset was changed
        //! @param preset being changed
        virtual void OnLightingPresetChanged([[maybe_unused]] AZ::Render::LightingPresetPtr preset) {}

        //! Signal that a preset was added
        //! @param preset being added
        virtual void OnModelPresetAdded([[maybe_unused]] AZ::Render::ModelPresetPtr preset) {}

        //! Signal that a preset was selected
        //! @param preset being selected
        virtual void OnModelPresetSelected([[maybe_unused]] AZ::Render::ModelPresetPtr preset) {}

        //! Signal that a preset was changed
        //! @param preset being changed
        virtual void OnModelPresetChanged([[maybe_unused]] AZ::Render::ModelPresetPtr preset) {}

        //! Notify when enabled state for shadow catcher changes
        virtual void OnShadowCatcherEnabledChanged([[maybe_unused]] bool enable) {}

        //! Notify when enabled state for grid changes
        virtual void OnGridEnabledChanged([[maybe_unused]] bool enable) {}
    };

    using MaterialViewportNotificationBus = AZ::EBus<MaterialViewportNotifications>;
} // namespace MaterialEditor
