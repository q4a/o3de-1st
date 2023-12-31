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

#include <QGraphicsLinearLayout>

#include <AzCore/Component/Component.h>

#include <Components/Nodes/NodeLayoutComponent.h>
#include <GraphCanvas/Components/Nodes/NodeConfiguration.h>
#include <GraphCanvas/Components/Nodes/NodeLayoutBus.h>
#include <GraphCanvas/Components/StyleBus.h>
#include <GraphCanvas/Styling/StyleHelper.h>

class QGraphicsGridLayout;

namespace GraphCanvas
{
    //! Lays out the parts of the generic Node
    class GeneralNodeLayoutComponent
        : public NodeLayoutComponent
        , public NodeNotificationBus::Handler
        , public StyleNotificationBus::Handler
    {
    public:
        AZ_COMPONENT(GeneralNodeLayoutComponent, "{2AD34925-FF0E-4D0D-A371-6338FBAE0F43}", NodeLayoutComponent);
        static void Reflect(AZ::ReflectContext*);
        
        static AZ::Entity* CreateGeneralNodeEntity(const char* nodeType, const NodeConfiguration& nodeConfiguration = NodeConfiguration());

        GeneralNodeLayoutComponent();
        ~GeneralNodeLayoutComponent();

        // AZ::Component
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
        {
            dependent.push_back(NodeLayoutSupportServiceCrc);
            dependent.push_back(AZ_CRC("GraphCanvas_TitleService", 0xfe6d63bc));
            dependent.push_back(AZ_CRC("GraphCanvas_SlotsContainerService", 0x948b6696));
        }

        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
        {
            required.push_back(AZ_CRC("GraphCanvas_NodeService", 0xcc0f32cc));
            required.push_back(AZ_CRC("GraphCanvas_StyledGraphicItemService", 0xeae4cdf4));
        }
        
        void Init() override;
        void Activate() override;
        void Deactivate() override;
        ////    

        // StyleNotificationBus
        void OnStyleChanged() override;
        ////

        // NodeNotificationBus
        void OnNodeActivated() override;
        ////
        
    protected:

        void UpdateLayoutParameters();

        QGraphicsLinearLayout* m_title;
        QGraphicsLinearLayout* m_slots;
    };
}