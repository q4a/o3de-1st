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

#include <Atom/RHI/Device.h>
#include <Atom/RHI/Factory.h>
#include <Atom/RHI/RHISystem.h>

#include <AzCore/Interface/Interface.h>

#include <AzFramework/API/ApplicationAPI.h>
#include <AzFramework/CommandLine/CommandLine.h>
#include <Atom/RHI.Reflect/PlatformLimitsDescriptor.h>

namespace AZ
{
    namespace RHI
    {
        RHISystemInterface* RHISystemInterface::Get()
        {
            return Interface<RHISystemInterface>::Get();
        }

        void RHISystem::InitDevice()
        {
            m_device = InitInternalDevice();
            Interface<RHISystemInterface>::Register(this);
        }
    
        void RHISystem::Init(const RHISystemDescriptor& descriptor)
        {
            m_cpuProfiler.Init();

            RHI::FrameSchedulerDescriptor frameSchedulerDescriptor;
            if (descriptor.m_platformLimits)
            {
                m_platformLimitsDescriptor = descriptor.m_platformLimits->m_platformLimitsDescriptor;
            }

            //If platformlimits.azasset file is not provided create an object with default config values.
            if (!m_platformLimitsDescriptor)
            {
                m_platformLimitsDescriptor = PlatformLimitsDescriptor::Create();
            }

            RHI::DeviceDescriptor deviceDesc;
            deviceDesc.m_platformLimitsDescriptor = m_platformLimitsDescriptor;
            if (m_device->PostInit(deviceDesc) != RHI::ResultCode::Success)
            {
                AZ_Assert(false, "RHISystem", "Unable to initialize RHI! \n");
                return;
            }
            
            m_drawListTagRegistry = RHI::DrawListTagRegistry::Create();
            m_pipelineStateCache = RHI::PipelineStateCache::Create(*m_device);

            frameSchedulerDescriptor.m_transientAttachmentPoolDescriptor.m_renderTargetBudgetInBytes = m_platformLimitsDescriptor->m_transientAttachmentPoolBudgets.m_renderTargetBudgetInBytes;
            frameSchedulerDescriptor.m_transientAttachmentPoolDescriptor.m_imageBudgetInBytes = m_platformLimitsDescriptor->m_transientAttachmentPoolBudgets.m_imageBudgetInBytes;
            frameSchedulerDescriptor.m_transientAttachmentPoolDescriptor.m_bufferBudgetInBytes = m_platformLimitsDescriptor->m_transientAttachmentPoolBudgets.m_bufferBudgetInBytes;

            switch (m_platformLimitsDescriptor->m_heapAllocationStrategy)
            {
                case HeapAllocationStrategy::Fixed:
                {
                    frameSchedulerDescriptor.m_transientAttachmentPoolDescriptor.m_heapParameters = RHI::HeapAllocationParameters();
                    break;
                }
                case  HeapAllocationStrategy::Paging:
                {
                    RHI::HeapPagingParameters heapAllocationParameters;
                    heapAllocationParameters.m_collectLatency = m_platformLimitsDescriptor->m_pagingParameters.m_collectLatency;
                    heapAllocationParameters.m_initialAllocationPercentage = m_platformLimitsDescriptor->m_pagingParameters.m_initialAllocationPercentage;
                    heapAllocationParameters.m_pageSizeInBytes = m_platformLimitsDescriptor->m_pagingParameters.m_pageSizeInBytes;
                    frameSchedulerDescriptor.m_transientAttachmentPoolDescriptor.m_heapParameters = RHI::HeapAllocationParameters(heapAllocationParameters);
                    break;
                }
                case HeapAllocationStrategy::MemoryHint:
                {
                    RHI::HeapMemoryHintParameters heapAllocationParameters;
                    heapAllocationParameters.m_heapSizeScaleFactor = m_platformLimitsDescriptor->m_usageHintParameters.m_heapSizeScaleFactor;
                    heapAllocationParameters.m_collectLatency = m_platformLimitsDescriptor->m_usageHintParameters.m_collectLatency;
                    heapAllocationParameters.m_maxHeapWastedPercentage = m_platformLimitsDescriptor->m_usageHintParameters.m_maxHeapWastedPercentage;
                    heapAllocationParameters.m_minHeapSizeInBytes = m_platformLimitsDescriptor->m_usageHintParameters.m_minHeapSizeInBytes;
                    frameSchedulerDescriptor.m_transientAttachmentPoolDescriptor.m_heapParameters = RHI::HeapAllocationParameters(heapAllocationParameters);
                    break;
                }
                default:
                {
                    AZ_Assert(false, "UnSupported type");
                    break;
                }
            }
                
            frameSchedulerDescriptor.m_platformLimitsDescriptor = m_platformLimitsDescriptor;
            m_frameScheduler.Init(*m_device, frameSchedulerDescriptor);

            // Register draw list tags declared from content.
            for (const Name& drawListName : descriptor.m_drawListTags)
            {
                RHI::DrawListTag drawListTag = m_drawListTagRegistry->AcquireTag(drawListName);

                AZ_Warning("RHISystem", drawListTag.IsValid(), "Failed to register draw list tag '%s'. Registry at capacity.", drawListName.GetCStr());
            }
        }

