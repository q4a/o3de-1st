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

#include "LmbrCentral_precompiled.h"
#include <AzTest/AzTest.h>

#include <AZTestShared/Math/MathTestHelpers.h>
#include <AzFramework/Components/TransformComponent.h>
#include <AzCore/Component/ComponentApplication.h>
#include <Shape/SplineComponent.h>
#include <Shape/TubeShapeComponent.h>
#include <AzCore/UnitTest/TestTypes.h>

namespace UnitTest
{
    class TubeShapeTest
        : public AllocatorsFixture
    {
        AZStd::unique_ptr<AZ::SerializeContext> m_serializeContext;
        AZStd::unique_ptr<AZ::ComponentDescriptor> m_transformComponentDescriptor;
        AZStd::unique_ptr<AZ::ComponentDescriptor> m_splineComponentDescriptor;
        AZStd::unique_ptr<AZ::ComponentDescriptor> m_tubeShapeComponentDescriptor;

    public:
        void SetUp() override
        {
            AllocatorsFixture::SetUp();
            m_serializeContext = AZStd::make_unique<AZ::SerializeContext>();

            m_transformComponentDescriptor = AZStd::unique_ptr<AZ::ComponentDescriptor>(AzFramework::TransformComponent::CreateDescriptor());
            m_transformComponentDescriptor->Reflect(&(*m_serializeContext));
            m_tubeShapeComponentDescriptor = AZStd::unique_ptr<AZ::ComponentDescriptor>(LmbrCentral::TubeShapeComponent::CreateDescriptor());
            m_tubeShapeComponentDescriptor->Reflect(&(*m_serializeContext));
            m_splineComponentDescriptor = AZStd::unique_ptr<AZ::ComponentDescriptor>(LmbrCentral::SplineComponent::CreateDescriptor());
            m_splineComponentDescriptor->Reflect(&(*m_serializeContext));
        }

        void TearDown() override
        {
            m_transformComponentDescriptor.reset();
            m_tubeShapeComponentDescriptor.reset();
            m_splineComponentDescriptor.reset();
            m_serializeContext.reset(); 
            AllocatorsFixture::TearDown();
        }
    };

    void CreateTube(const AZ::Transform& transform, const float radius, AZ::Entity& entity)
    {
        entity.CreateComponent<AzFramework::TransformComponent>();
        entity.CreateComponent<LmbrCentral::SplineComponent>();
        entity.CreateComponent<LmbrCentral::TubeShapeComponent>();

        entity.Init();
        entity.Activate();

        AZ::TransformBus::Event(entity.GetId(), &AZ::TransformBus::Events::SetWorldTM, transform);

        LmbrCentral::SplineComponentRequestBus::Event(
            entity.GetId(), &LmbrCentral::SplineComponentRequests::SetVertices,
            AZStd::vector<AZ::Vector3>
            {
                AZ::Vector3(-3.0f, 0.0f, 0.0f), AZ::Vector3(-1.0f, 0.0f, 0.0f),
                AZ::Vector3(1.0f, 0.0f, 0.0f), AZ::Vector3(3.0f, 0.0f, 0.0f)
            }
        );

        LmbrCentral::TubeShapeComponentRequestsBus::Event(entity.GetId(), &LmbrCentral::TubeShapeComponentRequestsBus::Events::SetRadius, radius);
    }

    TEST_F(TubeShapeTest, GetRayIntersectTubeSuccess1)
    {
        AZ::Entity entity;
        CreateTube(
            AZ::Transform::CreateIdentity(),
            1.0f,
            entity);

        bool rayHit = false;
        float distance;
        LmbrCentral::ShapeComponentRequestsBus::EventResult(
            rayHit, entity.GetId(), &LmbrCentral::ShapeComponentRequests::IntersectRay,
            AZ::Vector3(0.0f, -3.0f, 0.0f), AZ::Vector3(0.0f, 1.0f, 0.0f), distance);

        EXPECT_TRUE(rayHit);
        EXPECT_NEAR(distance, 2.0f, 1e-2f);
    }

