#pragma once
#include <cstdint>
using UINT=unsigned; using ULONG=unsigned; using HRESULT=int;
struct SRWLOCK {}; inline void AcquireSRWLockExclusive(SRWLOCK*) {} inline void ReleaseSRWLockExclusive(SRWLOCK*) {}
inline void AcquireSRWLockShared(SRWLOCK*) {} inline void ReleaseSRWLockShared(SRWLOCK*) {}
#define SRWLOCK_INIT {}
#define __uuidof(x) 0
enum DXGI_FORMAT { DXGI_FORMAT_UNKNOWN, DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
 DXGI_FORMAT_B8G8R8A8_UNORM, DXGI_FORMAT_B8G8R8A8_UNORM_SRGB, DXGI_FORMAT_BC1_UNORM, DXGI_FORMAT_BC1_UNORM_SRGB,
 DXGI_FORMAT_BC2_UNORM, DXGI_FORMAT_BC2_UNORM_SRGB, DXGI_FORMAT_BC3_UNORM, DXGI_FORMAT_BC3_UNORM_SRGB };
enum { D3D11_SRV_DIMENSION_UNKNOWN, D3D11_SRV_DIMENSION_TEXTURE2D };
enum { D3D11_BIND_RENDER_TARGET=0x20 };
struct D3D11_SHADER_RESOURCE_VIEW_DESC { DXGI_FORMAT Format=DXGI_FORMAT_UNKNOWN; int ViewDimension=0; struct { UINT MostDetailedMip=0; } Texture2D; };
struct D3D11_TEXTURE2D_DESC { UINT Width=0,Height=0,ArraySize=0,BindFlags=0; struct {UINT Count=0;} SampleDesc; };
struct ID3D11Texture2D;
struct ID3D11Resource { virtual ULONG AddRef()=0; virtual ULONG Release()=0; virtual HRESULT QueryInterface(int,void**)=0; };
struct ID3D11Texture2D: ID3D11Resource { virtual void GetDesc(D3D11_TEXTURE2D_DESC*)=0; };
struct ID3D11ShaderResourceView { virtual ULONG AddRef()=0; virtual ULONG Release()=0; virtual void GetDesc(D3D11_SHADER_RESOURCE_VIEW_DESC*)=0; virtual void GetResource(ID3D11Resource**)=0; };
struct ID3D11Device { virtual ULONG AddRef()=0; virtual ULONG Release()=0; };
struct ID3D11DeviceContext { virtual void GetDevice(ID3D11Device**)=0; virtual void PSGetShaderResources(UINT,UINT,ID3D11ShaderResourceView**)=0; };
