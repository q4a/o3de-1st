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

#include <Source/Viewport/InputController/Behavior.h>

namespace MaterialEditor
{
    //! Rotates target model in viewport
    class RotateModelBehavior final
        : public Behavior
    {
    public:
        RotateModelBehavior() = default;
        virtual ~RotateModelBehavior() = default;

        void Start() override;

    protected:
        void TickInternal(float x, float y) override;
        float GetSensitivityX() override;
        float GetSensitivityY() override;

    private:
        static constexpr float SensitivityX = 0.01f;
        static constexpr float SensitivityY = 0.01f;

        AZ::EntityId m_targetEntityId;
        AZ::Vector3 m_cameraRight = AZ::Vector3::CreateAxisX();
    };
} // namespace MaterialEditor
