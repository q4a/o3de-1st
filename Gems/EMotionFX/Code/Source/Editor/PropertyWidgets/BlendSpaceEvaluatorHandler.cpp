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

#include <Editor/PropertyWidgets/BlendSpaceEvaluatorHandler.h>
#include <Editor/PropertyWidgets/PropertyWidgetAllocator.h>
#include <EMotionFX/Source/AnimGraphManager.h>
#include <EMotionFX/Source/BlendSpaceManager.h>
#include <EMotionFX/Source/EMotionFXManager.h>
#include <QHBoxLayout>
#include <QMessageBox>


namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(BlendSpaceEvaluatorPicker, PropertyWidgetAllocator, 0)
    AZ_CLASS_ALLOCATOR_IMPL(BlendSpaceEvaluatorHandler,PropertyWidgetAllocator, 0)

    BlendSpaceEvaluatorPicker::BlendSpaceEvaluatorPicker(QWidget* parent)
        : QComboBox(parent)
    {
        connect(this, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &BlendSpaceEvaluatorPicker::OnCurrentIndexChanged);

        const BlendSpaceManager* blendSpaceManager = GetAnimGraphManager().GetBlendSpaceManager();

        const size_t evaluatorCount = blendSpaceManager->GetEvaluatorCount();
        for (size_t i = 0; i < evaluatorCount; ++i)
        {
            const BlendSpaceParamEvaluator* evaluator = blendSpaceManager->GetEvaluator(i);

            const AZ::TypeId evaluatorType = azrtti_typeid(evaluator);
            addItem(evaluator->GetName(), evaluatorType.ToString<AZStd::string>().c_str());
        }
    }


    void BlendSpaceEvaluatorPicker::OnCurrentIndexChanged([[maybe_unused]] int index)
    {
        emit EvaluatorChanged();
    }


    AZ::TypeId BlendSpaceEvaluatorPicker::GetTypeId(int index) const
    {
        const AZStd::string typeString = itemData(index).toString().toUtf8().data();

        const AZ::TypeId typeId(typeString.c_str(), typeString.size());
        return typeId;
    }


    void BlendSpaceEvaluatorPicker::SetEvaluatorType(AZ::TypeId type)
    {
        int evaluatorCount = count();
        for (int i = 0; i < evaluatorCount; ++i)
        {
            const AZ::TypeId evaluatorType = GetTypeId(i);
            if (evaluatorType == type)
            {
                setCurrentIndex(i);
                return;
            }
        }

        setCurrentIndex(-1);
    }


    AZ::TypeId BlendSpaceEvaluatorPicker::GetEvaluatorType() const
    {
        return GetTypeId(currentIndex());
    }

    //---------------------------------------------------------------------------------------------------------------------------------------------------------

    BlendSpaceEvaluatorHandler::BlendSpaceEvaluatorHandler()
        : QObject()
        , AzToolsFramework::PropertyHandler<AZ::TypeId, BlendSpaceEvaluatorPicker>()
    {
    }


    AZ::u32 BlendSpaceEvaluatorHandler::GetHandlerName() const
    {
        return AZ_CRC("BlendSpaceEvaluator", 0x9a3f7d07);
    }


    QWidget* BlendSpaceEvaluatorHandler::CreateGUI(QWidget* parent)
    {
        BlendSpaceEvaluatorPicker* picker = aznew BlendSpaceEvaluatorPicker(parent);

        connect(picker, &BlendSpaceEvaluatorPicker::EvaluatorChanged, this, [picker]()
        {
            EBUS_EVENT(AzToolsFramework::PropertyEditorGUIMessages::Bus, RequestWrite, picker);
        });

        return picker;
    }


    void BlendSpaceEvaluatorHandler::ConsumeAttribute(BlendSpaceEvaluatorPicker* GUI, AZ::u32 attrib, AzToolsFramework::PropertyAttributeReader* attrValue, [[maybe_unused]] const char* debugName)
    {
        if (attrib == AZ::Edit::Attributes::ReadOnly)
        {
            bool value;
            if (attrValue->Read<bool>(value))
            {
                GUI->setEnabled(!value);
            }
        }
    }


    void BlendSpaceEvaluatorHandler::WriteGUIValuesIntoProperty([[maybe_unused]] size_t index, BlendSpaceEvaluatorPicker* GUI, property_t& instance, [[maybe_unused]] AzToolsFramework::InstanceDataNode* node)
    {
        instance = GUI->GetEvaluatorType();
    }


    bool BlendSpaceEvaluatorHandler::ReadValuesIntoGUI([[maybe_unused]] size_t index, BlendSpaceEvaluatorPicker* GUI, const property_t& instance, [[maybe_unused]] AzToolsFramework::InstanceDataNode* node)
    {
        QSignalBlocker signalBlocker(GUI);
        GUI->SetEvaluatorType(instance);
        return true;
    }
} // namespace EMotionFX

#include <Source/Editor/PropertyWidgets/moc_BlendSpaceEvaluatorHandler.cpp>