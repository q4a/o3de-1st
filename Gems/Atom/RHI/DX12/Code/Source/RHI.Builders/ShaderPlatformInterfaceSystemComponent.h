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

#include <AzCore/Component/Component.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

#include <RHI.Builders/ShaderPlatformInterface.h>

namespace AZ
{
    namespace DX12
    {
        class ShaderPlatformInterfaceSystemComponent final
            : public AZ::Component
        {
        public:
            AZ_COMPONENT(ShaderPlatformInterfaceSystemComponent, "{BBF7EE3B-982F-44A6-AE51-62099299763A}");
            static void Reflect(AZ::ReflectContext* context);

            ShaderPlatformInterfaceSystemComponent() = default;
            ~ShaderPlatformInterfaceSystemComponent() override = default;

            static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

            // AZ::Component
            void Activate() override;
            void Deactivate() override;

        private:
            ShaderPlatformInterfaceSystemComponent(const ShaderPlatformInterfaceSystemComponent&) = delete;

            AZStd::vector<AZStd::unique_ptr<RHI::ShaderPlatformInterface>> m_shaderPlatformInterfaces;
        };
    } // namespace DX12
} // namespace AZ
