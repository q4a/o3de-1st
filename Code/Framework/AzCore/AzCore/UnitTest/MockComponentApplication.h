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

#include <AzCore/Component/ComponentApplicationBus.h>
#include <gmock/gmock.h>

namespace UnitTest
{
    class MockComponentApplication
        : public AZ::ComponentApplicationBus::Handler
    {
    public:
        MockComponentApplication();
        ~MockComponentApplication();

    protected:
        MOCK_METHOD1(FindEntity, AZ::Entity* (const AZ::EntityId&));
        MOCK_METHOD1(AddEntity, bool (AZ::Entity*));
        MOCK_METHOD0(Destroy, void ());
        MOCK_METHOD1(RegisterComponentDescriptor, void (const AZ::ComponentDescriptor*));
        MOCK_METHOD1(UnregisterComponentDescriptor, void (const AZ::ComponentDescriptor*));
        MOCK_METHOD1(RemoveEntity, bool (AZ::Entity*));
        MOCK_METHOD1(DeleteEntity, bool (const AZ::EntityId&));
        MOCK_METHOD1(GetEntityName, AZStd::string (const AZ::EntityId&));
        MOCK_METHOD1(EnumerateEntities, void (const ComponentApplicationRequests::EntityCallback&));
        MOCK_METHOD0(GetApplication, AZ::ComponentApplication* ());
        MOCK_METHOD0(GetSerializeContext, AZ::SerializeContext* ());
        MOCK_METHOD0(GetJsonRegistrationContext, AZ::JsonRegistrationContext* ());
        MOCK_METHOD0(GetBehaviorContext, AZ::BehaviorContext* ());
        MOCK_CONST_METHOD0(GetAppRoot, const char* ());
        MOCK_CONST_METHOD0(GetExecutableFolder, const char* ());
        MOCK_METHOD0(GetDrillerManager, AZ::Debug::DrillerManager* ());
        MOCK_CONST_METHOD1(QueryApplicationType, void(AZ::ApplicationTypeQuery&));
    };
} // namespace UnitTest
