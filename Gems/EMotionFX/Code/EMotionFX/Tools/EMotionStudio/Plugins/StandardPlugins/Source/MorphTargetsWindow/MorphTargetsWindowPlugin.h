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
#include <MysticQt/Source/DialogStack.h>
#include "../../../../EMStudioSDK/Source/DockWidgetPlugin.h"
#include <EMotionFX/CommandSystem/Source/SelectionCommands.h>
#include "MorphTargetGroupWidget.h"
#include <QVBoxLayout>
#include <QLabel>
#endif


namespace EMStudio
{
    class MorphTargetsWindowPlugin
        : public EMStudio::DockWidgetPlugin
    {
        Q_OBJECT
        MCORE_MEMORYOBJECTCATEGORY(MorphTargetsWindowPlugin, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_STANDARDPLUGINS);

    public:
        enum
        {
            CLASS_ID = 0x00000065
        };

        MorphTargetsWindowPlugin();
        ~MorphTargetsWindowPlugin();

        // overloaded
        const char* GetCompileDate() const override         { return MCORE_DATE; }
        const char* GetName() const override                { return "Morph Targets"; }
        uint32 GetClassID() const override                  { return MorphTargetsWindowPlugin::CLASS_ID; }
        const char* GetCreatorName() const override         { return "Amazon"; }
        float GetVersion() const override                   { return 1.0f;  }
        bool GetIsClosable() const override                 { return true;  }
        bool GetIsFloatable() const override                { return true;  }
        bool GetIsVertical() const override                 { return false; }

        // overloaded main init function
        bool Init() override;
        EMStudioPlugin* Clone() override;

        // update the morph targets window based on the current selection
        void ReInit(bool forceReInit = false);

        // clear all widgets from the window
        void Clear();

        // creates a new group with several morph targets
        void CreateGroup(const char* name, const AZStd::vector<EMotionFX::MorphTarget*>& morphTargets, const AZStd::vector<EMotionFX::MorphSetupInstance::MorphTarget*>& morphTargetInstances);
        EMotionFX::ActorInstance* GetActorInstance() const { return mCurrentActorInstance; }

        void UpdateInterface();
        void UpdateMorphTarget(const char* name);

    public slots:
        void WindowReInit(bool visible);

    private:
        // declare the callbacks
        MCORE_DEFINECOMMANDCALLBACK(CommandSelectCallback);
        MCORE_DEFINECOMMANDCALLBACK(CommandUnselectCallback);
        MCORE_DEFINECOMMANDCALLBACK(CommandClearSelectionCallback);
        MCORE_DEFINECOMMANDCALLBACK(CommandAdjustMorphTargetCallback);
        MCORE_DEFINECOMMANDCALLBACK(CommandAdjustActorInstanceCallback);

        AZStd::vector<MCore::Command::Callback*> m_callbacks;

        // holds the generated groups for the morph targets
        AZStd::vector<MorphTargetGroupWidget*>  mMorphTargetGroups;

        // holds the currently selected actor instance
        EMotionFX::ActorInstance*               mCurrentActorInstance;

        // some qt stuff
        QVBoxLayout*                            mStaticTextLayout;
        QWidget*                                mStaticTextWidget;
        MysticQt::DialogStack*                  mDialogStack;
        QLabel*                                 mInfoText;
    };
} // namespace EMStudio
