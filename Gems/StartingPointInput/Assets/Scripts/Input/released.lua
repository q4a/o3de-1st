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
local released = 
{
    Properties = 
    {
        IncomingInputEventName = "",
        OutgoingGameplayEventName = "",
    },
}

function released:OnActivate()
    local inputBusId = InputEventNotificationId(self.Properties.IncomingInputEventName)
    self.inputBus = InputEventNotificationBus.Connect(self, inputBusId)
end

function released:OnPressed(floatValue)
    GameplayNotificationBus.Event.OnEventEnd(GameplayNotificationId(self.entityId, self.Properties.OutgoingGameplayEventName, "float"), floatValue)
end

function released:OnHeld(floatValue)
end

function released:OnReleased(floatValue)
    GameplayNotificationBus.Event.OnEventBegin(GameplayNotificationId(self.entityId, self.Properties.OutgoingGameplayEventName, "float"), floatValue)
end

function released:OnDeactivate()
    self.inputBus:Disconnect()
end

return released