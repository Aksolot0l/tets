#pragma once
#include <cstdint>
inline std::uint64_t mockTick=100; inline std::uint64_t GetTickCount64() { return mockTick; }
using UINT=unsigned; using LONG=long; using BOOL=int; using FLOAT=float; using ULONG=unsigned long; using HRESULT=int;
#define STDMETHODCALLTYPE
#define TRUE 1
#define FALSE 0
#define FAILED(x) ((x)<0)
struct SRWLOCK { int unused=0; };
#define SRWLOCK_INIT {}
inline void AcquireSRWLockExclusive(SRWLOCK*){} inline void ReleaseSRWLockExclusive(SRWLOCK*){}
inline void AcquireSRWLockShared(SRWLOCK*){} inline void ReleaseSRWLockShared(SRWLOCK*){}
constexpr UINT D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT=128,D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT=8,D3D11_SO_BUFFER_SLOT_COUNT=4,D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE=16;
constexpr UINT D3D11_BIND_RENDER_TARGET=0x20,D3D11_BIND_SHADER_RESOURCE=8,D3D11_BIND_DEPTH_STENCIL=0x40,D3D11_CLEAR_DEPTH=1;
enum DXGI_FORMAT { DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_FORMAT_D24_UNORM_S8_UINT };
enum D3D11_COMPARISON_FUNC { D3D11_COMPARISON_LESS=2,D3D11_COMPARISON_LESS_EQUAL=4,D3D11_COMPARISON_GREATER=5,D3D11_COMPARISON_GREATER_EQUAL=7 };
enum D3D_FEATURE_LEVEL { D3D_FEATURE_LEVEL_11_0=0xb000,D3D_FEATURE_LEVEL_11_1=0xb100 };
constexpr int D3D11_USAGE_DEFAULT=0;
struct D3D11_TEXTURE2D_DESC {UINT Width=0,Height=0,MipLevels=0,ArraySize=0; DXGI_FORMAT Format{}; struct {UINT Count=0,Quality=0;} SampleDesc; int Usage=0; UINT BindFlags=0,CPUAccessFlags=0,MiscFlags=0;};
struct D3D11_DEPTH_STENCIL_DESC { BOOL DepthEnable=0; D3D11_COMPARISON_FUNC DepthFunc{}; BOOL StencilEnable=0;};
struct D3D11_VIEWPORT {float TopLeftX,TopLeftY,Width,Height,MinDepth,MaxDepth;};
struct D3D11_RECT {LONG left,top,right,bottom;};
struct Base { virtual ULONG AddRef()=0; virtual ULONG Release()=0; protected: virtual ~Base()=default; };
struct ID3D11Texture2D:Base{}; struct ID3D11RenderTargetView:Base{}; struct ID3D11ShaderResourceView:Base{}; struct ID3D11DepthStencilView:Base{};
struct ID3D11Buffer:Base{}; struct ID3D11Predicate:Base{}; struct ID3D11UnorderedAccessView:Base{};
struct ID3D11DepthStencilState:Base {virtual void GetDesc(D3D11_DEPTH_STENCIL_DESC*)=0;};
struct ID3D11Device:Base {
 virtual HRESULT CreateTexture2D(const D3D11_TEXTURE2D_DESC*,void*,ID3D11Texture2D**)=0;
 virtual HRESULT CreateRenderTargetView(ID3D11Texture2D*,void*,ID3D11RenderTargetView**)=0;
 virtual HRESULT CreateShaderResourceView(ID3D11Texture2D*,void*,ID3D11ShaderResourceView**)=0;
 virtual HRESULT CreateDepthStencilView(ID3D11Texture2D*,void*,ID3D11DepthStencilView**)=0;
 virtual D3D_FEATURE_LEVEL GetFeatureLevel()=0;
};
struct ID3D11DeviceContext {
 virtual ULONG AddRef()=0;
 virtual ULONG Release()=0;
 virtual void VSGetShaderResources(UINT,UINT,ID3D11ShaderResourceView**)=0;
 virtual void HSGetShaderResources(UINT,UINT,ID3D11ShaderResourceView**)=0;
 virtual void DSGetShaderResources(UINT,UINT,ID3D11ShaderResourceView**)=0;
 virtual void GSGetShaderResources(UINT,UINT,ID3D11ShaderResourceView**)=0;
 virtual void PSGetShaderResources(UINT,UINT,ID3D11ShaderResourceView**)=0;
 virtual void CSGetShaderResources(UINT,UINT,ID3D11ShaderResourceView**)=0;
 virtual void GetDevice(ID3D11Device**)=0;
 virtual void ClearRenderTargetView(ID3D11RenderTargetView*,const FLOAT[4])=0;
 virtual void RSGetViewports(UINT*,D3D11_VIEWPORT*)=0;
 virtual void OMGetDepthStencilState(ID3D11DepthStencilState**,UINT*)=0;
 virtual void OMGetRenderTargets(UINT,ID3D11RenderTargetView**,ID3D11DepthStencilView**)=0;
 virtual void SOGetTargets(UINT,ID3D11Buffer**)=0;
 virtual void GetPredication(ID3D11Predicate**,BOOL*)=0;
 virtual void OMGetRenderTargetsAndUnorderedAccessViews(UINT,ID3D11RenderTargetView**,ID3D11DepthStencilView**,UINT,UINT,ID3D11UnorderedAccessView**)=0;
 virtual void ClearDepthStencilView(ID3D11DepthStencilView*,UINT,float,UINT)=0;
 virtual void RSGetScissorRects(UINT*,D3D11_RECT*)=0;
 virtual void OMSetRenderTargets(UINT,ID3D11RenderTargetView* const*,ID3D11DepthStencilView*)=0;
 virtual void RSSetViewports(UINT,const D3D11_VIEWPORT*)=0;
 virtual void RSSetScissorRects(UINT,const D3D11_RECT*)=0;
};
