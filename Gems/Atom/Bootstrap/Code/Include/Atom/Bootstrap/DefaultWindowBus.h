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
#include <AzCore/EBus/EBus.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>

#include <Atom/RPI.Public/WindowContext.h>

namespace AZ
{
    namespace Render
    {
        namespace Bootstrap
        {
            class DefaultWindowInterface
                : public AZ::EBusTraits
            {
            public:
                virtual ~DefaultWindowInterface() = default;

                // EBusTraits overrides
                static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
                static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;

                virtual AZStd::shared_ptr<RPI::WindowContext> GetDefaultWindowContext() = 0;

                //! whether to create the default render scene when default window is created in Bootstrap
                virtual void SetCreateDefaultScene(bool create) = 0;
            };

            using DefaultWindowBus = AZ::EBus<DefaultWindowInterface>;

            class DefaultWindowNotification
                : public AZ::EBusTraits
            {
            public:
                //////////////////////////////////////////////////////////////////////////
                // EBusTraits overrides
                static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
                static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;

                virtual ~DefaultWindowNotification() = default;

                /**
                 * Custom connection policy to make sure all we are fully in sync
                 */
                template <class Bus>
                struct DefaultWindowConnectionPolicy
                    : public EBusConnectionPolicy<Bus>
                {
                    static void Connect(typename Bus::BusPtr& busPtr, typename Bus::Context& context, typename Bus::HandlerNode& handler, typename Bus::Context::ConnectLockGuard& connectLock, const typename Bus::BusIdType& id = 0)
                    {
                        // connect
                        EBusConnectionPolicy<Bus>::Connect(busPtr, context, handler, connectLock, id);

                        // Check if default window already exists and fire notifications if it does
                        AZStd::shared_ptr<RPI::WindowContext> sharedWC;
                        DefaultWindowBus::BroadcastResult(sharedWC, &DefaultWindowBus::Events::GetDefaultWindowContext);
                        if (sharedWC)
                        {
                            handler->DefaultWindowCreated();
                        }
                    }
                };
                template<typename Bus>
                using ConnectionPolicy = DefaultWindowConnectionPolicy<Bus>;
                //////////////////////////////////////////////////////////////////////////

                virtual void DefaultWindowCreated() {};
                virtual void DefaultWindowPreDestroy() {};
                virtual void DefaultWindowDestroyed() {};
            };
            using DefaultWindowNotificationBus = AZ::EBus<DefaultWindowNotification>;
        } // namespace Bootstrap
    } // namespace Render
} // namespace AZ
