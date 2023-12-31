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
#include "RHI/Atom_RHI_DX12_precompiled.h"
#include <RHI/RayTracingPipelineState.h>
#include <RHI/RayTracingShaderTable.h>
#include <Atom/RHI.Reflect/DX12/ShaderStageFunction.h>
#include <RHI/Buffer.h>
#include <RHI/Conversions.h>
#include <RHI/Device.h>
#include <Atom/RHI/Factory.h>
#include <AzCore/Debug/EventTrace.h>
#include <Atom/RHI/BufferPool.h>
#include <Atom/RHI/RayTracingBufferPools.h>
#include <RHI/ShaderResourceGroup.h>

namespace AZ
{
    namespace DX12
    {
        RHI::Ptr<RayTracingShaderTable> RayTracingShaderTable::Create()
        {
            return aznew RayTracingShaderTable;
        }

#ifdef AZ_DX12_DXR_SUPPORT
        uint32_t RayTracingShaderTable::FindLargestRecordSize(const RHI::RayTracingShaderTableRecordList& recordList)
        {
            uint32_t largestRecordSize = 0;
            for (const auto& record : recordList)
            {
                uint32_t recordSize = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;

                if (record.m_shaderResourceGroup)
                {
                    const ShaderResourceGroupCompiledData& compiledData = static_cast<const ShaderResourceGroup*>(record.m_shaderResourceGroup)->GetCompiledData();

                    recordSize += (compiledData.m_gpuConstantAddress != 0) ? sizeof(GpuVirtualAddress) : 0;
                    recordSize += (compiledData.m_gpuViewsDescriptorHandle.ptr != 0) ? sizeof(GpuDescriptorHandle) : 0;
                }

                largestRecordSize = AZStd::max(largestRecordSize, recordSize);
            }

            return largestRecordSize;
        }

