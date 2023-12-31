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

#ifndef CRYINCLUDE_EDITOR_INCLUDE_OBJECTEVENT_H
#define CRYINCLUDE_EDITOR_INCLUDE_OBJECTEVENT_H
#pragma once

//! Standart objects types.
enum ObjectType
{
    OBJTYPE_DUMMY           = 1 << 20,
    OBJTYPE_AZENTITY        = 1 << 21,
};

//////////////////////////////////////////////////////////////////////////
//! Events that objects may want to handle.
//! Passed to OnEvent method of CBaseObject.
enum ObjectEvent
{
    EVENT_INGAME    = 1,    //!< Signals that editor is switching into the game mode.
    EVENT_OUTOFGAME,        //!< Signals that editor is switching out of the game mode.
    EVENT_REFRESH,          //!< Signals that editor is refreshing level.
    EVENT_DBLCLICK,         //!< Signals that object have been double clicked.
    EVENT_KEEP_HEIGHT,  //!< Signals that object must preserve its height over changed terrain.
    EVENT_RELOAD_ENTITY,//!< Signals that entities scripts must be reloaded.
    EVENT_RELOAD_TEXTURES,//!< Signals that all possible textures in objects should be reloaded.
    EVENT_RELOAD_GEOM,  //!< Signals that all possible geometries should be reloaded.
    EVENT_UNLOAD_GEOM,  //!< Signals that all possible geometries should be unloaded.
    EVENT_MISSION_CHANGE,   //!< Signals that mission have been changed.
    EVENT_ALIGN_TOGRID, //!< Object should align itself to the grid.

    EVENT_PHYSICS_GETSTATE,//!< Signals that entities should accept their physical state from game.
    EVENT_PHYSICS_RESETSTATE,//!< Signals that physics state must be reseted on objects.
    EVENT_PHYSICS_APPLYSTATE,//!< Signals that the stored physics state must be applied to objects.

    EVENT_PRE_EXPORT, //!< Signals that the game is about to be exported, prepare any data if the object needs to

    EVENT_FREE_GAME_DATA,//!< Object should free game data that its holding.
    EVENT_CONFIG_SPEC_CHANGE,   //!< Called when config spec changed.
    EVENT_HIDE_HELPER, //!< Signals that happens when Helper mode switches to be hidden.
};

#endif // CRYINCLUDE_EDITOR_INCLUDE_OBJECTEVENT_H

