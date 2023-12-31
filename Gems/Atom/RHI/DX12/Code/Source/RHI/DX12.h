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

#include <AzCore/std/smart_ptr/intrusive_ptr.h>

// Due to PAL (legal) restrictions, each platform is required to define each <DirectXType>X type internally. To give the platforms flexibility on overlapping types,
// the platforms themselves need to implement the reference counter policies for each base DirectX type.
#define AZ_DX12_REFCOUNTED(DXTypeName) \
    namespace AZStd \
    { \
        template <> \
        struct IntrusivePtrCountPolicy<DXTypeName> \
        { \
            static void add_ref(DXTypeName* p) { p->AddRef(); } \
            static void release(DXTypeName* p) { p->Release(); } \
        }; \
    }

#include <RHI/DX12_Platform.h>

#include <AzCore/std/string/string.h>
#include <AzCore/std/string/string_view.h>
#include <AzCore/std/containers/vector.h>
#include <Atom/RHI.Reflect/Base.h>

#ifndef IID_GRAPHICS_PPV_ARGS
#define IID_GRAPHICS_PPV_ARGS(ppType) IID_PPV_ARGS(ppType)
#endif

namespace AZ
{
    namespace DX12
    {
        template<typename T>
        using DX12Ptr = Microsoft::WRL::ComPtr<T>;

        inline bool operator == (const D3D12_CPU_DESCRIPTOR_HANDLE& l, const D3D12_CPU_DESCRIPTOR_HANDLE& r)
        {
            return l.ptr == r.ptr;
        }

        inline bool operator != (const D3D12_CPU_DESCRIPTOR_HANDLE& l, const D3D12_CPU_DESCRIPTOR_HANDLE& r)
        {
            return l.ptr != r.ptr;
        }

        inline bool AssertSuccess(HRESULT hr)
        {
            bool success = SUCCEEDED(hr);
            AZ_Assert(success, "HRESULT not a success %x", hr);
            return success;
        }

        template<class T, class U>
        inline RHI::Ptr<T> DX12ResourceCast(U* resource)
        {
            DX12Ptr<T> newResource;
            resource->QueryInterface(IID_GRAPHICS_PPV_ARGS(newResource.GetAddressOf()));
            return newResource.Get();
        }

        enum
        {
            DX12_RESOURCE_STATE_COPY_QUEUE_BIT = 0x80000000,

            DX12_RESOURCE_STATE_VALID_API_MASK = ~DX12_RESOURCE_STATE_COPY_QUEUE_BIT
        };

        using GpuDescriptorHandle = D3D12_GPU_DESCRIPTOR_HANDLE;
        using GpuVirtualAddress = D3D12_GPU_VIRTUAL_ADDRESS;
        using CpuVirtualAddress = uint8_t*;

        DXGI_FORMAT GetBaseFormat(DXGI_FORMAT defaultFormat);
        DXGI_FORMAT GetSRVFormat(DXGI_FORMAT defaultFormat);
        DXGI_FORMAT GetUAVFormat(DXGI_FORMAT defaultFormat);
        DXGI_FORMAT GetDSVFormat(DXGI_FORMAT defaultFormat);
        DXGI_FORMAT GetStencilFormat(DXGI_FORMAT defaultFormat);

        namespace Alignment
        {
            enum
            {
                Buffer = 16,
                Constant = 256,
                Image = 512,
                CommittedBuffer = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT
            };
        }
    }
}
