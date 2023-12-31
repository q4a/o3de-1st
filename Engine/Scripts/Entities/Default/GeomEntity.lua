----------------------------------------------------------------------------------------------------
--
-- All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
-- its licensors.
--
-- For complete copyright and license terms please see the LICENSE at the root of this
-- distribution (the "License"). All use of this software is governed by the License,
-- or, if provided, by the license below or the license accompanying this file. Do not
-- remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
-- WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
--
--
----------------------------------------------------------------------------------------------------
Script.ReloadScript("scripts/Utils/EntityUtils.lua")

GeomEntity =
{
     Client = {},
    Server = {},

 Editor={
 Icon="physicsobject.bmp",
 IconOnTop=1,
    }
}


--------------------------------------------------------------------------
function GeomEntity.Server:OnInit()
    self:SetFlags(ENTITY_FLAG_CLIENT_ONLY,0);
end

--------------------------------------------------------------------------
function GeomEntity.Client:OnInit()
    self:SetFlags(ENTITY_FLAG_CLIENT_ONLY,0);
end

------------------------------------------------------------------------------------------------------
function GeomEntity:OnPhysicsBreak( vPos,nPartId,nOtherPartId )
    self:ActivateOutput("Break",nPartId+1 );
end

------------------------------------------------------------------------------------------------------
function GeomEntity:Event_Remove()
    self:DrawSlot(0,0);
    self:DestroyPhysics();
    self:ActivateOutput( "Remove", true );
end

------------------------------------------------------------------------------------------------------
function GeomEntity:Event_Hide()
    self:Hide(1);
    self:ActivateOutput( "Hide", true );
end

------------------------------------------------------------------------------------------------------
function GeomEntity:Event_UnHide()
    self:Hide(0);
    self:ActivateOutput( "UnHide", true );
end

function GeomEntity:OnLoad(table)  
    self.health = table.health;
    self.dead = table.dead;
    if(table.bAnimateOffScreenShadow) then
        self.bAnimateOffScreenShadow = table.bAnimateOffScreenShadow;
    else
        self.bAnimateOffScreenShadow = false;
    end
end

function GeomEntity:OnSave(table)  
    table.health = self.health;
    table.dead = self.dead;
    if(self.bAnimateOffScreenShadow) then
        table.bAnimateOffScreenShadow = self.bAnimateOffScreenShadow;
    else
        table.bAnimateOffScreenShadow = false;
    end
end

-------------------------------------------------------
function GeomEntity:OnPropertyChange()
    self:OnReset();
end



GeomEntity.FlowEvents =
{
    Inputs =
    {
        Hide = { GeomEntity.Event_Hide, "bool" },
        UnHide = { GeomEntity.Event_UnHide, "bool" },
        Remove = { GeomEntity.Event_Remove, "bool" },
    },
    Outputs =
    {
        Hide = "bool",
        UnHide = "bool",
        Remove = "bool",
        Break = "int",
    },
}


MakeTargetableByAI(GeomEntity);
MakeKillable(GeomEntity);
MakeRenderProxyOptions(GeomEntity);

