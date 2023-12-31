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

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/Math/Matrix4x4.h>

#include <AzFramework/Input/Devices/Keyboard/InputDeviceKeyboard.h>
#include <AzFramework/Input/Devices/Mouse/InputDeviceMouse.h>
#include <AzFramework/Components/CameraBus.h>

#include <AtomLyIntegration/CommonFeatures/Mesh/MeshComponentBus.h>
#include <Atom/RPI.Public/RPISystemInterface.h>
#include <Atom/RPI.Public/Scene.h>
#include <Atom/Feature/SkyBox/SkyBoxFeatureProcessorInterface.h>

#include <Source/Viewport/InputController/MaterialEditorViewportInputController.h>
#include <Source/Viewport/InputController/IdleBehavior.h>
#include <Source/Viewport/InputController/PanCameraBehavior.h>
#include <Source/Viewport/InputController/OrbitCameraBehavior.h>
#include <Source/Viewport/InputController/MoveCameraBehavior.h>
#include <Source/Viewport/InputController/DollyCameraBehavior.h>
#include <Source/Viewport/InputController/RotateModelBehavior.h>
#include <Source/Viewport/InputController/RotateEnvironmentBehavior.h>

namespace MaterialEditor
{
    MaterialEditorViewportInputController::MaterialEditorViewportInputController()
        : AzFramework::SingleViewportController()
        , m_targetPosition(AZ::Vector3::CreateZero())
    {
        m_behaviorMap[None] = AZStd::make_shared<IdleBehavior>();
        m_behaviorMap[Lmb] = AZStd::make_shared<PanCameraBehavior>();
        m_behaviorMap[Mmb] = AZStd::make_shared<MoveCameraBehavior>();
        m_behaviorMap[Rmb] = AZStd::make_shared<OrbitCameraBehavior>(); // orbit around model
        m_behaviorMap[Alt ^ Lmb] = AZStd::make_shared<OrbitCameraBehavior>(); // orbit around arbitrary point
        m_behaviorMap[Alt ^ Mmb] = AZStd::make_shared<MoveCameraBehavior>();
        m_behaviorMap[Alt ^ Rmb] = AZStd::make_shared<DollyCameraBehavior>();
        m_behaviorMap[Lmb ^ Rmb] = AZStd::make_shared<DollyCameraBehavior>();
        m_behaviorMap[Ctrl ^ Lmb] = AZStd::make_shared<RotateModelBehavior>();
        m_behaviorMap[Shift ^ Lmb] = AZStd::make_shared<RotateEnvironmentBehavior>();
        m_behavior = m_behaviorMap[None];
    }

    MaterialEditorViewportInputController::~MaterialEditorViewportInputController()
    {
        if (m_initialized)
        {
            MaterialEditorViewportInputControllerRequestBus::Handler::BusDisconnect();
        }
    }

    void MaterialEditorViewportInputController::Init(const AZ::EntityId& cameraEntityId, const AZ::EntityId& targetEntityId, const AZ::EntityId& iblEntityId)
    {
        if (m_initialized)
        {
            AZ_Error("MaterialEditorViewportInputController", false, "Controller already initialized.");
            return;
        }
        m_initialized = true;
        m_cameraEntityId = cameraEntityId;
        m_targetEntityId = targetEntityId;
        m_iblEntityId = iblEntityId;

        MaterialEditorViewportInputControllerRequestBus::Handler::BusConnect();
    }

    const AZ::EntityId& MaterialEditorViewportInputController::GetCameraEntityId() const
    {
        return m_cameraEntityId;
    }

    const AZ::EntityId& MaterialEditorViewportInputController::GetTargetEntityId() const
    {
        return m_targetEntityId;
    }

    const AZ::EntityId& MaterialEditorViewportInputController::GetIblEntityId() const
    {
        return m_iblEntityId;
    }

    const AZ::Vector3& MaterialEditorViewportInputController::GetTargetPosition() const
    {
        return m_targetPosition;
    }

    void MaterialEditorViewportInputController::SetTargetPosition(const AZ::Vector3& targetPosition)
    {
        m_targetPosition = targetPosition;
    }

