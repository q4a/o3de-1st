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

#include <GraphCanvas/Widgets/EditorContextMenu/ContextMenuActions/DisableMenuActions/DisableMenuActions.h>

#include <GraphCanvas/Components/SceneBus.h>
#include <GraphCanvas/Editor/GraphModelBus.h>

namespace GraphCanvas
{
    //////////////////////////////
    // SetEnabledStateMenuAction
    //////////////////////////////
    
    SetEnabledStateMenuAction::SetEnabledStateMenuAction(QObject* object)
        : DisableContextMenuAction("Disable", object)
        , m_enableState(false)        
    {
    }

    void SetEnabledStateMenuAction::SetEnableState(bool enableState)
    {
        if (m_enableState != enableState)
        {
            m_enableState = enableState;

            if (m_enableState)
            {
                setText("Enable");
            }
            else
            {
                setText("Disable");
            }
        }
    }
    
    ContextMenuAction::SceneReaction SetEnabledStateMenuAction::TriggerAction([[maybe_unused]] const AZ::Vector2& scenePos)
    {
        const GraphId& graphId = GetGraphId();

        if (m_enableState)
        {
            SceneRequestBus::Event(graphId, &SceneRequests::EnableSelection);
        }
        else
        {
            SceneRequestBus::Event(graphId, &SceneRequests::DisableSelection);
        }
        
        return SceneReaction::PostUndo;
    }
}