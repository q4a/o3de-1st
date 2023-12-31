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

#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/RTTI/ReflectionManager.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace UnitTest
{
    /**
     * Unit test fixture for setting up an AssetManager
     */
    class AssetManagerTestFixture
        : public AllocatorsTestFixture
        // Only used to provide the serialize context for now
        , public AZ::ComponentApplicationBus::Handler
    {
    protected:
        void SetUp() override;
        void TearDown() override;
        
        //////////////////////////////////////////////////////////////////////////
        // ComponentApplicationMessages. 
        AZ::ComponentApplication* GetApplication() override { return nullptr; }
        void RegisterComponentDescriptor(const AZ::ComponentDescriptor*) override { }
        void UnregisterComponentDescriptor(const AZ::ComponentDescriptor*) override { }
        bool AddEntity(AZ::Entity*) override { return false; }
        bool RemoveEntity(AZ::Entity*) override { return false; }
        bool DeleteEntity(const AZ::EntityId&) override { return false; }
        AZ::Entity* FindEntity(const AZ::EntityId&) override { return nullptr; }
        AZ::BehaviorContext*  GetBehaviorContext() override { return nullptr; }
        AZ::JsonRegistrationContext* GetJsonRegistrationContext() override { return nullptr; }
        const char* GetExecutableFolder() const override { return nullptr; }
        const char* GetAppRoot() const override { return nullptr; }
        AZ::Debug::DrillerManager* GetDrillerManager() override { return nullptr; }
        void EnumerateEntities(const EntityCallback& /*callback*/) override {}
        AZ::SerializeContext* GetSerializeContext() override
        {
            return m_reflectionManager ? m_reflectionManager->GetReflectContext<AZ::SerializeContext>() : nullptr;
        }
        void QueryApplicationType(AZ::ApplicationTypeQuery& /*appType*/) const override {}
        //////////////////////////////////////////////////////////////////////////

    private:
        AZStd::unique_ptr<AZ::ReflectionManager> m_reflectionManager;
    };
} // namespace UnitTest