        RHI::Ptr<Buffer> RayTracingShaderTable::BuildTable(RHI::Device& deviceBase,
                                                           const RHI::RayTracingBufferPools& bufferPools,
                                                           const RHI::RayTracingShaderTableRecordList& recordList,
                                                           uint32_t shaderRecordSize,
                                                           AZStd::wstring shaderTableName,
                                                           Microsoft::WRL::ComPtr<ID3D12StateObjectProperties>& stateObjectProperties)
        {
            Device& device = static_cast<Device&>(deviceBase);

            uint32_t shaderTableSize = shaderRecordSize * static_cast<uint32_t>(recordList.size());

            // create shader table buffer
            RHI::Ptr<RHI::Buffer> shaderTableBuffer = RHI::Factory::Get().CreateBuffer();
            AZ::RHI::BufferDescriptor shaderTableBufferDescriptor;
            shaderTableBufferDescriptor.m_bindFlags = RHI::BufferBindFlags::ShaderRead;
            shaderTableBufferDescriptor.m_byteCount = shaderTableSize;
            shaderTableBufferDescriptor.m_alignment = D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT;

            AZ::RHI::BufferInitRequest shaderTableBufferRequest;
            shaderTableBufferRequest.m_buffer = shaderTableBuffer.get();
            shaderTableBufferRequest.m_descriptor = shaderTableBufferDescriptor;
            RHI::ResultCode resultCode = bufferPools.GetShaderTableBufferPool()->InitBuffer(shaderTableBufferRequest);
            AZ_Assert(resultCode == RHI::ResultCode::Success, "failed to create shader table buffer");

            MemoryView& shaderTableMemoryView = static_cast<Buffer*>(shaderTableBuffer.get())->GetMemoryView();
            shaderTableMemoryView.SetName("RayTracingShaderTable");

            // copy records
            RHI::BufferMapResponse mapResponse;
            resultCode = bufferPools.GetShaderTableBufferPool()->MapBuffer(RHI::BufferMapRequest(*shaderTableBuffer, 0, shaderTableSize), mapResponse);
            AZ_Assert(resultCode == RHI::ResultCode::Success, "failed to map shader table buffer");
            uint8_t* mappedData = reinterpret_cast<uint8_t*>(mapResponse.m_data);

            for (const auto& record : recordList)
            {
                uint8_t* nextRecord = RHI::AlignUp(mappedData + shaderRecordSize, D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT);

                AZStd::wstring shaderExportNameWstring;
                AZStd::to_wstring(shaderExportNameWstring, record.m_shaderExportName.GetStringView().data(), record.m_shaderExportName.GetStringView().size());
                void* shaderIdentifier = stateObjectProperties->GetShaderIdentifier(shaderExportNameWstring.c_str());
                memcpy(mappedData, shaderIdentifier, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
                mappedData += D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;

                if (record.m_shaderResourceGroup)
                {
                    const ShaderResourceGroupCompiledData& compiledData = static_cast<const ShaderResourceGroup*>(record.m_shaderResourceGroup)->GetCompiledData();

                    if (compiledData.m_gpuConstantAddress != 0)
                    {
                        memcpy(mappedData, &compiledData.m_gpuConstantAddress, sizeof(compiledData.m_gpuConstantAddress));
                        mappedData += sizeof(GpuVirtualAddress);
                    }

                    if (compiledData.m_gpuViewsDescriptorHandle.ptr != 0)
                    {
                        memcpy(mappedData, &compiledData.m_gpuViewsDescriptorHandle, sizeof(compiledData.m_gpuViewsDescriptorHandle));
                        mappedData += sizeof(GpuDescriptorHandle);
                    }
                }

                mappedData = nextRecord;
            }

            bufferPools.GetShaderTableBufferPool()->UnmapBuffer(*shaderTableBuffer);

            return static_cast<Buffer*>(shaderTableBuffer.get());
        }
#endif

        RHI::ResultCode RayTracingShaderTable::InitInternal([[maybe_unused]] RHI::Device& deviceBase, [[maybe_unused]] const RHI::RayTracingShaderTableDescriptor* descriptor, [[maybe_unused]] const RHI::RayTracingBufferPools& bufferPools)
        {
#ifdef AZ_DX12_DXR_SUPPORT
            // advance to the next buffer
            m_currentBufferIndex = (m_currentBufferIndex + 1) % BufferCount;
            ShaderTableBuffers& buffers = m_buffers[m_currentBufferIndex];

            // clear the shader table if a null descriptor was passed in
            if (!descriptor)
            {
                buffers.m_rayGenerationTable = nullptr;
                buffers.m_rayGenerationTableSize = 0;
                buffers.m_missTable = nullptr;
                buffers.m_missTableSize = 0;
                buffers.m_missTableStride = 0;
                buffers.m_hitGroupTable = nullptr;
                buffers.m_hitGroupTableSize = 0;
                buffers.m_hitGroupTableStride = 0;
                return RHI::ResultCode::Success;
            }

            // retrieve the ID3D12StateObjectProperties interface from the raytracing pipeline state object
            // this is needed to get the shader identifiers to put in the table
            const RayTracingPipelineState* rayTracingPipelineState = static_cast<const RayTracingPipelineState*>(descriptor->GetPipelineState().get());

            Microsoft::WRL::ComPtr<ID3D12StateObjectProperties> stateObjectProperties;
            HRESULT hr = rayTracingPipelineState->Get()->QueryInterface(IID_GRAPHICS_PPV_ARGS(stateObjectProperties.GetAddressOf()));
            AZ_Assert(SUCCEEDED(hr), "Failed to query ID3D12StateObjectProperties");

            // ray generation shader table
            {
                // RayGeneration table must have one and only one record
                AZ_Assert(descriptor->GetRayGenerationRecord().size() == 1, "Descriptor must contain one and only one RayGeneration record");
                uint32_t shaderRecordSize = RHI::AlignUp(FindLargestRecordSize(descriptor->GetRayGenerationRecord()), D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT);

                buffers.m_rayGenerationTable = BuildTable(deviceBase, bufferPools, descriptor->GetRayGenerationRecord(), shaderRecordSize, L"Ray Generation Shader Table", stateObjectProperties);
                buffers.m_rayGenerationTableSize = shaderRecordSize;
            }

            // miss shader table
            {
                uint32_t shaderRecordSize = RHI::AlignUp(FindLargestRecordSize(descriptor->GetMissRecords()), D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT);

                buffers.m_missTable = BuildTable(deviceBase, bufferPools, descriptor->GetMissRecords(), shaderRecordSize, L"Miss Shader Table", stateObjectProperties);
                buffers.m_missTableSize = shaderRecordSize * static_cast<uint32_t>(descriptor->GetMissRecords().size());
                buffers.m_missTableStride = shaderRecordSize;
            }
            
            // hit group shader table
            {
                uint32_t shaderRecordSize = RHI::AlignUp(FindLargestRecordSize(descriptor->GetHitGroupRecords()), D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT);

                buffers.m_hitGroupTable = BuildTable(deviceBase, bufferPools, descriptor->GetHitGroupRecords(), shaderRecordSize, L"HitGroup Shader Table", stateObjectProperties);
                buffers.m_hitGroupTableSize = shaderRecordSize * static_cast<uint32_t>(descriptor->GetHitGroupRecords().size());
                buffers.m_hitGroupTableStride = shaderRecordSize;
            }
#endif
            return RHI::ResultCode::Success;
        }
    }
}
