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
#include <Atom/RHI.Reflect/ShaderResourceGroupLayout.h>
#include <Atom/RHI.Reflect/ShaderResourceGroupLayoutDescriptor.h>
#include <AzCore/Utils/TypeHash.h>
#include <AzCore/std/createdestroy.h>
#include <AzCore/std/parallel/lock.h>
#include <RHI/Conversion.h>
#include <RHI/DescriptorSetLayout.h>
#include <RHI/Device.h>

namespace AZ
{
    namespace Vulkan
    {
        const uint32_t DescriptorSetLayout::InvalidLayoutIndex = static_cast<uint32_t>(-1);

        AZ::HashValue64 DescriptorSetLayout::Descriptor::GetHash() const
        {
            return m_shaderResouceGroupLayout->GetHash();
        }

        RHI::Ptr<DescriptorSetLayout> DescriptorSetLayout::Create()
        {
            return aznew DescriptorSetLayout();
        }

        DescriptorSetLayout::~DescriptorSetLayout()
        {
            // do nothing.
        }

        VkDescriptorSetLayout DescriptorSetLayout::GetNativeDescriptorSetLayout() const
        {
            return m_nativeDescriptorSetLayout;
        }

        const AZStd::vector<VkDescriptorSetLayoutBinding>& DescriptorSetLayout::GetNativeLayoutBindings() const
        {
            return m_descriptorSetLayoutBindings;
        }

        size_t DescriptorSetLayout::GetDescriptorSetLayoutBindingsCount() const
        {
            return m_descriptorSetLayoutBindings.size();
        }

        VkDescriptorType DescriptorSetLayout::GetDescriptorType(size_t index) const
        {
            return m_descriptorSetLayoutBindings[index].descriptorType;
        }

        uint32_t DescriptorSetLayout::GetDescriptorCount(size_t index) const
        {
            return m_descriptorSetLayoutBindings[index].descriptorCount;
        }

        uint32_t DescriptorSetLayout::GetConstantDataSize() const
        {
            return m_constantDataSize;
        }

        uint32_t DescriptorSetLayout::GetBindingIndex(uint32_t index) const
        {
            return m_descriptorSetLayoutBindings[index].binding;
        }

        uint32_t DescriptorSetLayout::GetLayoutIndexFromGroupIndex(uint32_t groupIndex, ResourceType type) const
        {
            switch (type)
            {
            case ResourceType::ConstantData:
                return m_layoutIndexOffset[static_cast<uint32_t>(type)];
            case ResourceType::BufferView:
            case ResourceType::ImageView:
            case ResourceType::Sampler:
                return m_layoutIndexOffset[static_cast<uint32_t>(type)] + groupIndex;
            default:
                AZ_Assert(false, "Invalid type %d", static_cast<uint32_t>(type));
                return InvalidLayoutIndex;
            }
        }

        RHI::ResultCode DescriptorSetLayout::Init(const Descriptor& descriptor)
        {
            AZ_Assert(descriptor.m_device, "Device is null.");
            Base::Init(*descriptor.m_device);
            m_shaderResourceGroupLayout = descriptor.m_shaderResouceGroupLayout;

            m_layoutIndexOffset.fill(InvalidLayoutIndex);
            const RHI::ResultCode result = BuildNativeDescriptorSetLayout();
            RETURN_RESULT_IF_UNSUCCESSFUL(result);

            SetName(GetName());
            return result;
        }

        void DescriptorSetLayout::SetNameInternal(const AZStd::string_view& name)
        {
            if (IsInitialized() && !name.empty())
            {
                Debug::SetNameToObject(reinterpret_cast<uint64_t>(m_nativeDescriptorSetLayout), name.data(), VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT, static_cast<Device&>(GetDevice()));
            }
        }

        void DescriptorSetLayout::Shutdown()
        {
            if (m_nativeDescriptorSetLayout != VK_NULL_HANDLE)
            {
                auto& device = static_cast<Device&>(GetDevice());
                vkDestroyDescriptorSetLayout(device.GetNativeDevice(), m_nativeDescriptorSetLayout, nullptr);
                m_nativeDescriptorSetLayout = VK_NULL_HANDLE;
            }
            m_shaderResourceGroupLayout = nullptr;
            Base::Shutdown();
        }

