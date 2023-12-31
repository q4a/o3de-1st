#
# All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
# its licensors.
#
# For complete copyright and license terms please see the LICENSE at the root of this
# distribution (the "License"). All use of this software is governed by the License,
# or, if provided, by the license below or the license accompanying this file. Do not
# remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#

set(FILES
    ../../DX12/CCryDX12Object.cpp
    ../../DX12/CCryDX12Object.hpp
    ../../DX12/CryDX12.cpp
    ../../DX12/CryDX12.hpp
    ../../DX12/CryDX12Guid.hpp
    ../../DX12/CryDX12Legacy.hpp
    ../../DX12/API/DX12Base.cpp
    ../../DX12/API/DX12CommandList.cpp
    ../../DX12/API/DX12CommandListFence.cpp
    ../../DX12/API/DX12DescriptorHeap.cpp
    ../../DX12/API/DX12QueryHeap.cpp
    ../../DX12/API/DX12Device.cpp
    ../../DX12/API/DX12PSO.cpp
    ../../DX12/API/DX12Resource.cpp
    ../../DX12/API/DX12ResourceBarrierCache.cpp
    ../../DX12/API/DX12RootSignature.cpp
    ../../DX12/API/DX12Shader.cpp
    ../../DX12/API/DX12View.cpp
    ../../DX12/API/DX12SamplerState.cpp
    ../../DX12/API/DX12TimerHeap.cpp
    ../../DX12/API/DX12SwapChain.cpp
    ../../DX12/API/DX12Base.hpp
    ../../DX12/API/DX12CommandList.hpp
    ../../DX12/API/DX12CommandListFence.hpp
    ../../DX12/API/DX12DescriptorHeap.hpp
    ../../DX12/API/DX12QueryHeap.hpp
    ../../DX12/API/DX12Device.hpp
    ../../DX12/API/DX12PSO.hpp
    ../../DX12/API/DX12Resource.hpp
    ../../DX12/API/DX12ResourceBarrierCache.h
    ../../DX12/API/DX12RootSignature.hpp
    ../../DX12/API/DX12Shader.hpp
    ../../DX12/API/DX12View.hpp
    ../../DX12/API/DX12SamplerState.hpp
    ../../DX12/API/DX12SwapChain.hpp
    ../../DX12/API/DX12TimerHeap.h
    ../../DX12/API/DX12AsyncCommandQueue.hpp
    ../../DX12/API/DX12AsyncCommandQueue.cpp
    ../../DX12/Device/CCryDX12Device.cpp
    ../../DX12/Device/CCryDX12DeviceChild.cpp
    ../../DX12/Device/CCryDX12DeviceContext.cpp
    ../../DX12/Device/CCryDX12Device.hpp
    ../../DX12/Device/CCryDX12DeviceChild.hpp
    ../../DX12/Device/CCryDX12DeviceContext.hpp
    ../../DX12/GI/CCryDX12GIFactory.cpp
    ../../DX12/GI/CCryDX12SwapChain.cpp
    ../../DX12/GI/CCryDX12GIFactory.hpp
    ../../DX12/GI/CCryDX12SwapChain.hpp
    ../../DX12/Misc/SCryDX11PipelineState.cpp
    ../../DX12/Misc/SCryDX11PipelineState.hpp
    ../../DX12/Resource/CCryDX12Asynchronous.cpp
    ../../DX12/Resource/CCryDX12Resource.cpp
    ../../DX12/Resource/CCryDX12View.cpp
    ../../DX12/Resource/CCryDX12Asynchronous.hpp
    ../../DX12/Resource/CCryDX12Resource.hpp
    ../../DX12/Resource/CCryDX12View.hpp
    ../../DX12/Resource/Misc/CCryDX12Buffer.cpp
    ../../DX12/Resource/Misc/CCryDX12InputLayout.cpp
    ../../DX12/Resource/Misc/CCryDX12Query.cpp
    ../../DX12/Resource/Misc/CCryDX12Shader.cpp
    ../../DX12/Resource/Misc/CCryDX12Buffer.hpp
    ../../DX12/Resource/Misc/CCryDX12InputLayout.hpp
    ../../DX12/Resource/Misc/CCryDX12Query.hpp
    ../../DX12/Resource/Misc/CCryDX12Shader.hpp
    ../../DX12/Resource/State/CCryDX12BlendState.cpp
    ../../DX12/Resource/State/CCryDX12DepthStencilState.cpp
    ../../DX12/Resource/State/CCryDX12RasterizerState.cpp
    ../../DX12/Resource/State/CCryDX12SamplerState.cpp
    ../../DX12/Resource/State/CCryDX12BlendState.hpp
    ../../DX12/Resource/State/CCryDX12DepthStencilState.hpp
    ../../DX12/Resource/State/CCryDX12RasterizerState.hpp
    ../../DX12/Resource/State/CCryDX12SamplerState.hpp
    ../../DX12/Resource/Texture/CCryDX12Texture1D.cpp
    ../../DX12/Resource/Texture/CCryDX12Texture2D.cpp
    ../../DX12/Resource/Texture/CCryDX12Texture3D.cpp
    ../../DX12/Resource/Texture/CCryDX12TextureBase.cpp
    ../../DX12/Resource/Texture/CCryDX12Texture1D.hpp
    ../../DX12/Resource/Texture/CCryDX12Texture2D.hpp
    ../../DX12/Resource/Texture/CCryDX12Texture3D.hpp
    ../../DX12/Resource/Texture/CCryDX12TextureBase.hpp
    ../../DX12/Resource/View/CCryDX12DepthStencilView.cpp
    ../../DX12/Resource/View/CCryDX12RenderTargetView.cpp
    ../../DX12/Resource/View/CCryDX12ShaderResourceView.cpp
    ../../DX12/Resource/View/CCryDX12UnorderedAccessView.cpp
    ../../DX12/Resource/View/CCryDX12DepthStencilView.hpp
    ../../DX12/Resource/View/CCryDX12RenderTargetView.hpp
    ../../DX12/Resource/View/CCryDX12ShaderResourceView.hpp
    ../../DX12/Resource/View/CCryDX12UnorderedAccessView.hpp
    ../../DX12/Includes/d3d11tokenizedprogramformat.hpp
    ../../DX12/Includes/d3dx12.h
    ../../DX12/Includes/fasthash.inl
    ../../DX12/Includes/fasthash.h
    ../../DX12/Includes/concqueue.hpp
    ../../DX12/Includes/concqueue-mpmc-bounded.hpp
    ../../DX12/Includes/concqueue-mpsc.hpp
    ../../DX12/Includes/concqueue-spsc-bounded.hpp
    ../../DX12/Includes/concqueue-spsc.hpp
)
