/*
* All or portions of this file Copyright(c) Amazon.com, Inc.or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution(the "License").All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file.Do not
* remove or modify any license notices.This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
#pragma once

#include <AzCore/RTTI/RTTI.h>
#include <AzCore/Component/Component.h>
#include <Pipeline/LyShineBuilder/UiCanvasBuilderWorker.h>

namespace LyShine
{
    namespace LyShineBuilder
    {
        class LyShineBuilderComponent
            : public AZ::Component
        {
        public:
            AZ_COMPONENT(LyShineBuilderComponent, "{EBDFDA04-0D23-4E54-BD4C-2EF8EEF5A606}");
            static void Reflect(AZ::ReflectContext* context);

            LyShineBuilderComponent() = default;

            //////////////////////////////////////////////////////////////////////////
            // AZ::Component
            void Activate() override;
            void Deactivate() override;
            //////////////////////////////////////////////////////////////////////////

        private:

            //class cannot be copied
            LyShineBuilderComponent(const LyShineBuilderComponent&) = delete;
            LyShineBuilderComponent& operator=(const LyShineBuilderComponent&) = delete;

            UiCanvasBuilderWorker m_uiCanvasBuilder;
        };
    } // namespace LyShineBuilder
} // namespace LyShine