        RHI::ResultCode DescriptorSetLayout::BuildNativeDescriptorSetLayout()
        {
            const RHI::ResultCode buildResult = BuildDescriptorSetLayoutBindings();
            RETURN_RESULT_IF_UNSUCCESSFUL(buildResult);

            VkDescriptorSetLayoutCreateInfo createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            createInfo.pNext = nullptr;
            createInfo.flags = 0;
            createInfo.bindingCount = static_cast<uint32_t>(m_descriptorSetLayoutBindings.size());
            createInfo.pBindings = m_descriptorSetLayoutBindings.size() ? m_descriptorSetLayoutBindings.data() : nullptr;

            auto& device = static_cast<Device&>(GetDevice());
            const VkResult result = vkCreateDescriptorSetLayout(device.GetNativeDevice(), &createInfo, nullptr, &m_nativeDescriptorSetLayout);

            return ConvertResult(result);
        }

        RHI::ResultCode DescriptorSetLayout::BuildDescriptorSetLayoutBindings()
        {
            const AZStd::array_view<RHI::ShaderInputBufferDescriptor> bufferDescs = m_shaderResourceGroupLayout->GetShaderInputListForBuffers();
            const AZStd::array_view<RHI::ShaderInputImageDescriptor> imageDescs = m_shaderResourceGroupLayout->GetShaderInputListForImages();
            const AZStd::array_view<RHI::ShaderInputSamplerDescriptor> samplerDescs = m_shaderResourceGroupLayout->GetShaderInputListForSamplers();
            const AZStd::array_view<RHI::ShaderInputStaticSamplerDescriptor>& staticSamplerDescs = m_shaderResourceGroupLayout->GetStaticSamplers();

            // The + 1 is for Constant Data.
            m_descriptorSetLayoutBindings.reserve(1 + bufferDescs.size() + imageDescs.size() + samplerDescs.size() + staticSamplerDescs.size());
            m_constantDataSize = m_shaderResourceGroupLayout->GetConstantDataSize();
            if (m_constantDataSize)
            {
                AZStd::array_view<RHI::ShaderInputConstantDescriptor> inputListForConstants = m_shaderResourceGroupLayout->GetShaderInputListForConstants();
                AZ_Assert(!inputListForConstants.empty(), "Empty constant input list");
                m_descriptorSetLayoutBindings.emplace_back(VkDescriptorSetLayoutBinding{});
                VkDescriptorSetLayoutBinding& vbinding = m_descriptorSetLayoutBindings.back();
                // All constant data of the SRG have the same binding.
                vbinding.binding = inputListForConstants[0].m_registerId;
                vbinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                vbinding.descriptorCount = 1;
                vbinding.stageFlags = VK_SHADER_STAGE_ALL; // [GFX TODO][ATOM-347] find a way to get an appropriate shader visibility. 
                vbinding.pImmutableSamplers = nullptr;
                m_layoutIndexOffset[static_cast<uint32_t>(ResourceType::ConstantData)] = 0;
            }

            m_layoutIndexOffset[static_cast<uint32_t>(ResourceType::BufferView)] = bufferDescs.empty() ? 
                InvalidLayoutIndex : static_cast<uint32_t>(m_descriptorSetLayoutBindings.size());
            for (uint32_t index = 0; index < bufferDescs.size(); ++index)
            {
                const RHI::ShaderInputBufferDescriptor& desc = bufferDescs[index];
                m_descriptorSetLayoutBindings.emplace_back(VkDescriptorSetLayoutBinding{});
                VkDescriptorSetLayoutBinding& vbinding = m_descriptorSetLayoutBindings.back();
                vbinding.binding = desc.m_registerId;
                switch (desc.m_access)
                {
                case RHI::ShaderInputBufferAccess::Constant:
                    vbinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                    if (!ValidateUniformBufferDeviceLimits(desc))
                    {
                        return RHI::ResultCode::OutOfMemory;
                    }

                    break;
                case RHI::ShaderInputBufferAccess::Read:
                    vbinding.descriptorType = desc.m_type == RHI::ShaderInputBufferType::Typed ? 
                        VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER : VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
                    break;
                case RHI::ShaderInputBufferAccess::ReadWrite:
                    vbinding.descriptorType = desc.m_type == RHI::ShaderInputBufferType::Typed ? 
                        VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER : VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
                    break;
                default:
                    AZ_Assert(false, "ShaderInputBufferAccess is illegal.");
                    return RHI::ResultCode::InvalidArgument;
                }
                vbinding.descriptorCount = desc.m_count;
                vbinding.stageFlags = VK_SHADER_STAGE_ALL; // [GFX TODO][ATOM-347] find a way to get an appropriate shader visibility. 
                vbinding.pImmutableSamplers = nullptr;
            }

            m_layoutIndexOffset[static_cast<uint32_t>(ResourceType::ImageView)] = imageDescs.empty() ? 
                InvalidLayoutIndex : static_cast<uint32_t>(m_descriptorSetLayoutBindings.size());
            for (uint32_t index = 0; index < imageDescs.size(); ++index)
            {
                const RHI::ShaderInputImageDescriptor& desc = imageDescs[index];
                m_descriptorSetLayoutBindings.emplace_back(VkDescriptorSetLayoutBinding{});
                VkDescriptorSetLayoutBinding& vbinding = m_descriptorSetLayoutBindings.back();
                vbinding.binding = desc.m_registerId;
                if (desc.m_type == RHI::ShaderInputImageType::SubpassInput)
                {
                    vbinding.descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
                    vbinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
                }
                else
                {
                    switch (desc.m_access)
                    {
                    case RHI::ShaderInputImageAccess::Read:
                        vbinding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
                        break;
                    case RHI::ShaderInputImageAccess::ReadWrite:
                        vbinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
                        break;
                    default:
                        AZ_Assert(false, "ShaderInputImageAccess is illegal.");
                        return RHI::ResultCode::InvalidArgument;
                    }
                    vbinding.stageFlags = VK_SHADER_STAGE_ALL; // [GFX TODO][ATOM-347]  find a way to get an appropriate shader visibility. 
                }
               
                vbinding.descriptorCount = desc.m_count;
                vbinding.pImmutableSamplers = nullptr;
            }

            m_layoutIndexOffset[static_cast<uint32_t>(ResourceType::Sampler)] = samplerDescs.empty() ? 
                InvalidLayoutIndex : static_cast<uint32_t>(m_descriptorSetLayoutBindings.size());
            for (uint32_t index = 0; index < samplerDescs.size(); ++index)
            {
                const RHI::ShaderInputSamplerDescriptor& desc = samplerDescs[index];
                m_descriptorSetLayoutBindings.emplace_back(VkDescriptorSetLayoutBinding{});
                VkDescriptorSetLayoutBinding& vbinding = m_descriptorSetLayoutBindings.back();
                vbinding.binding = desc.m_registerId;
                vbinding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
                vbinding.descriptorCount = desc.m_count;
                vbinding.stageFlags = VK_SHADER_STAGE_ALL; // [GFX TODO][ATOM-347] find a way to get an appropriate shader visibility. 
                vbinding.pImmutableSamplers = nullptr;
            }

            if (!staticSamplerDescs.empty())
            {
                auto& device = static_cast<Device&>(GetDevice());
                m_nativeSamplers.resize(staticSamplerDescs.size(), VK_NULL_HANDLE);
                for (int index = 0; index < staticSamplerDescs.size(); ++index)
                {
                    const RHI::ShaderInputStaticSamplerDescriptor& staticSamplerInput  = staticSamplerDescs[index];
                    Sampler::Descriptor samplerDesc;
                    samplerDesc.m_device = &device;
                    samplerDesc.m_samplerState = staticSamplerInput.m_samplerState;
                    m_nativeSamplers[index] = device.AcquireSampler(samplerDesc)->GetNativeSampler();

                    m_descriptorSetLayoutBindings.emplace_back(VkDescriptorSetLayoutBinding{});
                    VkDescriptorSetLayoutBinding& vbinding = m_descriptorSetLayoutBindings.back();
                    vbinding.binding = staticSamplerInput.m_registerId;
                    vbinding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
                    vbinding.descriptorCount = 1;
                    vbinding.stageFlags = VK_SHADER_STAGE_ALL; // [GFX TODO][ATOM-347] find a way to get an appropriate shader visibility. 
                    vbinding.pImmutableSamplers = &m_nativeSamplers[index];
                }
            }

            return RHI::ResultCode::Success;
        }

        bool DescriptorSetLayout::ValidateUniformBufferDeviceLimits([[maybe_unused]] const RHI::ShaderInputBufferDescriptor& desc)
        {
#if defined (AZ_RHI_ENABLE_VALIDATION)       
            const auto& physicalDevice = static_cast<const PhysicalDevice&>(GetDevice().GetPhysicalDevice());
            const VkPhysicalDeviceLimits deviceLimits = physicalDevice.GetDeviceLimits();
            if (desc.m_count > deviceLimits.maxPerStageDescriptorUniformBuffers)
            {
                AZ_Assert(false, "Maximum number of uniform buffer exceeded(%d), needed %d", deviceLimits.maxPerStageDescriptorUniformBuffers, desc.m_count);
                return false;
            }       
#endif // AZ_RHI_ENABLE_VALIDATION

            return true;
        }

        const RHI::ShaderResourceGroupLayout* DescriptorSetLayout::GetShaderResourceGroupLayout() const
        {
            return m_shaderResourceGroupLayout.get();
        }
    }
}
