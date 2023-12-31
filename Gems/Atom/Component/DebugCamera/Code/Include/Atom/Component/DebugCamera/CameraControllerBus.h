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

#include <AzCore/Component/ComponentBus.h>

namespace AZ
{
    class ReflectContext;

    namespace Debug
    {
        //! Requests sent to any camera controllers in one entity
        class CameraControllerRequests
            : public ComponentBus
        {
        public:
            using MutexType = AZStd::recursive_mutex;
            static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
            
            //! Enable the controller that has this typeId. Any other controllers which are different types will be disabled.
            virtual void Enable(TypeId typeId) = 0;

            //! Reset a controller to initial stats if it's enabled.
            virtual void Reset() = 0;

            //! Disable the controller which is enabled
            virtual void Disable() = 0;
        };

        using CameraControllerRequestBus = AZ::EBus<CameraControllerRequests>;


        //! Notifications sent by each camera controller
        class CameraControllerNotifications
            : public ComponentBus
        {
        public:

            //! @param typeId  the ID of the controller that was enabled
            virtual void OnControllerEnabled([[maybe_unused]] TypeId typeId) {}

            //! @param typeId  the ID of the controller that was disabled
            virtual void OnControllerDisabled([[maybe_unused]] TypeId typeId) {}

            //! Called when user input begins a camera move
            //! @param controllerTypeId  the ID of this camera controller
            //! @param channels  the bitmask indicating the channels that began moving. The channel values are controller-specific.
            virtual void OnCameraMoveBegan([[maybe_unused]] TypeId controllerTypeId, [[maybe_unused]] uint32_t channels) {}

            //! Called when user input ends after a camera move
            //! @param controllerTypeId  the ID of this camera controller
            //! @param channels  the bitmask indicating the channels that ended moving. The channel values are controller-specific.
            virtual void OnCameraMoveEnded([[maybe_unused]] TypeId controllerTypeId, [[maybe_unused]] uint32_t channels) {}
        };

        using CameraControllerNotificationBus = AZ::EBus<CameraControllerNotifications>;
    } // namespace Debug
} // namespace AZ
