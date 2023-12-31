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

#include <AzCore/Math/Transform.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzFramework/Viewport/CameraState.h>
#include <AzFramework/Viewport/ViewportScreen.h>
#include <AzTest/AzTest.h>
#include <AzToolsFramework/ViewportUi/ViewportUiWidgetCallbacks.h>
#include <QWidget>

namespace UnitTest
{
    class ViewportUiWidgetCallbacksTest : public AzToolsFramework::ViewportUi::Internal::ViewportUiWidgetCallbacks
    {
    public:
        AZStd::vector<QPointer<QObject>>& GetWidgets()
        {
            return m_widgets;
        }
        AZStd::unordered_map<QObject*, AZStd::function<void(QPointer<QObject>)>>& GetUpdateCallbacks()
        {
            return m_updateCallbacks;
        }
    };

    TEST(ViewportUiWidgetCallbacks, AddWidgetAddsToInternalVector)
    {
        ViewportUiWidgetCallbacksTest testWidgetManager;

        EXPECT_EQ(testWidgetManager.GetWidgets().size(), 0);

        QObject* mockObject = new QWidget();
        testWidgetManager.AddWidget(mockObject);

        EXPECT_EQ(testWidgetManager.GetWidgets().size(), 1);
        EXPECT_EQ(testWidgetManager.GetWidgets()[0], mockObject);
    }

    TEST(ViewportUiWidgetCallbacks, AddWidgetDoesNotAddIfWidgetIsNull)
    {
        ViewportUiWidgetCallbacksTest testWidgetManager;

        EXPECT_EQ(testWidgetManager.GetWidgets().size(), 0);

        QObject* mockObject = nullptr;
        testWidgetManager.AddWidget(mockObject);

        EXPECT_EQ(testWidgetManager.GetWidgets().size(), 0);
    }

    TEST(ViewportUiWidgetCallbacks, RemoveWidgetRemovesFromInternalVector)
    {
        ViewportUiWidgetCallbacksTest testWidgetManager;
        QObject* mockObject = new QWidget();
        testWidgetManager.AddWidget(mockObject);

        EXPECT_EQ(testWidgetManager.GetWidgets().size(), 1);

        testWidgetManager.RemoveWidget(mockObject);

        EXPECT_EQ(testWidgetManager.GetWidgets().size(), 0);
    }

    TEST(ViewportUiWidgetCallbacks, RegisterUpdateCallbackStoresCallbackFunction)
    {
        ViewportUiWidgetCallbacksTest testWidgetManager;
        QObject* mockObject = new QWidget();
        testWidgetManager.AddWidget(mockObject);

        EXPECT_EQ(testWidgetManager.GetUpdateCallbacks().size(), 0);

        auto mockFn = []([[maybe_unused]] QObject* object)
        {
            return;
        };

        testWidgetManager.RegisterUpdateCallback(mockObject, mockFn);

        EXPECT_EQ(testWidgetManager.GetUpdateCallbacks().size(), 1);
    }

    class ViewportUiWidgetAssertFixture
        : public ::testing::Test
        , UnitTest::TraceBusRedirector
    {
    public:
        void SetUp() override
        {
            AZ::Debug::TraceMessageBus::Handler::BusConnect();
        }
        void TearDown() override
        {
            AZ::Debug::TraceMessageBus::Handler::BusDisconnect();
        }
    };

    TEST_F(ViewportUiWidgetAssertFixture, RegisterUpdateCallbackDoesNotRegisterFunctionForNotAddedObject)
    {
        ViewportUiWidgetCallbacksTest testWidgetManager;
        QObject* mockObject = new QWidget();

        auto mockFn = []([[maybe_unused]] QObject* object)
        {
            return;
        };

        AZ_TEST_START_TRACE_SUPPRESSION;
        testWidgetManager.RegisterUpdateCallback(mockObject, mockFn);
        AZ_TEST_STOP_TRACE_SUPPRESSION(1);

        EXPECT_EQ(testWidgetManager.GetUpdateCallbacks().size(), 0);
    }

    TEST(ViewportUiWidgetCallbacks, UpdateCallsCallbackFunction)
    {
        ViewportUiWidgetCallbacksTest testWidgetManager;
        QWidget* mockObject = new QWidget();
        mockObject->setVisible(true);
        testWidgetManager.AddWidget(mockObject);

        auto mockFn = [](QObject* object)
        {
            static_cast<QWidget*>(object)->setVisible(false);
        };

        testWidgetManager.RegisterUpdateCallback(mockObject, mockFn);
        testWidgetManager.Update();

        EXPECT_FALSE(mockObject->isVisible());
    }

    TEST(ViewportUiWidgetCallbacks, UpdateRemovesDeletedObjects)
    {
        ViewportUiWidgetCallbacksTest testWidgetManager;
        QWidget* mockObject = new QWidget();
        mockObject->setVisible(true);
        testWidgetManager.AddWidget(mockObject);

        auto mockFn = []([[maybe_unused]] QObject* object)
        {
            return;
        };

        testWidgetManager.RegisterUpdateCallback(mockObject, mockFn);

        EXPECT_EQ(testWidgetManager.GetWidgets().size(), 1);
        EXPECT_EQ(testWidgetManager.GetUpdateCallbacks().size(), 1);

        delete mockObject;
        testWidgetManager.Update();

        EXPECT_EQ(testWidgetManager.GetWidgets().size(), 0);
    }
} // namespace UnitTest
