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

#include <Atom/RHI/DeviceObject.h>
#include <Atom/RHI.Reflect/Metal/PipelineLayoutDescriptor.h>
#include <Atom/RHI.Reflect/SamplerState.h>
#include <Metal/Metal.h>
#include <RHI/BufferMemoryAllocator.h>
#include <RHI/Conversions.h>
#include <RHI/ShaderResourceGroupPool.h>

//[GFX TODO][ATOM - 3653] - Remove one of the ways of allocating constant/argument buffers
//#define ARGUMENTBUFFER_PAGEALLOCATOR

struct ResourceBindingData
{
    AZ::RHI::Ptr<AZ::Metal::Memory> m_resourcPtr;
    union
    {
        AZ::RHI::ShaderInputImageAccess m_imageAccess;
        AZ::RHI::ShaderInputBufferAccess m_bufferAccess;
    };
    
    bool operator==(const ResourceBindingData& other) const
    {
        return this->m_resourcPtr == other.m_resourcPtr;
    };
    
    size_t GetHash() const
    {
        return static_cast<size_t>(m_resourcPtr->GetHash());
    }
};

namespace AZStd
{
   template<>
   struct hash<ResourceBindingData>
   {
       size_t operator()(const ResourceBindingData& resourceBindingData) const noexcept
       {
           return resourceBindingData.GetHash();
       }
   };
}

namespace AZ
{
    namespace Metal
    {
        class Device;
        class BufferMemoryAllocator;
        class ShaderResourceGroup;
        struct ShaderResourceGroupCompiledData;
        
        class ArgumentBuffer final
            : public RHI::DeviceObject
        {
            using Base = RHI::DeviceObject;
            
        public:
            AZ_CLASS_ALLOCATOR(ArgumentBuffer, AZ::SystemAllocator, 0);
            AZ_RTTI(ArgumentBuffer, "FEFE8823-7772-4EA0-9241-65C49ADFF6B3", Base);

            ArgumentBuffer() = default;

            static RHI::Ptr<ArgumentBuffer> Create();
            void Init(Device* device,
                      RHI::ConstPtr<RHI::ShaderResourceGroupLayout> srgLayout,
                      ShaderResourceGroup& group,
                      ShaderResourceGroupPool* srgPool);

            void UpdateImageViews(const RHI::ShaderInputImageDescriptor& shaderInputImage,
                                  const RHI::ShaderInputImageIndex shaderInputIndex,
                                  const AZStd::array_view<RHI::ConstPtr<RHI::ImageView>>& imageViews);
            
            void UpdateSamplers(const RHI::ShaderInputSamplerDescriptor& shaderInputSampler,
                                const RHI::ShaderInputSamplerIndex shaderInputIndex,
                                const AZStd::array_view<RHI::SamplerState>& samplerStates);
            
            void UpdateBufferViews(const RHI::ShaderInputBufferDescriptor& shaderInputBuffer,
                                   const RHI::ShaderInputBufferIndex shaderInputIndex,
                                   const AZStd::array_view<RHI::ConstPtr<RHI::BufferView>>& bufferViews);
            
            void UpdateConstantBufferViews(AZStd::array_view<uint8_t> rawData);
                        
            id<MTLBuffer> GetArgEncoderBuffer() const;
            size_t GetOffset() const;
            
            void AddUntrackedResourcesToEncoder(id<MTLCommandEncoder> commandEncoder, const ShaderResourceGroupVisibility& srgResourcesVisInfo) const;
            
            void ClearResourceTracking();
            
            //////////////////////////////////////////////////////////////////////////
            // RHI::DeviceObject
            void Shutdown() override;
            //////////////////////////////////////////////////////////////////////////
            
        private:
            
            bool CreateArgumentDescriptors(NSMutableArray * argBufferDecriptors);
            void AttachStaticSamplers();
            void AttachConstantBuffer();
            
            // Use a cache to store and retrieve samplers
            id<MTLSamplerState> GetMtlSampler(MTLSamplerDescriptor* samplerDesc);

            using ResourceBindingsSet = AZStd::unordered_set<ResourceBindingData>;
            using ResourceBindingsMap =  AZStd::unordered_map<AZ::Name, ResourceBindingsSet>;
            ResourceBindingsMap m_resourceBindings;
            
            void ApplyUseResourceToCompute(id<MTLCommandEncoder> encoder, const ResourceBindingsSet& resourceBindingData) const;
            void ApplyUseResourceToGraphic(id<MTLCommandEncoder> encoder, RHI::ShaderStageMask visShaderMask, const ResourceBindingsSet& resourceBindingDataSet) const;
            //! Use visibility information to call UseResource on all resources for this Argument Buffer
            void ApplyUseResource(id<MTLCommandEncoder> encoder,
                                  const ResourceBindingsMap& resourceMap,
                                  const ShaderResourceGroupVisibility& srgResourcesVisInfo) const;
            void BindNullSamplers(uint32_t registerId, uint32_t samplerCount);
            
            Device* m_device = nullptr;
            RHI::ConstPtr<RHI::ShaderResourceGroupLayout> m_srgLayout;
                        
            id <MTLArgumentEncoder> m_argumentEncoder;
            uint32_t m_constantBufferSize = 0;
                        
#if defined(ARGUMENTBUFFER_PAGEALLOCATOR)
            BufferMemoryView m_argumentBuffer;
            BufferMemoryView m_constantBuffer;
#else
            //We are keeping the non-page allocation implementation for GPU captures
            //GPU captures dont work well with argument buffers with offsets
            MemoryView m_argumentBuffer;
            MemoryView m_constantBuffer;
#endif
            
            ShaderResourceGroupPool* m_srgPool = nullptr;
            
            static const int MaxEntriesInArgTable = 31;
            NSCache* m_samplerCache;
        };
    }
}