    // firing at end of tube
    TEST_F(TubeShapeTest, GetRayIntersectTubeSuccess2)
    {
        AZ::Entity entity;
        CreateTube(
            AZ::Transform::CreateIdentity(),
            1.0f,
            entity);

        bool rayHit = false;
        float distance;
        LmbrCentral::ShapeComponentRequestsBus::EventResult(
            rayHit, entity.GetId(), &LmbrCentral::ShapeComponentRequests::IntersectRay,
            AZ::Vector3(6.0f, 0.0f, 0.0f), AZ::Vector3(-1.0f, 0.0f, 0.0f), distance);

        EXPECT_TRUE(rayHit);
        EXPECT_NEAR(distance, 2.0f, 1e-2f);
    }

    // firing at beginning of tube
    TEST_F(TubeShapeTest, GetRayIntersectTubeSuccess3)
    {
        AZ::Entity entity;
        CreateTube(
            AZ::Transform::CreateIdentity(),
            1.0f,
            entity);

        bool rayHit = false;
        float distance;
        LmbrCentral::ShapeComponentRequestsBus::EventResult(
            rayHit, entity.GetId(), &LmbrCentral::ShapeComponentRequests::IntersectRay,
            AZ::Vector3(-6.0f, 0.0f, 0.0f), AZ::Vector3(1.0f, 0.0f, 0.0f), distance);

        EXPECT_TRUE(rayHit);
        EXPECT_NEAR(distance, 2.0f, 1e-2f);
    }

    // transformed and scaled
    TEST_F(TubeShapeTest, GetRayIntersectTubeSuccess4)
    {
        AZ::Entity entity;
        CreateTube(
            AZ::Transform::CreateTranslation(AZ::Vector3(-40.0f, 6.0f, 1.0f)) *
            AZ::Transform::CreateScale(AZ::Vector3(2.5f, 1.0f, 1.0f)), // test max scale
            1.0f,
            entity);

        // set variable radius
        LmbrCentral::TubeShapeComponentRequestsBus::Event(
            entity.GetId(), &LmbrCentral::TubeShapeComponentRequestsBus::Events::SetVariableRadius, 0, 1.0f);
        LmbrCentral::TubeShapeComponentRequestsBus::Event(
            entity.GetId(), &LmbrCentral::TubeShapeComponentRequestsBus::Events::SetVariableRadius, 3, 2.0f);

        bool rayHit = false;
        float distance;
        LmbrCentral::ShapeComponentRequestsBus::EventResult(
            rayHit, entity.GetId(), &LmbrCentral::ShapeComponentRequests::IntersectRay,
            AZ::Vector3(-17.0f, 6.0f, 1.0f), AZ::Vector3(-1.0f, 0.0f, 0.0f), distance);

        EXPECT_TRUE(rayHit);
        EXPECT_NEAR(distance, 8.0f, 1e-2f);
    }

    // above tube
    TEST_F(TubeShapeTest, GetRayIntersectTubeFailure)
    {
        AZ::Entity entity;
        CreateTube(
            AZ::Transform::CreateIdentity(),
            1.0f,
            entity);

        bool rayHit = false;
        float distance;
        LmbrCentral::ShapeComponentRequestsBus::EventResult(
            rayHit, entity.GetId(), &LmbrCentral::ShapeComponentRequests::IntersectRay,
            AZ::Vector3(0.0f, 2.0f, 2.0f), AZ::Vector3(0.0f, 1.0f, 0.0f), distance);

        EXPECT_FALSE(rayHit);
    }

    TEST_F(TubeShapeTest, GetAabb1)
    {
        AZ::Entity entity;
        CreateTube(AZ::Transform::CreateFromQuaternionAndTranslation(
            AZ::Quaternion::CreateIdentity(), AZ::Vector3(0.0f, -10.0f, 0.0f)),
            1.0f, entity);

        AZ::Aabb aabb;
        LmbrCentral::ShapeComponentRequestsBus::EventResult(
            aabb, entity.GetId(), &LmbrCentral::ShapeComponentRequests::GetEncompassingAabb);

        EXPECT_THAT(aabb.GetMin(), IsClose(AZ::Vector3(-4.0f, -11.0f, -1.0f)));
        EXPECT_THAT(aabb.GetMax(), IsClose(AZ::Vector3(4.0f, -9.0f, 1.0f)));
    }

