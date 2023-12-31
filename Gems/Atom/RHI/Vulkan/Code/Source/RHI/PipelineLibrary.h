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

#include <Atom/RHI/PipelineLibrary.h>
#include <Atom/RHI.Reflect/PipelineLibraryData.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <RHI/GraphicsPipeline.h>
#include <RHI/Pipeline.h>

namespace AZ
{
    namespace Vulkan
    {
        class PipelineLibrary final
            : public RHI::PipelineLibrary
        {
            using Base = RHI::PipelineLibrary;

        public:
            AZ_CLASS_ALLOCATOR(PipelineLibrary, AZ::SystemAllocator, 0);
            AZ_RTTI(PipelineLibrary, "EB865D8F-7753-4E06-8401-310CC1CF2378", Base);

            static RHI::Ptr<PipelineLibrary> Create();

            VkPipelineCache GetNativePipelineCache() const;

        private:
            PipelineLibrary() = default;

            //////////////////////////////////////////////////////////////////////////
            // RHI::Object
            void SetNameInternal(const AZStd::string_view& name) override;
            //////////////////////////////////////////////////////////////////////////

            //////////////////////////////////////////////////////////////////////////
            // RHI::PipelineLibrary
            RHI::ResultCode InitInternal(RHI::Device& device, const RHI::PipelineLibraryData* serializedData) override;
            void ShutdownInternal() override;
            RHI::ResultCode MergeIntoInternal(AZStd::array_view<const RHI::PipelineLibrary*> libraries) override;
            RHI::ConstPtr<RHI::PipelineLibraryData> GetSerializedDataInternal() const override;
            //////////////////////////////////////////////////////////////////////////

            VkPipelineCache m_nativePipelineCache = VK_NULL_HANDLE;
        };
    }
}