        RHI::Ptr<RHI::Device> RHISystem::InitInternalDevice()
        {
            RHI::PhysicalDeviceList physicalDevices = RHI::Factory::Get().EnumeratePhysicalDevices();

            AZ_Printf("RHISystem", "Initializing RHI...\n");

            if (physicalDevices.empty())
            {
                AZ_Printf("RHISystem", "Unable to initialize RHI! No supported physical device found.\n");
                return nullptr;
            }

            // Command line set up to search for forcing adapter argument
            const AzFramework::CommandLine* commandLine = nullptr;
            AzFramework::ApplicationRequests::Bus::BroadcastResult(commandLine, &AzFramework::ApplicationRequests::GetApplicationCommandLine);

            AZStd::string preferredUserAdapterName;
            if (commandLine)
            {
                preferredUserAdapterName = commandLine->GetSwitchValue("forceAdapter", 0);
            }

            RHI::PhysicalDevice* preferredUserDevice{};
            RHI::PhysicalDevice* preferredVendorDevice{};

            for (RHI::Ptr<RHI::PhysicalDevice>& physicalDevice : physicalDevices)
            {
                const RHI::PhysicalDeviceDescriptor& descriptor = physicalDevice->GetDescriptor();

                AZ_Printf("RHISystem", "\tEnumerated physical device: %s\n", descriptor.m_description.c_str());

                if (!preferredUserDevice && descriptor.m_description == preferredUserAdapterName)
                {
                    preferredUserDevice = physicalDevice.get();
                }

                // Record the first nVidia or AMD device we find.
                if (!preferredVendorDevice && (descriptor.m_vendorId == RHI::VendorId::AMD || descriptor.m_vendorId == RHI::VendorId::nVidia))
                {
                    preferredVendorDevice = physicalDevice.get();
                }
            }

            AZ_Warning("RHISystem", preferredUserAdapterName.empty() || preferredUserDevice, "Specified adapter name not found: '%s'", preferredUserAdapterName.c_str());

            RHI::PhysicalDevice* physicalDeviceFound{};

            if (preferredUserDevice)
            {
                // First, prefer the user specified device if found.
                physicalDeviceFound = preferredUserDevice;
            }
            else if (preferredVendorDevice)
            {
                // Second, prefer specific vendor devices.
                physicalDeviceFound = preferredVendorDevice;
            }
            else
            {
                // Default to first device if no other preferred device is found.
                physicalDeviceFound = physicalDevices.front().get();
            }

            AZ_Printf("RHISystem", "\tUsing physical device: %s\n", physicalDeviceFound->GetDescriptor().m_description.c_str());

            RHI::Ptr<RHI::Device> device = RHI::Factory::Get().CreateDevice();
            if (device->Init(*physicalDeviceFound) == RHI::ResultCode::Success)
            {
                return device;
            }

            AZ_Error("RHISystem", false, "Failed to initialize RHI device.");
            return nullptr;
        }