    TEST_F(TubeShapeTest, GetAabb2)
    {
        AZ::Entity entity;
        CreateTube(AZ::Transform::CreateFromQuaternionAndTranslation(
            AZ::Quaternion::CreateFromAxisAngle(AZ::Vector3::CreateAxisX(), AZ::Constants::QuarterPi) *
            AZ::Quaternion::CreateFromAxisAngle(AZ::Vector3::CreateAxisY(), AZ::Constants::QuarterPi), AZ::Vector3(-10.0f, -10.0f, 0.0f)),
            2.0f, entity);

        AZ::Aabb aabb;
        LmbrCentral::ShapeComponentRequestsBus::EventResult(
            aabb, entity.GetId(), &LmbrCentral::ShapeComponentRequests::GetEncompassingAabb);

        EXPECT_THAT(aabb.GetMin(), IsClose(AZ::Vector3(-14.1213f, -13.5f, -3.5f)));
        EXPECT_THAT(aabb.GetMax(), IsClose(AZ::Vector3(-5.8786f, -6.5f, 3.5f)));
    }

    TEST_F(TubeShapeTest, GetAabb3)
    {
        AZ::Entity entity;
        CreateTube(AZ::Transform::CreateFromQuaternionAndTranslation(
            AZ::Quaternion::CreateFromAxisAngle(AZ::Vector3::CreateAxisX(), AZ::Constants::QuarterPi) *
            AZ::Quaternion::CreateFromAxisAngle(AZ::Vector3::CreateAxisY(), AZ::Constants::QuarterPi), AZ::Vector3(-10.0f, -10.0f, 0.0f)),
            2.0f, entity);

        // set variable radius
        LmbrCentral::TubeShapeComponentRequestsBus::Event(
            entity.GetId(), &LmbrCentral::TubeShapeComponentRequestsBus::Events::SetVariableRadius, 0, 1.0f);

        AZ::Aabb aabb;
        LmbrCentral::ShapeComponentRequestsBus::EventResult(
            aabb, entity.GetId(), &LmbrCentral::ShapeComponentRequests::GetEncompassingAabb);

        EXPECT_THAT(aabb.GetMin(), IsClose(AZ::Vector3(-15.1213f, -14.5f, -4.5f)));
        EXPECT_THAT(aabb.GetMax(), IsClose(AZ::Vector3(-4.87867f, -5.5f, 4.5f)));
    }

    // variable radius and scale
    TEST_F(TubeShapeTest, GetAabb4)
    {
        AZ::Entity entity;
        CreateTube(AZ::Transform::CreateScale(AZ::Vector3(1.0f, 1.0f, 2.0f)), 1.0f, entity);

        // set variable radius
        LmbrCentral::TubeShapeComponentRequestsBus::Event(
            entity.GetId(), &LmbrCentral::TubeShapeComponentRequestsBus::Events::SetVariableRadius, 0, 1.0f);
        LmbrCentral::TubeShapeComponentRequestsBus::Event(
            entity.GetId(), &LmbrCentral::TubeShapeComponentRequestsBus::Events::SetVariableRadius, 3, 3.0f);

        AZ::Aabb aabb;
        LmbrCentral::ShapeComponentRequestsBus::EventResult(
            aabb, entity.GetId(), &LmbrCentral::ShapeComponentRequests::GetEncompassingAabb);

        EXPECT_THAT(aabb.GetMin(), IsClose(AZ::Vector3(-14.0f, -8.0f, -8.0f)));
        EXPECT_THAT(aabb.GetMax(), IsClose(AZ::Vector3(14.0f, 8.0f, 8.0f)));
    }

    // variable radius and scale
    TEST_F(TubeShapeTest, IsPointInsideSuccess1)
    {
        AZ::Entity entity;
        CreateTube(
            AZ::Transform::CreateTranslation(AZ::Vector3(37.0f, 36.0f, 32.0f)) *
            AZ::Transform::CreateScale(AZ::Vector3(1.0f, 2.0f, 1.0f)), 1.5f, entity);

        // set variable radius
        LmbrCentral::TubeShapeComponentRequestsBus::Event(
            entity.GetId(), &LmbrCentral::TubeShapeComponentRequestsBus::Events::SetVariableRadius, 0, 1.0f);
        LmbrCentral::TubeShapeComponentRequestsBus::Event(
            entity.GetId(), &LmbrCentral::TubeShapeComponentRequestsBus::Events::SetVariableRadius, 3, 3.0f);

        bool inside;
        LmbrCentral::ShapeComponentRequestsBus::EventResult(
            inside, entity.GetId(), &LmbrCentral::ShapeComponentRequests::IsPointInside, AZ::Vector3(43.6f, 35.8f, 37.86f));

        EXPECT_TRUE(inside);
    }

