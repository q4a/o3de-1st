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

#if !defined(Q_MOC_RUN)
#include <QWidget>

#include <AzCore/std/containers/vector.h>
#include <AzQtComponents/Components/Widgets/Card.h>

#include <Source/Editor/ObjectEditor.h>

#include <EMotionStudio/Plugins/StandardPlugins/Source/MotionEvents/EventDataEditor.h>
#endif

class QMenu;
class QAction;

namespace EMotionFX
{
    class ObjectEditor;
    class MotionEvent;
}

namespace EMStudio
{
    class MotionEventEditor
        : public QWidget
    {
        Q_OBJECT //AUTOMOC
    public:
        MotionEventEditor(EMotionFX::Motion* motion = nullptr, EMotionFX::MotionEvent* event = nullptr, QWidget* parent = nullptr);

        EMotionFX::MotionEvent* GetMotionEvent() const { return m_motionEvent; }
        void SetMotionEvent(EMotionFX::Motion* motion, EMotionFX::MotionEvent* event);

    private:
        void Init();

        EMotionFX::MotionEvent* m_motionEvent = nullptr;
        EMotionFX::ObjectEditor* m_baseObjectEditor = nullptr;
        EMStudio::EventDataEditor m_eventDataEditor;
    };
} // end namespace EMStudio
