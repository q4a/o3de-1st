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

#ifndef CRYINCLUDE_EDITOR_UTILS_PROPERTYMISCCTRL_H
#define CRYINCLUDE_EDITOR_UTILS_PROPERTYMISCCTRL_H
#pragma once

#if !defined(Q_MOC_RUN)
#include <AzCore/base.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyEditorAPI.h>
#include "ReflectedVar.h"
#include "Util/VariablePropertyType.h"
#include "Controls/ColorGradientCtrl.h"
#include "Controls/SplineCtrl.h"
#include <QWidget>
#endif

class QLabel;
class QLineEdit;

class UserPropertyEditor : public QWidget
{
    Q_OBJECT
public:
    AZ_CLASS_ALLOCATOR(UserPropertyEditor, AZ::SystemAllocator, 0);
    UserPropertyEditor(QWidget *pParent = nullptr);

    void SetValue(const QString &value, bool notify = true);
    QString GetValue() const { return m_value; }

    void SetData(bool canEdit, bool useTree, const QString &treeSeparator, const QString &dialogTitle, const std::vector<IVariable::IGetCustomItems::SItem>& items);
    void onEditClicked();

signals:
    void ValueChanged(const QString &value);
    void RefreshItems();

private:
    QLabel *m_valueLabel;
    QString m_value;

    bool m_canEdit;
    bool m_useTree;
    QString m_treeSeparator;
    QString m_dialogTitle;
    std::vector<IVariable::IGetCustomItems::SItem> m_items;
};

class UserPopupWidgetHandler : public QObject, public AzToolsFramework::PropertyHandler < CReflectedVarUser, UserPropertyEditor>
{
public:
    AZ_CLASS_ALLOCATOR(UserPopupWidgetHandler, AZ::SystemAllocator, 0);
    bool IsDefaultHandler() const override { return false; }
    QWidget* CreateGUI(QWidget *pParent) override;

    AZ::u32 GetHandlerName(void) const override  {return AZ_CRC("ePropertyUser", 0x65b972c0); }
    void ConsumeAttribute(UserPropertyEditor* GUI, AZ::u32 attrib, AzToolsFramework::PropertyAttributeReader* attrValue, const char* debugName) override;
    void WriteGUIValuesIntoProperty(size_t index, UserPropertyEditor* GUI, property_t& instance, AzToolsFramework::InstanceDataNode* node) override;
    bool ReadValuesIntoGUI(size_t index, UserPropertyEditor* GUI, const property_t& instance, AzToolsFramework::InstanceDataNode* node)  override;

};


class LensFlarePropertyWidget : public QWidget
{
    Q_OBJECT
public:
    AZ_CLASS_ALLOCATOR(LensFlarePropertyWidget, AZ::SystemAllocator, 0);
    LensFlarePropertyWidget(QWidget *pParent = nullptr);

    void SetValue(const QString &value);
    QString GetValue() const;

    void OnEditClicked();

signals:
    void ValueChanged(const QString &value);

private:
    QLineEdit *m_valueEdit;
};

class LensFlareHandler : public QObject, public AzToolsFramework::PropertyHandler < CReflectedVarGenericProperty, LensFlarePropertyWidget>
{
public:
    AZ_CLASS_ALLOCATOR(LensFlareHandler, AZ::SystemAllocator, 0);
    bool IsDefaultHandler() const override { return false; }
    QWidget* CreateGUI(QWidget *pParent) override;

    AZ::u32 GetHandlerName(void) const override  { return AZ_CRC("ePropertyFlare", 0x5ce803df); }
    void ConsumeAttribute(LensFlarePropertyWidget* GUI, AZ::u32 attrib, AzToolsFramework::PropertyAttributeReader* attrValue, const char* debugName) override;
    void WriteGUIValuesIntoProperty(size_t index, LensFlarePropertyWidget* GUI, property_t& instance, AzToolsFramework::InstanceDataNode* node) override;
    bool ReadValuesIntoGUI(size_t index, LensFlarePropertyWidget* GUI, const property_t& instance, AzToolsFramework::InstanceDataNode* node)  override;
};

class FloatCurveHandler : public QObject, public AzToolsFramework::PropertyHandler < CReflectedVarSpline, CSplineCtrl>
{
public:
    AZ_CLASS_ALLOCATOR(FloatCurveHandler, AZ::SystemAllocator, 0);
    bool IsDefaultHandler() const override { return false; }
    QWidget* CreateGUI(QWidget *pParent) override;

    AZ::u32 GetHandlerName(void) const override { return AZ_CRC("ePropertyFloatCurve", 0x7440ccce); }

    void ConsumeAttribute(CSplineCtrl* GUI, AZ::u32 attrib, AzToolsFramework::PropertyAttributeReader* attrValue, const char* debugName) override;
    void WriteGUIValuesIntoProperty(size_t index, CSplineCtrl* GUI, property_t& instance, AzToolsFramework::InstanceDataNode* node) override;
    bool ReadValuesIntoGUI(size_t index, CSplineCtrl* GUI, const property_t& instance, AzToolsFramework::InstanceDataNode* node)  override;

    void OnSplineChange(CSplineCtrl*);
};

class ColorCurveHandler : public QObject, public AzToolsFramework::PropertyHandler < CReflectedVarSpline, CColorGradientCtrl>
{
public:
    AZ_CLASS_ALLOCATOR(ColorCurveHandler, AZ::SystemAllocator, 0);
    bool IsDefaultHandler() const override { return false; }
    QWidget* CreateGUI(QWidget *pParent) override;

    AZ::u32 GetHandlerName(void) const override { return AZ_CRC("ePropertyColorCurve", 0xa30da4ec); }

    void ConsumeAttribute(CColorGradientCtrl* GUI, AZ::u32 attrib, AzToolsFramework::PropertyAttributeReader* attrValue, const char* debugName) override;
    void WriteGUIValuesIntoProperty(size_t index, CColorGradientCtrl* GUI, property_t& instance, AzToolsFramework::InstanceDataNode* node) override;
    bool ReadValuesIntoGUI(size_t index, CColorGradientCtrl* GUI, const property_t& instance, AzToolsFramework::InstanceDataNode* node)  override;
};
#endif // CRYINCLUDE_EDITOR_UTILS_PROPERTYMISCCTRL_H