        void RHISystem::Shutdown()
        {
            Interface<RHISystemInterface>::Unregister(this);
            m_frameScheduler.Shutdown();

            m_platformLimitsDescriptor = nullptr;
            m_drawListTagRegistry = nullptr;
            m_pipelineStateCache = nullptr;
            m_device->PreShutdown();
            AZ_Assert(m_device->use_count()==1, "The ref count for Device is %i but it should be 1 here to ensure all the resources are released", m_device->use_count());
            m_device = nullptr;

            m_cpuProfiler.Shutdown();
        }

        void RHISystem::FrameUpdate(FrameGraphCallback frameGraphCallback)
        {
            AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzRender);

            {
                AZ_PROFILE_SCOPE(AZ::Debug::ProfileCategory::AzRender, "main per-frame work");
                m_frameScheduler.BeginFrame();

                frameGraphCallback(m_frameScheduler);

                /**
                 * This exists as a hook to enable RHI sample tests, which are allowed to queue their
                 * own RHI scopes to the frame scheduler. This happens prior to the RPI pass graph registration.
                 */
                RHISystemNotificationBus::Broadcast(&RHISystemNotificationBus::Events::OnFramePrepare, m_frameScheduler);
            
                RHI::MessageOutcome outcome = m_frameScheduler.Compile(m_compileRequest);
                if (outcome.IsSuccess())
                {
                    m_frameScheduler.Execute(RHI::JobPolicy::Parallel);
                }
                else
                {
                    AZ_Error("RHISystem", false, "Frame Scheduler Compilation Failure: %s", outcome.GetError().c_str());
                }

                m_pipelineStateCache->Compact();
            }

            m_frameScheduler.EndFrame();
        }

        RHI::Device* RHISystem::GetDevice()
        {
            return m_device.get();
        }

        RHI::PipelineStateCache* RHISystem::GetPipelineStateCache()
        {
            return m_pipelineStateCache.get();
        }

        RHI::DrawListTagRegistry* RHISystem::GetDrawListTagRegistry()
        {
            return m_drawListTagRegistry.get();
        }

        const RHI::FrameSchedulerCompileRequest& RHISystem::GetFrameSchedulerCompileRequest() const
        {
            return m_compileRequest;
        }

        void RHISystem::ModifyFrameSchedulerStatisticsFlags(RHI::FrameSchedulerStatisticsFlags statisticsFlags, bool enableFlags)
        {
            m_compileRequest.m_statisticsFlags =
                enableFlags
                ? RHI::SetBits(m_compileRequest.m_statisticsFlags, statisticsFlags)
                : RHI::ResetBits(m_compileRequest.m_statisticsFlags, statisticsFlags);
        }

        const RHI::CpuTimingStatistics* RHISystem::GetCpuTimingStatistics() const
        {
            return m_frameScheduler.GetCpuTimingStatistics();
        }

        const RHI::TransientAttachmentStatistics* RHISystem::GetTransientAttachmentStatistics() const
        {
            return m_frameScheduler.GetTransientAttachmentStatistics();
        }

        const AZ::RHI::TransientAttachmentPoolDescriptor* RHISystem::GetTransientAttachmentPoolDescriptor() const
        {
            return m_frameScheduler.GetTransientAttachmentPoolDescriptor();
        }

        ConstPtr<PlatformLimitsDescriptor> RHISystem::GetPlatformLimitsDescriptor() const
        {
            return m_platformLimitsDescriptor;
        }

    } //namespace RPI
} //namespace AZ
