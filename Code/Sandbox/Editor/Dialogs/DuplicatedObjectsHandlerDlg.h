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
// Original file Copyright Crytek GMBH or its affiliates, used under license.

#ifndef CRYINCLUDE_EDITOR_DIALOGS_DUPLICATEDOBJECTSHANDLERDLG_H
#define CRYINCLUDE_EDITOR_DIALOGS_DUPLICATEDOBJECTSHANDLERDLG_H
#pragma once

#if !defined(Q_MOC_RUN)
#include <QDialog>
#endif

namespace Ui
{
    class DuplicatedObjectsHandlerDlg;
}

class CDuplicatedObjectsHandlerDlg
    : public QDialog
{
    Q_OBJECT
public:
    CDuplicatedObjectsHandlerDlg(const QString& msg, QWidget* pParent = nullptr);
    virtual ~CDuplicatedObjectsHandlerDlg();

    enum EResult
    {
        eResult_None,
        eResult_Override,
        eResult_CreateCopies
    };

    EResult GetResult() const
    {
        return m_result;
    }

protected:

    EResult m_result;

    void OnBnClickedOverrideBtn();
    void OnBnClickedCreateCopiesBtn();

    QScopedPointer<Ui::DuplicatedObjectsHandlerDlg> m_ui;
};

#endif // CRYINCLUDE_EDITOR_DIALOGS_DUPLICATEDOBJECTSHANDLERDLG_H
