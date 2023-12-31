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

#include "WhiteBox_precompiled.h"

#include "Components/EditorWhiteBoxColliderComponent.h"
#include "EditorWhiteBoxComponent.h"
#include "EditorWhiteBoxSystemComponent.h"
#include "WhiteBoxEditorModule.h"

namespace WhiteBox
{
    AZ_CLASS_ALLOCATOR_IMPL(WhiteBoxEditorModule, AZ::SystemAllocator, 0)

    WhiteBoxEditorModule::WhiteBoxEditorModule()
        : WhiteBoxModule()
    {
        // push results of [MyComponent]::CreateDescriptor() into m_descriptors here
        m_descriptors.insert(
            m_descriptors.end(),
            {
                EditorWhiteBoxSystemComponent::CreateDescriptor(),
                EditorWhiteBoxComponent::CreateDescriptor(),
                EditorWhiteBoxColliderComponent::CreateDescriptor(),
            });
    }

    WhiteBoxEditorModule::~WhiteBoxEditorModule() = default;

    AZ::ComponentTypeList WhiteBoxEditorModule::GetRequiredSystemComponents() const
    {
        // add required SystemComponents to the SystemEntity
        return AZ::ComponentTypeList{azrtti_typeid<EditorWhiteBoxSystemComponent>()};
    }
} // namespace WhiteBox

AZ_DECLARE_MODULE_CLASS(Gem_WhiteBoxEditor, WhiteBox::WhiteBoxEditorModule)
