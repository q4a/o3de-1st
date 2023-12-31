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
#include <AzCore/std/string/string.h>

namespace AZ
{
    namespace Render
    {
        //! EditorMaterialSystemComponentRequests provides an interface to communicate with MaterialEditor
        class EditorMaterialSystemComponentRequests
            : public AZ::EBusTraits
        {
        public:
            // Only a single handler is allowed
            static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
            static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;

            //! Open document in material editor
            virtual void OpenInMaterialEditor(const AZStd::string& sourcePath) = 0;
        };
        using EditorMaterialSystemComponentRequestBus = AZ::EBus<EditorMaterialSystemComponentRequests>;
    } // namespace Render
} // namespace AZ
