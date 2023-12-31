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

#include <AzCore/Component/EntityId.h>
#include <AzCore/EBus/EBus.h>

class QGraphicsLayout;
class QGraphicsLayoutItem;

namespace GraphCanvas
{
    static const AZ::Crc32 NodeLayoutServiceCrc = AZ_CRC("GraphCanvas_NodeLayoutService", 0x3dc121b7);
    static const AZ::Crc32 NodeSlotsServiceCrc = AZ_CRC("GraphCanvas_NodeSlotsService", 0x28f0a117);
    static const AZ::Crc32 NodeLayoutSupportServiceCrc = AZ_CRC("GraphCanvas_NodeLayoutSupportService", 0xa8b639be);

    //! NodeLayoutRequests
    //! Requests that are serviced by a node layout implementation.
    class NodeLayoutRequests : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = AZ::EntityId;

        
        //! Obtain the layout component as a \code QGraphicsLayout*. 
        virtual QGraphicsLayout* GetLayout() { return{}; }
    };

    using NodeLayoutRequestBus = AZ::EBus<NodeLayoutRequests>;

    //! NodeSlotRequestBus
    //! Used for making requests of a particular slot.
    class NodeSlotsRequests
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = AZ::EntityId;

        virtual QGraphicsLayoutItem* GetGraphicsLayoutItem() = 0;
    };

    using NodeSlotsRequestBus = AZ::EBus<NodeSlotsRequests>;
}