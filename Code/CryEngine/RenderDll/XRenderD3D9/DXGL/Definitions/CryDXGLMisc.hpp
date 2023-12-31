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
// Original file Copyright Crytek GMBH or its affiliates, used under license.

// Description : Internal declarations of types and macros required by the
//               DXGL library


#ifndef __CRYDXGLMISC__
#define __CRYDXGLMISC__

#if defined(WIN32)
#include <windows.h>
#include <objbase.h>
#include <mmsyscom.h>
#endif //defined(WIN32)

#if defined(DO_RENDERLOG) || !defined(_RELEASE)
#define DXGL_VIRTUAL_DEVICE_AND_CONTEXT 1
#else
#define DXGL_VIRTUAL_DEVICE_AND_CONTEXT 0
#endif

#include "CryDXGLGuid.hpp"

#if !defined(WIN32)

#ifdef NO_ILINE
    #define FORCEINLINE inline
#else
    #define FORCEINLINE __attribute__((always_inline)) inline
#endif

#define SEVERITY_SUCCESS    0
#define SEVERITY_ERROR      1

#undef SUCCEEDED
#define SUCCEEDED(x) ((x) >= 0)
#undef FAILED
#define FAILED(x) (!(SUCCEEDED(x)))

#define MAKE_HRESULT(sev, fac, code) \
    ((HRESULT) (((unsigned long)(sev) << 31) | ((unsigned long)(fac) << 16) | ((unsigned long)(code))))

#define S_OK                         0
#define S_FALSE                      1

#define CCHDEVICENAME 32

#define CONST const
#define VOID void
#define THIS void
#define THIS_
#if !defined(TRUE) || !defined(FALSE)
#undef TRUE
#undef FALSE
#define TRUE true
#define FALSE false
#endif //!defined(TRUE) || !defined(FALSE)

#define interface struct
#define STDMETHOD(method)        virtual HRESULT STDMETHODCALLTYPE method
#define STDMETHOD_(type, method)  virtual type STDMETHODCALLTYPE method
#define STDMETHODV(method)       virtual HRESULT STDMETHODVCALLTYPE method
#define STDMETHODV_(type, method) virtual type STDMETHODVCALLTYPE method

#if defined(UNICODE)
#define TEXT(_STRING) L ## _STRING
#else
#define TEXT(_STRING) _STRING
#endif

typedef char CHAR;
typedef int32 INT;
typedef unsigned char UCHAR;
typedef uint8 UINT8;
typedef uint32 UINT32;
typedef uint32 UINT;
typedef const void* LPCVOID;

typedef uint32 HMONITOR;
typedef void* HINSTANCE;

typedef struct _LUID
{
    uint32 LowPart;
    long HighPart;
} LUID, * PLUID;

typedef RECT* LPRECT;
typedef const RECT* LPCRECT;

#ifndef HRESULT_VALUES_DEFINED
#define HRESULT_VALUES_DEFINED
enum
{
    E_OUTOFMEMORY  = 0x8007000E,
    E_FAIL         = 0x80004005,
    E_ABORT        = 0x80004004,
    E_INVALIDARG   = 0x80070057,
    E_NOINTERFACE  = 0x80004002,
    E_NOTIMPL      = 0x80004001,
    E_UNEXPECTED   = 0x8000FFFF
};
#endif

#else

// Required as long as Windows builds allow compilation through a separate DLL (CryD3DCompilerStub) to distinguish between ID3D11Blob and CCryDXGLBlob
#define DXGL_BLOB_INTEROPERABILITY

typedef RECT D3D11_RECT;

#endif  // !defined(WIN32)

#if defined(_MSC_VER)
    #define DXGL_EXPORT __declspec(dllexport)
    #define DXGL_IMPORT __declspec(dllimport)
#elif defined(__GNUC__)
    #define DXGL_EXPORT __attribute__ ((visibility("default")))
    #define DXGL_IMPORT __attribute__ ((visibility("default")))
#else
    #define DXGL_EXPORT
    #define DXGL_IMPORT
#endif

////////////////////////////////////////////////////////////////////////////
//  Forward declaration of typedef interfaces
////////////////////////////////////////////////////////////////////////////

