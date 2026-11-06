#pragma once
// Minimal non-Windows COM-shaped API used only by visual_gpu_depth.cpp tests.
#include <cstdint>
#define STDMETHODCALLTYPE
using HRESULT=int; using UINT=unsigned; using INT=int; using FLOAT=float; using ULONG=unsigned;
constexpr HRESULT E_FAIL=-1;
#define SUCCEEDED(x) ((x)>=0)
#define FAILED(x) ((x)<0)
enum D3D11_COMPARISON_FUNC { D3D11_COMPARISON_NEVER=1,D3D11_COMPARISON_LESS=2,D3D11_COMPARISON_EQUAL=3,D3D11_COMPARISON_LESS_EQUAL=4,D3D11_COMPARISON_GREATER=5,D3D11_COMPARISON_GREATER_EQUAL=7,D3D11_COMPARISON_ALWAYS=8 };
enum { D3D11_DEPTH_WRITE_MASK_ZERO=0, D3D11_STENCIL_OP_KEEP=1,
 D3D11_DEFAULT_STENCIL_READ_MASK=255, D3D11_DEFAULT_STENCIL_WRITE_MASK=255,
 D3D11_BLEND_ZERO=1,D3D11_BLEND_ONE=2,D3D11_BLEND_INV_SRC_ALPHA=6,
 D3D11_BLEND_BLEND_FACTOR=14,D3D11_BLEND_INV_BLEND_FACTOR=15,D3D11_BLEND_OP_ADD=1,
 D3D11_COLOR_WRITE_ENABLE_ALL=15 };
constexpr int TRUE=1, FALSE=0;
struct D3D11_DEPTH_STENCILOP_DESC { int StencilFailOp,StencilDepthFailOp,StencilPassOp; D3D11_COMPARISON_FUNC StencilFunc; };
struct D3D11_DEPTH_STENCIL_DESC { int DepthEnable,DepthWriteMask; D3D11_COMPARISON_FUNC DepthFunc; int StencilEnable,StencilReadMask,StencilWriteMask; D3D11_DEPTH_STENCILOP_DESC FrontFace,BackFace; };
struct D3D11_RENDER_TARGET_BLEND_DESC { int BlendEnable,SrcBlend,DestBlend,BlendOp,SrcBlendAlpha,DestBlendAlpha,BlendOpAlpha,RenderTargetWriteMask; };
struct D3D11_BLEND_DESC { int AlphaToCoverageEnable,IndependentBlendEnable; D3D11_RENDER_TARGET_BLEND_DESC RenderTarget[8]; };
struct ID3D11DepthStencilView { virtual ULONG AddRef()=0; virtual ULONG Release()=0; };
struct ID3D11DepthStencilState { virtual ~ID3D11DepthStencilState() = default; virtual void GetDesc(D3D11_DEPTH_STENCIL_DESC*)=0; virtual ULONG AddRef()=0; virtual ULONG Release()=0; };
struct ID3D11BlendState { virtual ~ID3D11BlendState() = default; virtual void GetDesc(D3D11_BLEND_DESC*)=0; virtual ULONG AddRef()=0; virtual ULONG Release()=0; };
struct ID3D11Device {
 virtual HRESULT CreateDepthStencilState(const D3D11_DEPTH_STENCIL_DESC*,ID3D11DepthStencilState**)=0;
 virtual HRESULT CreateBlendState(const D3D11_BLEND_DESC*,ID3D11BlendState**)=0;
 virtual ULONG Release()=0;
};
struct ID3D11DeviceContext {
 virtual void OMGetRenderTargets(UINT,void*,ID3D11DepthStencilView**)=0;
 virtual void OMGetDepthStencilState(ID3D11DepthStencilState**,UINT*)=0;
 virtual void OMGetBlendState(ID3D11BlendState**,FLOAT*,UINT*)=0;
 virtual void GetDevice(ID3D11Device**)=0;
 virtual void OMSetDepthStencilState(ID3D11DepthStencilState*,UINT)=0;
 virtual void OMSetBlendState(ID3D11BlendState*,const FLOAT*,UINT)=0;
};
struct ID3D11Buffer {};