    float MaterialEditorViewportInputController::GetDistanceToTarget() const
    {
        AZ::Vector3 cameraPosition;
        AZ::TransformBus::EventResult(cameraPosition, m_cameraEntityId, &AZ::TransformBus::Events::GetLocalTranslation);
        return cameraPosition.GetDistance(m_targetPosition);
    }

    void MaterialEditorViewportInputController::GetExtents(float& distanceMin, float& distanceMax) const
    {
        distanceMin = m_distanceMin;
        distanceMax = m_distanceMax;
    }

    void MaterialEditorViewportInputController::UpdateViewport(AzFramework::ViewportId, AzFramework::FloatSeconds deltaTime, AZ::ScriptTimePoint)
    {
        if (m_keysChanged)
        {
            if (m_timeToBehaviorSwitchMs > 0)
            {
                m_timeToBehaviorSwitchMs -= deltaTime.count();
            }
            if (m_timeToBehaviorSwitchMs <= 0)
            {
                EvaluateControlBehavior();
                m_keysChanged = false;
            }
        }
    }

    bool MaterialEditorViewportInputController::HandleInputChannelEvent([[maybe_unused]] AzFramework::ViewportId viewport, const AzFramework::InputChannel& inputChannel)
    {
        using namespace AzFramework;

        const InputChannelId& inputChannelId = inputChannel.GetInputChannelId();
        const InputChannel::State state = inputChannel.GetState();
        const KeyMask keysOld = m_keys;

        switch (inputChannel.GetState())
        {
        case InputChannel::State::Began:
            if (inputChannelId == InputDeviceMouse::Button::Left)
            {
                m_keys |= Lmb;
            }
            else if (inputChannelId == InputDeviceMouse::Button::Middle)
            {
                m_keys |= Mmb;
            }
            else if (inputChannelId == InputDeviceMouse::Button::Right)
            {
                m_keys |= Rmb;
            }
            else if (inputChannelId == InputDeviceKeyboard::Key::ModifierAltL)
            {
                m_keys |= Alt;
            }
            else if (inputChannelId == InputDeviceKeyboard::Key::ModifierCtrlL)
            {
                m_keys |= Ctrl;
            }
            else if (inputChannelId == InputDeviceKeyboard::Key::ModifierShiftL)
            {
                m_keys |= Shift;
            }
            if (inputChannelId == InputDeviceMouse::Movement::X)
            {
                m_behavior->MoveX(inputChannel.GetValue());
            }
            else if (inputChannelId == InputDeviceMouse::Movement::Y)
            {
                m_behavior->MoveY(inputChannel.GetValue());
            }
            break;
        case InputChannel::State::Ended:
            if (inputChannelId == InputDeviceMouse::Button::Left)
            {
                m_keys &= ~Lmb;
            }
            else if (inputChannelId == InputDeviceMouse::Button::Middle)
            {
                m_keys &= ~Mmb;
            }
            else if (inputChannelId == InputDeviceMouse::Button::Right)
            {
                m_keys &= ~Rmb;
            }
            else if (inputChannelId == InputDeviceKeyboard::Key::ModifierAltL)
            {
                m_keys &= ~Alt;
            }
            else if (inputChannelId == InputDeviceKeyboard::Key::ModifierCtrlL)
            {
                m_keys &= ~Ctrl;
            }
            else if (inputChannelId == InputDeviceKeyboard::Key::ModifierShiftL)
            {
                m_keys &= ~Shift;
            }
            else if (inputChannelId == InputDeviceKeyboard::Key::AlphanumericZ)
            {
                Reset();
            }
            break;
        case InputChannel::State::Updated:
            if (inputChannelId == InputDeviceMouse::Movement::X)
            {
                m_behavior->MoveX(inputChannel.GetValue());
            }
            else if (inputChannelId == InputDeviceMouse::Movement::Y)
            {
                m_behavior->MoveY(inputChannel.GetValue());
            }
            break;
        }

        if (keysOld != m_keys)
        {
            m_keysChanged = true;
            m_timeToBehaviorSwitchMs = BehaviorSwitchDelayMs;
        }
        return false;
    }

