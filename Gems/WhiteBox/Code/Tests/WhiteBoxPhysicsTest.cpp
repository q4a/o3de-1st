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

#include "WhiteBox_precompiled.h"

#include "Components/EditorWhiteBoxColliderComponent.h"
#include "Components/WhiteBoxColliderComponent.h"
#include "WhiteBox/EditorWhiteBoxComponentBus.h"
#include "WhiteBox/WhiteBoxToolApi.h"
#include "WhiteBoxTestFixtures.h"
#include "WhiteBoxTestUtil.h"

#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Slice/SliceAssetHandler.h>
#include <AzQtComponents/Utilities/QtPluginPaths.h>
#include <AzTest/GemTestEnvironment.h>
#include <AzToolsFramework/Commands/PreemptiveUndoCache.h>
#include <AzToolsFramework/Entity/EditorEntityContextComponent.h>
#include <AzToolsFramework/ToolsComponents/TransformComponent.h>

namespace UnitTest
{
    class EditorWhiteBoxPhysicsTestEnvironment : public AZ::Test::GemTestEnvironment
    {
        // AZ::Test::GemTestEnvironment ...
        void AddGemsAndComponents() override;
        void PostSystemEntityActivate() override;

    public:
        EditorWhiteBoxPhysicsTestEnvironment() = default;
        ~EditorWhiteBoxPhysicsTestEnvironment() override = default;
    };

    void EditorWhiteBoxPhysicsTestEnvironment::PostSystemEntityActivate()
    {
        AZ::SerializeContext* serializeContext = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(
            serializeContext, &AZ::ComponentApplicationBus::Events::GetSerializeContext);

        AZ::Data::AssetManager::Instance().RegisterHandler(
            aznew AZ::SliceAssetHandler(serializeContext), AZ::AzTypeInfo<AZ::SliceAsset>::Uuid());
    }

    void EditorWhiteBoxPhysicsTestEnvironment::AddGemsAndComponents()
    {
        AddDynamicModulePaths({"Gem.PhysX.Editor.4e08125824434932a0fe3717259caa47.v0.1.0"});
        AddComponentDescriptors(
            {AzToolsFramework::EditorEntityContextComponent::CreateDescriptor(),
             WhiteBox::EditorWhiteBoxComponent::CreateDescriptor(), WhiteBox::WhiteBoxComponent::CreateDescriptor(),
             WhiteBox::WhiteBoxColliderComponent::CreateDescriptor(),
             WhiteBox::EditorWhiteBoxColliderComponent::CreateDescriptor(),
             AzToolsFramework::Components::TransformComponent::CreateDescriptor()});
        AddRequiredComponents({AzToolsFramework::EditorEntityContextComponent::TYPEINFO_Uuid()});
    }

    class WhiteBoxPhysicsFixture : public ::testing::Test
    {
    };

    TEST_F(WhiteBoxPhysicsFixture, EditorWhiteBoxColliderComponentCanBeAddedToAnEmptyWhiteBoxComponent)
    {
        // given
        // create an entity with a transform and editor white box component
        AZ::Entity entity;
        entity.CreateComponent<AzToolsFramework::Components::TransformComponent>();
        auto editorWhiteBoxComponent = entity.CreateComponent<WhiteBox::EditorWhiteBoxComponent>();

        entity.Init();
        entity.Activate();

        WhiteBox::WhiteBoxMesh* whiteBox = nullptr;
        WhiteBox::EditorWhiteBoxComponentRequestBus::EventResult(
            whiteBox, AZ::EntityComponentIdPair(entity.GetId(), editorWhiteBoxComponent->GetId()),
            &WhiteBox::EditorWhiteBoxComponentRequests::GetWhiteBoxMesh);

        // clear all data from the white box mesh
        WhiteBox::Api::Clear(*whiteBox);

        // error messages present in EditorWhiteBoxComponent prior to fix for empty WhiteBoxMesh
        UnitTest::ErrorHandler physxCookFailed("Failed to cook triangle mesh. Please check the data is correct");
        UnitTest::ErrorHandler colliderCookFailed("Failed to cook mesh data");
        UnitTest::ErrorHandler invalidShape("Trying to add an invalid shape");
        UnitTest::ErrorHandler invalidConfiguration("Unable to create a shape from configuration");
        UnitTest::ErrorHandler physxError("TriangleMesh::loadFromDesc: desc.isValid() failed!");

        // when
        // add an editor white box collider component
        entity.Deactivate();
        auto editorWhiteBoxColliderComponent = entity.CreateComponent<WhiteBox::EditorWhiteBoxColliderComponent>();
        entity.Activate();

        // then
        // ensure none of the previous error messages are reported
        EXPECT_EQ(physxCookFailed.GetWarningCount(), 0);
        EXPECT_EQ(colliderCookFailed.GetWarningCount(), 0);
        EXPECT_EQ(invalidShape.GetErrorCount(), 0);
        EXPECT_EQ(invalidConfiguration.GetErrorCount(), 0);
        EXPECT_EQ(physxError.GetErrorCount(), 0);
    }

} // namespace UnitTest

// required to support running integration tests with Qt and PhysX
AZTEST_EXPORT int AZ_UNIT_TEST_HOOK_NAME(int argc, char** argv)
{
    ::testing::InitGoogleMock(&argc, argv);
    AzQtComponents::PrepareQtPaths();
    QApplication app(argc, argv);
    AZ::Test::printUnusedParametersWarning(argc, argv);
    AZ::Test::addTestEnvironments({new UnitTest::EditorWhiteBoxPhysicsTestEnvironment()});
    int result = RUN_ALL_TESTS();
    return result;
}

IMPLEMENT_TEST_EXECUTABLE_MAIN();
