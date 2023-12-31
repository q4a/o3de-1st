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
#include <LyShine/Bus/UiCanvasManagerBus.h>
#include <LyShine/Bus/World/UiCanvasRefBus.h>
#include <LyShine/UiAssetTypes.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
class UiCanvasAssetRefComponent
    : public AZ::Component
    , public UiCanvasRefBus::Handler
    , public UiCanvasAssetRefBus::Handler
    , public UiCanvasManagerNotificationBus::Handler
{
public: // member functions

    AZ_COMPONENT(UiCanvasAssetRefComponent, "{05BED4D7-E331-4020-9C17-BD3F4CE4DE85}");

    UiCanvasAssetRefComponent();

    // UiCanvasRefInterface
    AZ::EntityId GetCanvas() override;
    // ~UiCanvasRefInterface

    // UiCanvasAssetRefInterface
    AZStd::string GetCanvasPathname() override;
    void SetCanvasPathname(const AZStd::string& pathname) override;
    bool GetIsAutoLoad() override;
    void SetIsAutoLoad(bool isAutoLoad) override;
    bool GetShouldLoadDisabled() override;
    void SetShouldLoadDisabled(bool shouldLoadDisabled) override;

    AZ::EntityId LoadCanvas() override;
    void UnloadCanvas() override;
    // ~UiCanvasAssetRefInterface

    // UiCanvasManagerNotification
    void OnCanvasUnloaded(AZ::EntityId canvasEntityId) override;
    // ~UiCanvasManagerNotification

public: // static member functions

    static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC("UiCanvasRefService", 0xb4cb5ef4));
    }

    static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC("UiCanvasRefService", 0xb4cb5ef4));
    }

    static void GetRequiredServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& required)
    {
    }

    static void Reflect(AZ::ReflectContext* context);

protected: // member functions

    // AZ::Component
    void Activate() override;
    void Deactivate() override;
    // ~AZ::Component

    AZ_DISABLE_COPY_MOVE(UiCanvasAssetRefComponent);

protected: // data

    //! Persistent properties
    AzFramework::SimpleAssetReference<LyShine::CanvasAsset> m_canvasAssetRef;
    bool m_isAutoLoad;
    bool m_shouldLoadDisabled;

    //! The UI Canvas that is associated with this component entity
    AZ::EntityId m_canvasEntityId;
};