    void MaterialEditorViewportInputController::Reset()
    {
        CalculateExtents();

        // reset camera
        m_targetPosition = m_modelCenter;
        const float distance = m_distanceMin * StartingDistanceMultiplier;
        const AZ::Quaternion cameraRotation = AZ::Quaternion::CreateFromAxisAngle(AZ::Vector3::CreateAxisZ(), StartingRotationAngle);
        AZ::Vector3 cameraPosition(m_targetPosition.GetX(), m_targetPosition.GetY() - distance, m_targetPosition.GetZ());
        cameraPosition = cameraRotation.TransformVector(cameraPosition);
        AZ::Transform cameraTransform = AZ::Transform::CreateFromQuaternionAndTranslation(cameraRotation, cameraPosition);
        AZ::TransformBus::Event(m_cameraEntityId, &AZ::TransformBus::Events::SetLocalTM, cameraTransform);

        // reset model
        AZ::Transform modelTransform = AZ::Transform::CreateIdentity();
        AZ::TransformBus::Event(m_targetEntityId, &AZ::TransformBus::Events::SetLocalTM, modelTransform);

        // reset environment
        AZ::Transform iblTransform = AZ::Transform::CreateIdentity();
        AZ::TransformBus::Event(m_iblEntityId, &AZ::TransformBus::Events::SetLocalTM, iblTransform);
        const AZ::Matrix4x4 rotationMatrix = AZ::Matrix4x4::CreateIdentity();
        AZ::RPI::ScenePtr scene = AZ::RPI::RPISystemInterface::Get()->GetDefaultScene();
        auto skyBoxFeatureProcessorInterface = scene->GetFeatureProcessor<AZ::Render::SkyBoxFeatureProcessorInterface>();
        skyBoxFeatureProcessorInterface->SetCubemapRotationMatrix(rotationMatrix);
    }

    void MaterialEditorViewportInputController::SetFieldOfView(float value)
    {
        Camera::CameraRequestBus::Event(m_cameraEntityId, &Camera::CameraRequestBus::Events::SetFovDegrees, value);
    }

    void MaterialEditorViewportInputController::CalculateExtents()
    {
        AZ::TransformBus::EventResult(m_modelCenter, m_targetEntityId, &AZ::TransformBus::Events::GetLocalTranslation);

        AZ::Data::AssetId modelAssetId;
        AZ::Render::MeshComponentRequestBus::EventResult(modelAssetId, m_targetEntityId,
            &AZ::Render::MeshComponentRequestBus::Events::GetModelAssetId);

        if (modelAssetId.IsValid())
        {
            AZ::Data::Asset<AZ::RPI::ModelAsset> modelAsset = AZ::Data::AssetManager::Instance().GetAsset(modelAssetId, azrtti_typeid<AZ::RPI::ModelAsset>(), AZ::Data::AssetLoadBehavior::PreLoad);
            modelAsset.BlockUntilLoadComplete();
            if (modelAsset.IsReady())
            {
                const AZ::Aabb& aabb = modelAsset->GetAabb();
                float radius;
                aabb.GetAsSphere(m_modelCenter, radius);

                m_distanceMin = 0.5f * AZ::GetMin(AZ::GetMin(aabb.GetExtents().GetX(), aabb.GetExtents().GetY()), aabb.GetExtents().GetZ()) + DepthNear;
                m_distanceMax = radius * MaxDistanceMultiplier;
            }
        }
    }

    void MaterialEditorViewportInputController::EvaluateControlBehavior()
    {
        auto it = m_behaviorMap.find(m_keys);
        if (it == m_behaviorMap.end())
        {
            m_behavior = m_behaviorMap[None];
            return;
        }

        if (it->second == m_behavior)
        {
            return;
        }

        if (m_keys == Rmb)
        {
            // Rmb+drag orbits the camera around the model, so we need to reset target position to model center
            m_targetPosition = m_modelCenter;
        }

        m_behavior->End();
        m_behavior = it->second;
        m_behavior->Start();
    }
} // namespace MaterialEditor
