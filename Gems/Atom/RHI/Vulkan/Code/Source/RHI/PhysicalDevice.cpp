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
#include "Atom_RHI_Vulkan_precompiled.h"
#include <Atom/RHI/MemoryStatisticsBuilder.h>
#include <AzCore/std/containers/set.h>
#include <AzCore/std/string/conversions.h>
#include <RHI/Conversion.h>
#include <RHI/Instance.h>
#include <RHI/PhysicalDevice.h>
#include <Vulkan_Traits_Platform.h>

namespace AZ
{
    namespace Vulkan
    {
        static constexpr size_t MinGPUMemSize = AZ_TRAIT_ATOM_VULKAN_MIN_GPU_MEM;

        RHI::PhysicalDeviceList PhysicalDevice::Enumerate()
        {
            RHI::PhysicalDeviceList physicalDeviceList;
            VkResult result = VK_SUCCESS;

            VkInstance instance = Instance::GetInstance().GetNativeInstance();

            uint32_t physicalDeviceCount = 0;
            result = vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, nullptr);
            AssertSuccess(result);
            if (physicalDeviceCount == 0)
            {
                AZ_Error("Vulkan", false, "No Vulkan compatible physical devices were found!");
                return physicalDeviceList;
            }

            AZStd::vector<VkPhysicalDevice> physicalDevices;
            physicalDevices.resize(physicalDeviceCount);

            result = vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, physicalDevices.data());
            AssertSuccess(result);

            if (ConvertResult(result) != RHI::ResultCode::Success)
            {
                AZ_Error("Vulkan", false, GetResultString(result));
                return physicalDeviceList;
            }

            if (physicalDevices.empty())
            {
                AZ_Error("Vulkan", false, "No suitable Vulkan devices were found!");
            }

            physicalDeviceList.reserve(physicalDevices.size());
            for (const VkPhysicalDevice& device : physicalDevices)
            {
                RHI::Ptr<PhysicalDevice> physicalDevice = aznew PhysicalDevice;
                physicalDevice->Init(device);
                size_t gpuMemSize =  physicalDevice->GetDescriptor().m_heapSizePerLevel[static_cast<size_t>(RHI::HeapMemoryLevel::Device)];
                AZ_Warning("Vulkan", gpuMemSize >= MinGPUMemSize, "Rejecting GPU %s as it's gpu mem size of %zu bytes is less than min required size of %zu bytes for Vulkan API", physicalDevice->GetDescriptor().m_description.c_str(), gpuMemSize, MinGPUMemSize);
                if (gpuMemSize >= MinGPUMemSize)
                {
                    physicalDeviceList.emplace_back(physicalDevice);
                }
            }

