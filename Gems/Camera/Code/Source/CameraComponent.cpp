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
#include "Camera_precompiled.h"
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Component/TransformBus.h>
#include <AzFramework/Entity/EntityDebugDisplayBus.h>
#include <Atom/RPI.Public/View.h>
#include <Atom/RPI.Public/ViewportContextManager.h>
#include <Atom/RPI.Public/ViewportContext.h>

#include "CameraComponent.h"

#include <MathConversion.h>
#include <AzCore/Math/MatrixUtils.h>
#include <IRenderer.h>
#include <AzCore/RTTI/BehaviorContext.h>

namespace Camera
{
    namespace ClassConverters
    {
        extern bool DeprecateCameraComponentWithoutEditor(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement);
        extern bool UpdateCameraComponentToUseController(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement);
    }

    CameraComponent::CameraComponent(const CameraComponentConfig& properties)
        : CameraComponentBase(properties)
    {
    }

    void CameraComponent::Activate()
    {
        CameraComponentBase::Activate();

        m_controller.ActivateAtomView();
    }

    void CameraComponent::Deactivate()
    {
        m_controller.DeactivateAtomView();

        CameraComponentBase::Deactivate();
    }

    static bool UpdateGameCameraComponentToUseController(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement)
    {
        if (!ClassConverters::UpdateCameraComponentToUseController(context, classElement))
        {
            return false;
        }

        classElement.Convert<CameraComponent>(context);
        return true;
    }

    void CameraComponent::Reflect(AZ::ReflectContext* reflection)
    {
        CameraComponentBase::Reflect(reflection);

        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection);
        if (serializeContext)
        {
            serializeContext->ClassDeprecate("CameraComponent", "{A0C21E18-F759-4E72-AF26-7A36FC59E477}", &ClassConverters::DeprecateCameraComponentWithoutEditor);
            serializeContext->ClassDeprecate("CameraComponent", "{E409F5C0-9919-4CA5-9488-1FE8A041768E}", &UpdateGameCameraComponentToUseController);
            serializeContext->Class<CameraComponent, CameraComponentBase>()
                ->Version(0)
            ;
        }

        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(reflection))
        {
            behaviorContext->EBus<CameraRequestBus>("CameraRequestBus")
                ->Event("GetNearClipDistance", &CameraRequestBus::Events::GetNearClipDistance)
                ->Event("GetFarClipDistance", &CameraRequestBus::Events::GetFarClipDistance)
                ->Event("GetFovDegrees", &CameraRequestBus::Events::GetFovDegrees)
                ->Event("SetFovDegrees", &CameraRequestBus::Events::SetFovDegrees)
                ->Event("GetFovRadians", &CameraRequestBus::Events::GetFovRadians)
                ->Event("SetFovRadians", &CameraRequestBus::Events::SetFovRadians)
                ->Event("GetFov", &CameraRequestBus::Events::GetFov) // Deprecated in 1.13
                ->Event("SetFov", &CameraRequestBus::Events::SetFov) // Deprecated in 1.13
                ->Event("SetNearClipDistance", &CameraRequestBus::Events::SetNearClipDistance)
                ->Event("SetFarClipDistance", &CameraRequestBus::Events::SetFarClipDistance)
                ->Event("MakeActiveView", &CameraRequestBus::Events::MakeActiveView)
                ->VirtualProperty("FieldOfView","GetFovDegrees","SetFovDegrees")
                ->VirtualProperty("NearClipDistance", "GetNearClipDistance", "SetNearClipDistance")
                ->VirtualProperty("FarClipDistance", "GetFarClipDistance", "SetFarClipDistance")
                ;

            behaviorContext->Class<CameraComponent>()->RequestBus("CameraRequestBus");

            behaviorContext->EBus<CameraSystemRequestBus>("CameraSystemRequestBus")
                ->Attribute(AZ::Script::Attributes::Category, "Camera")
                ->Event("GetActiveCamera", &CameraSystemRequestBus::Events::GetActiveCamera)
                ;
        }
    }
} //namespace Camera

