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

#include <EMotionFX/Source/AnimGraphMotionNode.h>
#include <EMotionFX/Source/AnimGraphNode.h>
#include <EMotionFX/Source/Parameter/TagParameter.h>
#include <Editor/AnimGraphEditorBus.h>
#include <Editor/PropertyWidgets/AnimGraphTagHandler.h>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QSignalBlocker>


namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(AnimGraphTagSelector, EditorAllocator, 0)
    AZ_CLASS_ALLOCATOR_IMPL(AnimGraphTagHandler, EditorAllocator, 0)

    AnimGraphTagSelector::AnimGraphTagSelector(QWidget* parent)
        : TagSelector(parent)
    {
        connect(this, &AnimGraphTagSelector::TagsChanged, this, [this]()
            {
                EBUS_EVENT(AzToolsFramework::PropertyEditorGUIMessages::Bus, RequestWrite, this);
                AzToolsFramework::PropertyEditorGUIMessages::Bus::Broadcast(&AzToolsFramework::PropertyEditorGUIMessages::Bus::Handler::OnEditingFinished, this);
            });
    }

    void AnimGraphTagSelector::GetAvailableTags(QVector<QString>& outTags) const
    {
        outTags.clear();

        if (!m_animGraph)
        {
            AZ_Error("EMotionFX", false, "Cannot open anim graph node selection window. No valid anim graph.");
            return;
        }

        const ValueParameterVector& valueParameters = m_animGraph->RecursivelyGetValueParameters();
        for (const ValueParameter* valueParameter : valueParameters)
        {
            if (azrtti_typeid(valueParameter) == azrtti_typeid<TagParameter>())
            {
                outTags.push_back(valueParameter->GetName().c_str());
            }
        }
    }

    //---------------------------------------------------------------------------------------------------------------------------------------------------------

    AnimGraphTagHandler::AnimGraphTagHandler()
        : QObject()
        , AzToolsFramework::PropertyHandler<AZStd::vector<AZStd::string>, AnimGraphTagSelector>()
        , m_animGraph(nullptr)
    {
    }

    AZ::u32 AnimGraphTagHandler::GetHandlerName() const
    {
        return AZ_CRC("AnimGraphTags", 0x05dc9a94);
    }


    QWidget* AnimGraphTagHandler::CreateGUI(QWidget* parent)
    {
        return aznew AnimGraphTagSelector(parent);
    }


    void AnimGraphTagHandler::ConsumeAttribute(AnimGraphTagSelector* GUI, AZ::u32 attrib, AzToolsFramework::PropertyAttributeReader* attrValue, [[maybe_unused]] const char* debugName)
    {
        if (attrib == AZ::Edit::Attributes::ReadOnly)
        {
            bool value;
            if (attrValue->Read<bool>(value))
            {
                GUI->setEnabled(!value);
            }
        }

        if (attrib == AZ_CRC("AnimGraph", 0x0d53d4b3))
        {
            attrValue->Read<AnimGraph*>(m_animGraph);
            GUI->SetAnimGraph(m_animGraph);
        }
    }


    void AnimGraphTagHandler::WriteGUIValuesIntoProperty([[maybe_unused]] size_t index, AnimGraphTagSelector* GUI, property_t& instance, [[maybe_unused]] AzToolsFramework::InstanceDataNode* node)
    {
        instance = GUI->GetTags();
    }


    bool AnimGraphTagHandler::ReadValuesIntoGUI([[maybe_unused]] size_t index, AnimGraphTagSelector* GUI, const property_t& instance, [[maybe_unused]] AzToolsFramework::InstanceDataNode* node)
    {
        QSignalBlocker signalBlocker(GUI);
        GUI->SetTags(instance);
        return true;
    }
} // namespace EMotionFX