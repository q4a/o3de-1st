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
    //! Moves camera along its vertical and horizontal axis
    class MoveCameraBehavior final
        : public Behavior
    {
    public:
        MoveCameraBehavior() = default;
        virtual ~MoveCameraBehavior() = default;

        void Start() override;
        void End() override;

    protected:
        void TickInternal(float x, float y) override;
        float GetSensitivityX() override;
        float GetSensitivityY() override;

    private:
        static constexpr float SensitivityX = 0.01f;
        static constexpr float SensitivityY = 0.01f;

        AZ::EntityId m_cameraEntityId;
    };
} // namespace MaterialEditor
