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

#include <Atom/RHI.Reflect/AttachmentId.h>
#include <Atom/RHI.Reflect/Handle.h>

#include <Atom/RPI.Reflect/Pass/PassAttachmentReflect.h>
#include <Atom/RPI.Reflect/Pass/PassData.h>

#include <AtomCore/std/containers/array_view.h>

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/containers/array.h>

namespace AZ
{
    namespace RPI
    {
        using SlotIndex = RHI::Handle<uint32_t>;

        //! This class represents a request for a Pass to be instantiated from a PassTemplate
        //! It also contains a list of inputs for the instantiated pass
        struct PassRequest final
        {
            AZ_TYPE_INFO(PassRequest, "{C43802D1-8501-4D7A-B642-85F8646DF46D}");
            AZ_CLASS_ALLOCATOR(PassRequest, SystemAllocator, 0);
            static void Reflect(ReflectContext* context);

            PassRequest() = default;
            ~PassRequest() = default;

            //! Add a pass connection to the list of input connections
            void AddInputConnection(PassConnection inputConnection);

            //! Name of the pass this request will instantiate
            Name m_passName;

            //! Name of the template from which the pass will be created
            Name m_templateName;

            //! Names of Passes that this Pass should execute after
            AZStd::vector<Name> m_executeAfterPasses;

            //! Names of Passes that this Pass shoudl execute before
            AZStd::vector<Name> m_executeBeforePasses;

            //! The attachments to be used as inputs to the instantiated Pass
            PassConnectionList m_inputConnections;

            //! Optional data to be used during pass initialization
            AZStd::shared_ptr<PassData> m_passData = nullptr;

            //! Initial state of the pass when created (enabled/disabled)
            bool m_passEnabled = true;
        };

        using PassRequestList = AZStd::vector<PassRequest>;
        using PassRequestListView = AZStd::array_view<PassRequest>;

    }   // namespace RPI
}   // namespace AZ
