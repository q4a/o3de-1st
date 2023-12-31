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

#include <AzCore/Memory/SystemAllocator.h>
#include <Editor/LineEditValidatable.h>

namespace EMStudio
{
    AZ_CLASS_ALLOCATOR_IMPL(LineEditValidatable, AZ::SystemAllocator, 0)

    const QRegExp LineEditValidatable::s_defaultRegExp = QRegExp("(^[^{}\"%<>:\\\\/|?*]*$)");

    LineEditValidatable::LineEditValidatable(QWidget* parent, const QRegExp& regExp)
        : QLineEdit(parent)
        , m_validationExp(regExp)
        , m_lineValidator(m_validationExp, 0)
    {
        setValidator(&m_lineValidator);
        connect(this, &QLineEdit::textChanged, this, &LineEditValidatable::OnTextChanged);
        connect(this, &QLineEdit::editingFinished, this, &LineEditValidatable::OnEditingFinished);
    }

    void LineEditValidatable::SetPreviousText(const QString& previousText)
    {
        m_previousText = previousText;
    }

    QString LineEditValidatable::GetPreviousText() const
    {
        return m_previousText;
    }

    void LineEditValidatable::OnTextChanged()
    {
        if (IsValid())
        {
            setStyleSheet("");
        }
        else
        {
            setStyleSheet("border: 1px solid red;");
        }

        emit TextChanged();
    }

    void LineEditValidatable::OnEditingFinished()
    {
        if (IsValid() && m_previousText != text())
        {
            emit TextEditingFinished();
        }
        else
        {
            setText(m_previousText);
        }
    }

    bool LineEditValidatable::IsValid() const
    {
        if (m_validatorFunc)
        {
            return m_validatorFunc();
        }

        // Always return true in case the validator function is not callable and has not been set.
        return true;
    }

    void LineEditValidatable::focusInEvent([[maybe_unused]] QFocusEvent* event)
    {
        selectAll();
    }
} // namespace EMStudio