    // variable radius and scale
    TEST_F(TubeShapeTest, IsPointInsideSuccess2)
    {
        AZ::Entity entity;
        CreateTube(
            AZ::Transform::CreateTranslation(AZ::Vector3(37.0f, 36.0f, 32.0f)) *
            AZ::Transform::CreateRotationZ(AZ::Constants::QuarterPi) *
            AZ::Transform::CreateRotationY(AZ::Constants::QuarterPi) *
            AZ::Transform::CreateScale(AZ::Vector3(0.8f, 1.5f, 1.5f)), 1.5f, entity);

        // set variable radius
        LmbrCentral::TubeShapeComponentRequestsBus::Event(
            entity.GetId(), &LmbrCentral::TubeShapeComponentRequestsBus::Events::SetVariableRadius, 0, 1.0f);
        LmbrCentral::TubeShapeComponentRequestsBus::Event(
            entity.GetId(), &LmbrCentral::TubeShapeComponentRequestsBus::Events::SetVariableRadius, 1, 0.2f);
        LmbrCentral::TubeShapeComponentRequestsBus::Event(
            entity.GetId(), &LmbrCentral::TubeShapeComponentRequestsBus::Events::SetVariableRadius, 2, 0.5f);
        LmbrCentral::TubeShapeComponentRequestsBus::Event(
            entity.GetId(), &LmbrCentral::TubeShapeComponentRequestsBus::Events::SetVariableRadius, 3, 2.0f);

        bool inside;
        LmbrCentral::ShapeComponentRequestsBus::EventResult(
            inside, entity.GetId(), &LmbrCentral::ShapeComponentRequests::IsPointInside, AZ::Vector3(37.6f, 36.76f, 34.0f));

        EXPECT_TRUE(inside);
    }

    // distance scaled - along length
    TEST_F(TubeShapeTest, DistanceFromPoint1)
    {
        AZ::Entity entity;
        CreateTube(
            AZ::Transform::CreateTranslation(AZ::Vector3(37.0f, 36.0f, 39.0f)) *
            AZ::Transform::CreateScale(AZ::Vector3(2.0f, 1.5f, 1.5f)), 1.5f, entity);

        LmbrCentral::TubeShapeComponentRequestsBus::Event(
            entity.GetId(), &LmbrCentral::TubeShapeComponentRequestsBus::Events::SetVariableRadius, 0, 1.0f);
        LmbrCentral::TubeShapeComponentRequestsBus::Event(
            entity.GetId(), &LmbrCentral::TubeShapeComponentRequestsBus::Events::SetVariableRadius, 1, 0.2f);
        LmbrCentral::TubeShapeComponentRequestsBus::Event(
            entity.GetId(), &LmbrCentral::TubeShapeComponentRequestsBus::Events::SetVariableRadius, 2, 0.5f);
        LmbrCentral::TubeShapeComponentRequestsBus::Event(
            entity.GetId(), &LmbrCentral::TubeShapeComponentRequestsBus::Events::SetVariableRadius, 3, 2.0f);

        float distance;
        LmbrCentral::ShapeComponentRequestsBus::EventResult(
            distance, entity.GetId(), &LmbrCentral::ShapeComponentRequests::DistanceFromPoint, AZ::Vector3(53.0f, 36.0f, 39.0f));

        EXPECT_NEAR(distance, 3.0f, 1e-2f);
    }

    // distance scaled - along length
    TEST_F(TubeShapeTest, DistanceFromPoint2)
    {
        AZ::Entity entity;
        CreateTube(
            AZ::Transform::CreateTranslation(AZ::Vector3(37.0f, 36.0f, 39.0f)) *
            AZ::Transform::CreateScale(AZ::Vector3(2.0f, 1.5f, 1.5f)), 1.5f, entity);

        LmbrCentral::TubeShapeComponentRequestsBus::Event(
            entity.GetId(), &LmbrCentral::TubeShapeComponentRequestsBus::Events::SetVariableRadius, 0, 1.0f);
        LmbrCentral::TubeShapeComponentRequestsBus::Event(
            entity.GetId(), &LmbrCentral::TubeShapeComponentRequestsBus::Events::SetVariableRadius, 1, 0.2f);
        LmbrCentral::TubeShapeComponentRequestsBus::Event(
            entity.GetId(), &LmbrCentral::TubeShapeComponentRequestsBus::Events::SetVariableRadius, 2, 0.5f);
        LmbrCentral::TubeShapeComponentRequestsBus::Event(
            entity.GetId(), &LmbrCentral::TubeShapeComponentRequestsBus::Events::SetVariableRadius, 3, 2.0f);

        float distance;
        LmbrCentral::ShapeComponentRequestsBus::EventResult(
            distance, entity.GetId(), &LmbrCentral::ShapeComponentRequests::DistanceFromPoint, AZ::Vector3(39.0f, 41.0f, 39.0f));

        EXPECT_NEAR(distance, 1.0f, 1e-2f);
    }

