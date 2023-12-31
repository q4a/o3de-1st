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

#include "OperatorDivideByNumber.h"
#include <ScriptCanvas/Libraries/Core/MethodUtility.h>
#include <ScriptCanvas/Core/Contracts/MathOperatorContract.h>
#include <AzCore/Math/MathUtils.h>
#include <ScriptCanvas/Data/NumericData.h>

#include <ScriptCanvas/Utils/SerializationUtils.h>

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace Operators
        {
            bool OperatorDivideByNumber::OperatorDivideByNumberVersionConverter(AZ::SerializeContext& serializeContext, AZ::SerializeContext::DataElementNode& rootElement)
            {
                if (rootElement.GetVersion() < Version::RemoveOperatorBase)
                {
                    // Remove ArithmeticOperatorUnary
                    if (!SerializationUtils::RemoveBaseClass(serializeContext, rootElement))
                    {
                        return false;
                    }

                    // Remove ArithmeticOperator
                    if (!SerializationUtils::RemoveBaseClass(serializeContext, rootElement))
                    {
                        return false;
                    }
                }

                return true;
            }

            void OperatorDivideByNumber::OnInit()
            {
                auto groupedSlots = GetSlotsWithDynamicGroup(GetDynamicGroupId());

                if (groupedSlots.empty())
                {
                    auto inputDataSlots = GetAllSlotsByDescriptor(SlotDescriptors::DataIn());
                    auto outputDataSlots = GetAllSlotsByDescriptor(SlotDescriptors::DataOut());

                    for (const Slot* inputSlot : inputDataSlots)
                    {
                        if (inputSlot->IsDynamicSlot() && inputSlot->GetName().compare("Divisor") != 0)
                        {
                            if (inputSlot->GetDynamicGroup() != GetDynamicGroupId())
                            {
                                SetDynamicGroup(inputSlot->GetId(), GetDynamicGroupId());
                            }
                        }
                    }

                    if (outputDataSlots.size() == 1)
                    {
                        const Slot* resultSlot = outputDataSlots.front();

                        if (resultSlot->GetDynamicGroup() != GetDynamicGroupId())
                        {
                            SetDynamicGroup(resultSlot->GetId(), GetDynamicGroupId());
                        }
                    }

                    groupedSlots = GetSlotsWithDynamicGroup(GetDynamicGroupId());
                }

                for (const Slot* slot : groupedSlots)
                {
                    if (slot->IsInput())
                    {
                        m_operandId = slot->GetId();
                    }
                }
            }

            void OperatorDivideByNumber::OnInputSignal(const SlotId& slotId)
            {
                if (slotId != OperatorDivideByNumberProperty::GetInSlotId(this))
                {
                    return;
                }

                Data::Type type = GetDisplayType(AZ::Crc32("DivideGroup"));

                if (!type.IsValid())
                {
                    return;
                }

                const Datum* operand = FindDatum(m_operandId);

                if (operand == nullptr)
                {
                    SCRIPTCANVAS_REPORT_ERROR((*this), "Operand is nullptr");
                    return;
                }

                const float divisorValue = aznumeric_cast<float>(OperatorDivideByNumberProperty::GetDivisor(this));

                if (AZ::IsClose(divisorValue, 0.f, std::numeric_limits<float>::epsilon()))
                {
                    SCRIPTCANVAS_REPORT_ERROR((*this), "Division by zero");
                    return;
                }

                Datum result;

                switch (type.GetType())
                {
                case Data::eType::Number:
                {
                    const Data::NumberType* number = operand->GetAs<Data::NumberType>();
                    Data::NumberType resultValue = (*number) / divisorValue;
                    result = Datum(resultValue);
                }
                break;
                case Data::eType::Vector2:
                {
                    const Data::Vector2Type* vector = operand->GetAs<Data::Vector2Type>();
                    AZ::Vector2 resultValue = (*vector) / divisorValue;
                    result = Datum(resultValue);
                }
                break;
                case Data::eType::Vector3:
                {
                    const Data::Vector3Type* vector = operand->GetAs<Data::Vector3Type>();
                    AZ::Vector3 resultValue = (*vector) / divisorValue;
                    result = Datum(resultValue);
                }
                break;
                case Data::eType::Vector4:
                {
                    const Data::Vector4Type* vector = operand->GetAs<Data::Vector4Type>();
                    AZ::Vector4 resultValue = (*vector) / divisorValue;
                    result = Datum(resultValue);
                }
                break;
                case Data::eType::Quaternion:
                {
                    const Data::QuaternionType* quaternion = operand->GetAs<Data::QuaternionType>();
                    Data::QuaternionType resultValue = (*quaternion) / divisorValue;
                    result = Datum(resultValue);
                }
                break;
                case Data::eType::Matrix3x3:
                {
                    const Data::Matrix3x3Type* matrix = operand->GetAs<Data::Matrix3x3Type>();
                    Data::Matrix3x3Type resultValue = (*matrix) / divisorValue;
                    result = Datum(resultValue);
                }
                break;
                case Data::eType::Color:
                {
                    const Data::ColorType *color = operand->GetAs<Data::ColorType>();
                    Data::ColorType resultValue = (*color) / divisorValue;
                    result = Datum(resultValue);
                }
                break;
                default:
                    result = Datum();
                    AZ_Error("Script Canvas", false, "Divide by Number does not support the provided data type.");
                    break;
                }

                PushOutput(result, (*OperatorDivideByNumberProperty::GetResultSlot(this)));
                SignalOutput(OperatorDivideByNumberProperty::GetOutSlotId(this));
            }
        }
    }
}
