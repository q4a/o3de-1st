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

#include <AzCore/EBus/EBus.h>

namespace AZ
{
    namespace Render
    {
        class AtomAutomationInterface
            : public EBusTraits
        {
        public:
            virtual ~AtomAutomationInterface() = default;

            //! Return type matches FindComponentTypeIds from EditorComponentAPIBus
            virtual AZStd::vector<AZ::Uuid> GetMeshComponentTypeId() = 0;
        };

        using AtomAutomationBus = EBus<AtomAutomationInterface>;

    } // namespace Render
} // namespace AZ