    TEST_F(TubeShapeTest, RadiiCannotBeNegativeFromVariableChange)
    {
        AZ::Entity entity;
        const float baseRadius = 1.0f;
        CreateTube(
            AZ::Transform::CreateTranslation(AZ::Vector3::CreateZero()), baseRadius, entity);

        // setting variable radius
        LmbrCentral::TubeShapeComponentRequestsBus::Event(
            entity.GetId(), &LmbrCentral::TubeShapeComponentRequestsBus::Events::SetVariableRadius, 0, -2.0f);

        float totalRadius = 0.0f;
        LmbrCentral::TubeShapeComponentRequestsBus::EventResult(
            totalRadius, entity.GetId(), &LmbrCentral::TubeShapeComponentRequestsBus::Events::GetTotalRadius,
            AZ::SplineAddress());

        float variableRadius = 0.0f;
        LmbrCentral::TubeShapeComponentRequestsBus::EventResult(
            variableRadius, entity.GetId(), &LmbrCentral::TubeShapeComponentRequestsBus::Events::GetVariableRadius,
            0);

        using ::testing::FloatEq;

        EXPECT_THAT(totalRadius, FloatEq(0.0f));
        EXPECT_THAT(variableRadius, FloatEq(-1.0f));
    }

    TEST_F(TubeShapeTest, RadiiCannotBeNegativeFromBaseChange)
    {
        AZ::Entity entity;
        const float baseRadius = 5.0f;
        CreateTube(
            AZ::Transform::CreateTranslation(AZ::Vector3::CreateZero()), baseRadius, entity);

        // setting variable radii
        LmbrCentral::TubeShapeComponentRequestsBus::Event(
            entity.GetId(), &LmbrCentral::TubeShapeComponentRequestsBus::Events::SetVariableRadius, 0, -2.0f);
        LmbrCentral::TubeShapeComponentRequestsBus::Event(
            entity.GetId(), &LmbrCentral::TubeShapeComponentRequestsBus::Events::SetVariableRadius, 1, -3.0f);
        LmbrCentral::TubeShapeComponentRequestsBus::Event(
            entity.GetId(), &LmbrCentral::TubeShapeComponentRequestsBus::Events::SetVariableRadius, 2, -4.0f);
        LmbrCentral::TubeShapeComponentRequestsBus::Event(
            entity.GetId(), &LmbrCentral::TubeShapeComponentRequestsBus::Events::SetVariableRadius, 3, -0.5f);

        // set base radius
        LmbrCentral::TubeShapeComponentRequestsBus::Event(
            entity.GetId(), &LmbrCentral::TubeShapeComponentRequestsBus::Events::SetRadius, 1.0f);

        // expected (clamped) values
        AZStd::pair<float, float> totalAndVariableRadii[] = {
            { 0.0f, -1.0f }, { 0.0f, -1.0f }, { 0.0f, -1.0f }, { 0.5f, -0.5f } };

        using ::testing::FloatEq;

        // verify all expected values for each vertex
        for (size_t vertIndex = 0; vertIndex < 4; ++vertIndex)
        {
            const auto radiis = totalAndVariableRadii[vertIndex];

            float totalRadius = 0.0f;
            LmbrCentral::TubeShapeComponentRequestsBus::EventResult(
                totalRadius, entity.GetId(), &LmbrCentral::TubeShapeComponentRequestsBus::Events::GetTotalRadius,
                AZ::SplineAddress(vertIndex));

            float variableRadius = 0.0f;
            LmbrCentral::TubeShapeComponentRequestsBus::EventResult(
                variableRadius, entity.GetId(), &LmbrCentral::TubeShapeComponentRequestsBus::Events::GetVariableRadius,
                vertIndex);

            EXPECT_THAT(totalRadius, FloatEq(radiis.first));
            EXPECT_THAT(variableRadius, FloatEq(radiis.second));
        }
    }
}