#if !DXGL_FULL_EMULATION

typedef class CCryDXGLResource ID3D11Resource;
typedef class CCryDXGLBuffer ID3D11Buffer;
typedef class CCryDXGLTexture1D ID3D11Texture1D;
typedef class CCryDXGLTexture2D ID3D11Texture2D;
typedef class CCryDXGLTexture3D ID3D11Texture3D;
typedef class CCryDXGLView ID3D11View;
typedef class CCryDXGLRenderTargetView ID3D11RenderTargetView;
typedef class CCryDXGLDepthStencilView ID3D11DepthStencilView;
typedef class CCryDXGLShaderResourceView ID3D11ShaderResourceView;
typedef class CCryDXGLUnorderedAccessView ID3D11UnorderedAccessView;
typedef class CCryDXGLInputLayout ID3D11InputLayout;
typedef class CCryDXGLVertexShader ID3D11VertexShader;
typedef class CCryDXGLHullShader ID3D11HullShader;
typedef class CCryDXGLDomainShader ID3D11DomainShader;
typedef class CCryDXGLGeometryShader ID3D11GeometryShader;
typedef class CCryDXGLPixelShader ID3D11PixelShader;
typedef class CCryDXGLComputeShader ID3D11ComputeShader;
typedef class CCryDXGLSamplerState ID3D11SamplerState;
typedef class CCryDXGLQuery ID3D11Asynchronous;
typedef class CCryDXGLQuery ID3D11Predicate;
typedef class CCryDXGLQuery ID3D11Query;
typedef class CCryDXGLGIOutput IDXGIOutput;
typedef class CCryDXGLSwapChain IDXGISwapChain;
typedef class CCryDXGLGIFactory IDXGIFactory;
typedef class CCryDXGLGIFactory IDXGIFactory1;
typedef class CCryDXGLGIObject IDXGIObject;
typedef class CCryDXGLGIAdapter IDXGIAdapter;
typedef class CCryDXGLGIAdapter IDXGIAdapter1;
typedef class CCryDXGLDeviceChild ID3D11DeviceChild;
typedef class CCryDXGLSwitchToRef ID3D11SwitchToRef;
typedef class CCryDXGLShaderReflection ID3D11ShaderReflection;
typedef class CCryDXGLShaderReflectionVariable ID3D11ShaderReflectionVariable;
typedef class CCryDXGLShaderReflectionVariable ID3D11ShaderReflectionType;
typedef class CCryDXGLShaderReflectionConstBuffer ID3D11ShaderReflectionConstantBuffer;
typedef class CCryDXGLShaderReflection ID3D11ShaderReflection;
typedef class CCryDXGLBlendState ID3D11BlendState;
typedef class CCryDXGLDepthStencilState ID3D11DepthStencilState;
typedef class CCryDXGLRasterizerState ID3D11RasterizerState;
#if DXGL_VIRTUAL_DEVICE_AND_CONTEXT
struct ID3D11Device;
struct ID3D11DeviceContext;
#else
typedef class CCryDXGLDevice ID3D11Device;
typedef class CCryDXGLDevice IDXGIDevice;
typedef class CCryDXGLDeviceContext ID3D11DeviceContext;
#endif

#endif //!DXGL_FULL_EMULATION

#if defined (DXGL_BLOB_INTEROPERABILITY) || DXGL_FULL_EMULATION
typedef struct ID3D10Blob ID3DBlob;
#else
typedef class CCryDXGLBlob ID3D10Blob;
typedef class CCryDXGLBlob ID3DBlob;
#endif

#include "ICryDXGLUnknown.hpp"
#include "DXGL_D3D11.h"
#include "DXGL_D3DX11.h"
#include "DXGL_D3DCompiler.h"


////////////////////////////////////////////////////////////////////////////
//  Helper functions
////////////////////////////////////////////////////////////////////////////

inline UINT D3D11CalcSubresource(UINT MipSlice, UINT ArraySlice, UINT MipLevels)
{
    return MipSlice + ArraySlice * MipLevels;
}


#include "CryDXGLLegacy.hpp"

#endif //__CRYDXGLMISC__