            return physicalDeviceList;
        }

        const VkPhysicalDevice& PhysicalDevice::GetNativePhysicalDevice() const
        {
            return m_vkPhysicalDevice;
        }

        const VkPhysicalDeviceMemoryProperties& PhysicalDevice::GetMemoryProperties() const
        {
            return m_memoryProperty;
        }

        const VkPhysicalDeviceLimits& PhysicalDevice::GetDeviceLimits() const
        {
            return m_deviceProperties.limits;
        }

        const VkPhysicalDeviceFeatures& PhysicalDevice::GetPhysicalDeviceFeatures() const
        {
            return m_deviceFeatures;
        }

        const VkPhysicalDeviceProperties& PhysicalDevice::GetPhysicalDeviceProperties() const
        {
            return m_deviceProperties;
        }

        const VkPhysicalDeviceConservativeRasterizationPropertiesEXT& PhysicalDevice::GetPhysicalDeviceConservativeRasterProperties() const
        {
            return m_conservativeRasterProperties;
        }

        const VkPhysicalDeviceDepthClipEnableFeaturesEXT& PhysicalDevice::GetPhysicalDeviceDepthClipEnableFeatures() const
        {
            return m_dephClipEnableFeatures;
        }

        const VkPhysicalDeviceRobustness2FeaturesEXT& PhysicalDevice::GetPhysicalDeviceRobutness2Features() const
        {
            return m_robutness2Features;
        }

        const VkPhysicalDeviceSeparateDepthStencilLayoutsFeaturesKHR& PhysicalDevice::GetPhysicalDeviceSeparateDepthStencilFeatures() const
        {
            return m_separateDepthStencilFeatures;
        }

        const VkPhysicalDeviceShaderFloat16Int8FeaturesKHR& PhysicalDevice::GetPhysicalDeviceFloat16Int8Features() const
        {
            return m_float16Int8Features;
        }

        const VkPhysicalDeviceVulkan12Features& PhysicalDevice::GetPhysicalDeviceVulkan12Features() const
        {
            return m_vulkan12Features;
        }

        VkFormatProperties PhysicalDevice::GetFormatProperties(RHI::Format format, bool raiseAsserts) const
        {
            VkFormatProperties formatProperties{};
            VkFormat vkFormat = ConvertFormat(format, raiseAsserts);
            if (vkFormat != VK_FORMAT_UNDEFINED)
            {
                vkGetPhysicalDeviceFormatProperties(GetNativePhysicalDevice(), vkFormat, &formatProperties);
            }
            return formatProperties;
        }

        StringList PhysicalDevice::GetDeviceLayerNames() const
        {
            StringList layerNames;
            uint32_t layerPropertyCount = 0;
            VkResult result = vkEnumerateDeviceLayerProperties(m_vkPhysicalDevice, &layerPropertyCount, nullptr);
            if (IsError(result) || layerPropertyCount == 0)
            {
                return layerNames;
            }

            AZStd::vector<VkLayerProperties> layerProperties(layerPropertyCount);
            result = vkEnumerateDeviceLayerProperties(m_vkPhysicalDevice, &layerPropertyCount, layerProperties.data());
            if (IsError(result))
            {
                return layerNames;
            }

            layerNames.reserve(layerNames.size() + layerProperties.size());
            for (uint32_t layerPropertyIndex = 0; layerPropertyIndex < layerPropertyCount; ++layerPropertyIndex)
            {
                layerNames.emplace_back(layerProperties[layerPropertyIndex].layerName);
            }

            return layerNames;
        }

        StringList PhysicalDevice::GetDeviceExtensionNames(const char* layerName /*=nullptr*/) const
        {
            StringList extensionNames;
            uint32_t extPropertyCount = 0;
            VkResult result = vkEnumerateDeviceExtensionProperties(m_vkPhysicalDevice, layerName, &extPropertyCount, nullptr);
            if (IsError(result) || extPropertyCount == 0)
            {
                return extensionNames;
            }

            AZStd::vector<VkExtensionProperties> extProperties;
            extProperties.resize(extPropertyCount);

            result = vkEnumerateDeviceExtensionProperties(m_vkPhysicalDevice, layerName, &extPropertyCount, extProperties.data());
            if (IsError(result))
            {
                return extensionNames;
            }

            extensionNames.reserve(extensionNames.size() + extProperties.size());
            for (uint32_t extPropertyIndex = 0; extPropertyIndex < extPropertyCount; extPropertyIndex++)
            {
                extensionNames.emplace_back(extProperties[extPropertyIndex].extensionName);
            }
            return extensionNames;
        }

        bool PhysicalDevice::IsFormatSupported(RHI::Format format, VkImageTiling tiling, VkFormatFeatureFlags features) const
        {
            VkFormatProperties properties = GetFormatProperties(format);
            switch (tiling)
            {
            case VK_IMAGE_TILING_OPTIMAL:
                return RHI::CheckBitsAll(properties.optimalTilingFeatures, features);
            case VK_IMAGE_TILING_LINEAR:
                return RHI::CheckBitsAll(properties.linearTilingFeatures, features);
            default:
                AZ_Assert(false, "Invalid image tiling type %d", tiling);
                return false;
            }
        }

        void PhysicalDevice::LoadSupportedFeatures()
        {
            uint32_t majorVersion = VK_VERSION_MAJOR(m_deviceProperties.apiVersion);
            uint32_t minorVersion = VK_VERSION_MINOR(m_deviceProperties.apiVersion);

            m_features.reset();
            m_features.set(static_cast<size_t>(DeviceFeature::Compatible2dArrayTexture), (majorVersion >= 1 && minorVersion >= 1) || VK_DEVICE_EXTENSION_SUPPORTED(KHR_maintenance1));
            m_features.set(static_cast<size_t>(DeviceFeature::CustomSampleLocation), VK_DEVICE_EXTENSION_SUPPORTED(EXT_sample_locations));
            m_features.set(static_cast<size_t>(DeviceFeature::Predication), VK_DEVICE_EXTENSION_SUPPORTED(EXT_conditional_rendering));
            m_features.set(static_cast<size_t>(DeviceFeature::ConservativeRaster), VK_DEVICE_EXTENSION_SUPPORTED(EXT_conservative_rasterization));
            m_features.set(static_cast<size_t>(DeviceFeature::DepthClipEnable), VK_DEVICE_EXTENSION_SUPPORTED(EXT_depth_clip_enable) && m_dephClipEnableFeatures.depthClipEnable);
            m_features.set(static_cast<size_t>(DeviceFeature::DrawIndirectCount), (majorVersion >= 1 && minorVersion >= 2 && m_vulkan12Features.drawIndirectCount) || VK_DEVICE_EXTENSION_SUPPORTED(KHR_draw_indirect_count));
            m_features.set(static_cast<size_t>(DeviceFeature::NullDescriptor), m_robutness2Features.nullDescriptor && VK_DEVICE_EXTENSION_SUPPORTED(EXT_robustness2));
            m_features.set(static_cast<size_t>(DeviceFeature::SeparateDepthStencil),
                (m_separateDepthStencilFeatures.separateDepthStencilLayouts && VK_DEVICE_EXTENSION_SUPPORTED(KHR_separate_depth_stencil_layouts)) ||
                (m_vulkan12Features.separateDepthStencilLayouts));
        }

        void PhysicalDevice::CompileMemoryStatistics(RHI::MemoryStatisticsBuilder& builder) const
        {
            if (VK_DEVICE_EXTENSION_SUPPORTED(KHR_get_physical_device_properties2) && VK_DEVICE_EXTENSION_SUPPORTED(EXT_memory_budget))
            {
                VkPhysicalDeviceMemoryBudgetPropertiesEXT budget = {};
                budget.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_BUDGET_PROPERTIES_EXT;

                VkPhysicalDeviceMemoryProperties2 properties = {};
                properties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_PROPERTIES_2;
                properties.pNext = &budget;
                vkGetPhysicalDeviceMemoryProperties2KHR(m_vkPhysicalDevice, &properties);

                for (uint32_t i = 0; i < properties.memoryProperties.memoryHeapCount; ++i)
                {
                    RHI::MemoryStatistics::Heap* heapStats = builder.AddHeap();
                    heapStats->m_name = AZStd::string::format("Heap %d", static_cast<int>(i));
                    heapStats->m_heapMemoryType = RHI::CheckBitsAll(properties.memoryProperties.memoryHeaps[i].flags, static_cast<VkMemoryHeapFlags>(VK_MEMORY_HEAP_DEVICE_LOCAL_BIT)) ? RHI::HeapMemoryLevel::Device : RHI::HeapMemoryLevel::Host;
                    heapStats->m_memoryUsage.m_budgetInBytes = budget.heapBudget[i];
                    heapStats->m_memoryUsage.m_reservedInBytes = 0;
                    heapStats->m_memoryUsage.m_residentInBytes = budget.heapUsage[i];
                }
            }
        }

        void PhysicalDevice::Init(VkPhysicalDevice vkPhysicalDevice)
        {
            m_vkPhysicalDevice = vkPhysicalDevice;

            if (VK_INSTANCE_EXTENSION_SUPPORTED(KHR_get_physical_device_properties2))
            {
                VkPhysicalDeviceDepthClipEnableFeaturesEXT& dephClipEnableFeatures = m_dephClipEnableFeatures;
                dephClipEnableFeatures = {};
                dephClipEnableFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DEPTH_CLIP_ENABLE_FEATURES_EXT;

                VkPhysicalDeviceRobustness2FeaturesEXT& robustness2Feature = m_robutness2Features;
                robustness2Feature = {};
                robustness2Feature.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ROBUSTNESS_2_FEATURES_EXT;
                dephClipEnableFeatures.pNext = &robustness2Feature;

                VkPhysicalDeviceShaderFloat16Int8FeaturesKHR& float16Int8Features = m_float16Int8Features;
                float16Int8Features = {};
                float16Int8Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_FLOAT16_INT8_FEATURES_KHR;
                robustness2Feature.pNext = &float16Int8Features;

                VkPhysicalDeviceSeparateDepthStencilLayoutsFeaturesKHR& separateDepthStencilFeatures = m_separateDepthStencilFeatures;
                separateDepthStencilFeatures = {};
                separateDepthStencilFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SEPARATE_DEPTH_STENCIL_LAYOUTS_FEATURES;
                float16Int8Features.pNext = &separateDepthStencilFeatures;

                VkPhysicalDeviceVulkan12Features& vulkan12Features = m_vulkan12Features;
                vulkan12Features = {};
                vulkan12Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
                separateDepthStencilFeatures.pNext = &vulkan12Features;

                VkPhysicalDeviceFeatures2 deviceFeatures2 = {};
                deviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
                deviceFeatures2.pNext = &dephClipEnableFeatures;

                vkGetPhysicalDeviceFeatures2KHR(vkPhysicalDevice, &deviceFeatures2);
                m_deviceFeatures = deviceFeatures2.features;

                VkPhysicalDeviceProperties2 deviceProps2 = {};
                m_conservativeRasterProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_CONSERVATIVE_RASTERIZATION_PROPERTIES_EXT;
                deviceProps2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2_KHR;
                deviceProps2.pNext = &m_conservativeRasterProperties;
                vkGetPhysicalDeviceProperties2KHR(vkPhysicalDevice, &deviceProps2);
                m_deviceProperties = deviceProps2.properties;
            }
            else
            {
                vkGetPhysicalDeviceFeatures(vkPhysicalDevice, &m_deviceFeatures);
                vkGetPhysicalDeviceProperties(vkPhysicalDevice, &m_deviceProperties);
            }

            m_descriptor.m_description = m_deviceProperties.deviceName;

            switch (m_deviceProperties.deviceType)
            {
            case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
                m_descriptor.m_type = RHI::PhysicalDeviceType::GpuDiscrete;
                break;
            case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
                m_descriptor.m_type = RHI::PhysicalDeviceType::GpuIntegrated;
                break;
            case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
                m_descriptor.m_type = RHI::PhysicalDeviceType::GpuVirtual;
                break;
            case VK_PHYSICAL_DEVICE_TYPE_CPU:
                m_descriptor.m_type = RHI::PhysicalDeviceType::Cpu;
                break;
            default:
                m_descriptor.m_type = RHI::PhysicalDeviceType::Unknown;
                break;
            }

            m_descriptor.m_vendorId = m_deviceProperties.vendorID;
            m_descriptor.m_deviceId = m_deviceProperties.deviceID;
            m_descriptor.m_driverVersion = m_deviceProperties.driverVersion;

            vkGetPhysicalDeviceMemoryProperties(vkPhysicalDevice, &m_memoryProperty);

            AZStd::set<uint32_t> heapIndicesDevice;
            AZStd::set<uint32_t> heapIndicesHost;
            for (uint32_t typeIndex = 0; typeIndex < m_memoryProperty.memoryTypeCount; ++typeIndex)
            {
                const VkMemoryPropertyFlags propertyFlags = m_memoryProperty.memoryTypes[typeIndex].propertyFlags;
                if (RHI::CheckBitsAny(propertyFlags, static_cast<VkMemoryPropertyFlags>(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)))
                {
                    const uint32_t heapIndex = m_memoryProperty.memoryTypes[typeIndex].heapIndex;
                    AZ_Assert(heapIndex < m_memoryProperty.memoryHeapCount, "Heap Index is wrong.");
                    heapIndicesDevice.emplace(heapIndex);
                }
                else if (RHI::CheckBitsAny(propertyFlags, static_cast<VkMemoryPropertyFlags>(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)))
                {
                    const uint32_t hidx = m_memoryProperty.memoryTypes[typeIndex].heapIndex;
                    AZ_Assert(hidx < m_memoryProperty.memoryHeapCount, "Heap Index is wrong.");
                    heapIndicesHost.emplace(hidx);
                }
            }

            VkDeviceSize memsize_device = 0;
            for (uint32_t heapIndex : heapIndicesDevice)
            {
                const VkMemoryHeap& heap = m_memoryProperty.memoryHeaps[heapIndex];
                AZ_Assert(RHI::CheckBitsAny(heap.flags, static_cast<VkMemoryHeapFlags>(VK_MEMORY_HEAP_DEVICE_LOCAL_BIT)), "Device local heap does not have device local bit.");
                memsize_device += heap.size;
            }

            VkDeviceSize memsize_host = 0;
            for (uint32_t heapIndex : heapIndicesHost)
            {
                const VkMemoryHeap& heap = m_memoryProperty.memoryHeaps[heapIndex];
                AZ_Assert(!RHI::CheckBitsAny(heap.flags, static_cast<VkMemoryHeapFlags>(VK_MEMORY_HEAP_DEVICE_LOCAL_BIT)), "Host heap have device local bit.");
                memsize_host += heap.size;
            }

            m_descriptor.m_heapSizePerLevel[static_cast<size_t>(RHI::HeapMemoryLevel::Device)] = static_cast<size_t>(memsize_device);
            m_descriptor.m_heapSizePerLevel[static_cast<size_t>(RHI::HeapMemoryLevel::Host)] = static_cast<size_t>(memsize_host);
        }

        void PhysicalDevice::Shutdown()
        {
            m_vkPhysicalDevice = VK_NULL_HANDLE;
        }

        bool PhysicalDevice::IsFeatureSupported(DeviceFeature feature) const
        {
            uint32_t index = static_cast<uint32_t>(feature);
            AZ_Assert(index < m_features.size(), "Invalid feature %d", index);
            return m_features.test(index);
        }

    }
}
