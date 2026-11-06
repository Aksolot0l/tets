#pragma once
// Experimental D3D11 gate. Draw hooks are global but touch state ONLY during
// the adapter's scoped tessDraw on the same immediate context. Missing or
// mismatched world depth fails closed. Original depth/blend is restored after
// each draw. This still requires validation in the actual game.
#ifdef VISUAL_GPU_DEPTH_TEST_TYPES
#include VISUAL_GPU_DEPTH_TEST_TYPES
#else
#include <windows.h>
#include <d3d11.h>
#endif
#include <atomic>
#include "visual_hooks.h"
namespace visual_gpu_depth {
using Draw = void (STDMETHODCALLTYPE*)(ID3D11DeviceContext*,UINT,UINT);
using DrawIndexed = void (STDMETHODCALLTYPE*)(ID3D11DeviceContext*,UINT,UINT,INT);
using DrawInstanced = void (STDMETHODCALLTYPE*)(ID3D11DeviceContext*,UINT,UINT,UINT,UINT);
using DrawIndexedInstanced = void (STDMETHODCALLTYPE*)(ID3D11DeviceContext*,UINT,UINT,UINT,INT,UINT);
using DrawAuto = void (STDMETHODCALLTYPE*)(ID3D11DeviceContext*);
using DrawIndirect = void (STDMETHODCALLTYPE*)(ID3D11DeviceContext*,ID3D11Buffer*,UINT);
inline Draw originalDraw=nullptr;
inline DrawIndexed originalIndexed=nullptr;
inline DrawInstanced originalInstanced=nullptr;
inline DrawIndexedInstanced originalIndexedInstanced=nullptr;
inline DrawAuto originalAuto=nullptr;
inline DrawIndirect originalIndirect=nullptr, originalIndexedIndirect=nullptr;
inline std::atomic<unsigned> submitted{0}, rejected{0};
inline void* installedTargets[7]{};
inline constexpr std::size_t drawSlots[]={12,13,20,21,38,39,40};
inline thread_local bool inVisualDraw=false;
inline thread_local ID3D11DeviceContext* expectedContext=nullptr;
inline thread_local ID3D11DepthStencilView* expectedDepth=nullptr;
inline thread_local D3D11_COMPARISON_FUNC worldCompare=D3D11_COMPARISON_LESS_EQUAL;
inline thread_local float opacity=1.f;

template<class T> void release(T*& p) { if (p) { p->Release(); p=nullptr; } }
struct Guard {
 ID3D11DeviceContext* ctx=nullptr;
 ID3D11DepthStencilState* oldDepth=nullptr; ID3D11DepthStencilState* newDepth=nullptr;
 ID3D11BlendState* oldBlend=nullptr; ID3D11BlendState* newBlend=nullptr;
 UINT stencil=0, sampleMask=0xffffffffu; FLOAT oldFactor[4]{};
 bool active=false;
 explicit Guard(ID3D11DeviceContext* c) : ctx(c) {
  if (!ctx || ctx!=expectedContext || !expectedDepth) return;
  ID3D11DepthStencilView* current=nullptr; ctx->OMGetRenderTargets(0,nullptr,&current);
  bool same=current && current==expectedDepth; release(current); if (!same) return;
  ctx->OMGetDepthStencilState(&oldDepth,&stencil);
  ctx->OMGetBlendState(&oldBlend,oldFactor,&sampleMask);
  D3D11_DEPTH_STENCIL_DESC depth{};
  if (oldDepth) oldDepth->GetDesc(&depth);
  else {
   depth.StencilReadMask=D3D11_DEFAULT_STENCIL_READ_MASK; depth.StencilWriteMask=D3D11_DEFAULT_STENCIL_WRITE_MASK;
   depth.FrontFace={D3D11_STENCIL_OP_KEEP,D3D11_STENCIL_OP_KEEP,D3D11_STENCIL_OP_KEEP,D3D11_COMPARISON_ALWAYS};
   depth.BackFace=depth.FrontFace;
  }
  depth.DepthEnable=TRUE; depth.DepthWriteMask=D3D11_DEPTH_WRITE_MASK_ZERO;
  depth.DepthFunc=worldCompare; depth.StencilEnable=FALSE;
  D3D11_BLEND_DESC blend{};
  if (oldBlend) oldBlend->GetDesc(&blend);
  else for (auto& target:blend.RenderTarget) {
   target.SrcBlend=D3D11_BLEND_ONE; target.DestBlend=D3D11_BLEND_ZERO; target.BlendOp=D3D11_BLEND_OP_ADD;
   target.SrcBlendAlpha=D3D11_BLEND_ONE; target.DestBlendAlpha=D3D11_BLEND_ZERO; target.BlendOpAlpha=D3D11_BLEND_OP_ADD;
   target.RenderTargetWriteMask=D3D11_COLOR_WRITE_ENABLE_ALL;
  }
  auto& target=blend.RenderTarget[0]; target.BlendEnable=TRUE;
  target.SrcBlend=D3D11_BLEND_BLEND_FACTOR; target.DestBlend=D3D11_BLEND_INV_BLEND_FACTOR;
  target.BlendOp=D3D11_BLEND_OP_ADD; target.SrcBlendAlpha=D3D11_BLEND_ONE;
  target.DestBlendAlpha=D3D11_BLEND_INV_SRC_ALPHA; target.BlendOpAlpha=D3D11_BLEND_OP_ADD;
  ID3D11Device* device=nullptr; ctx->GetDevice(&device);
  if (device) {
   HRESULT d=device->CreateDepthStencilState(&depth,&newDepth);
   HRESULT b=SUCCEEDED(d) ? device->CreateBlendState(&blend,&newBlend) : E_FAIL;
   release(device);
   if (SUCCEEDED(d) && SUCCEEDED(b) && newDepth && newBlend) {
    const FLOAT factor[4]={opacity,opacity,opacity,opacity};
    ctx->OMSetDepthStencilState(newDepth,stencil);
    ctx->OMSetBlendState(newBlend,factor,sampleMask);
    active=true;
   }
  }
 }
 ~Guard() {
  if (active) { ctx->OMSetDepthStencilState(oldDepth,stencil); ctx->OMSetBlendState(oldBlend,oldFactor,sampleMask); }
  release(newBlend); release(oldBlend); release(newDepth); release(oldDepth);
 }
};
template<class Call> void draw(ID3D11DeviceContext* ctx,Call call) {
 if (!inVisualDraw) { call(); return; }
 Guard guard(ctx);
 if (!guard.active) { rejected.fetch_add(1,std::memory_order_relaxed); return; }
 submitted.fetch_add(1,std::memory_order_relaxed); call();
}
inline void STDMETHODCALLTYPE onDraw(ID3D11DeviceContext* c,UINT n,UINT start) {
 draw(c,[&]{ originalDraw(c,n,start); });
}
inline void STDMETHODCALLTYPE onIndexed(ID3D11DeviceContext* c,UINT n,UINT start,INT base) {
 draw(c,[&]{ originalIndexed(c,n,start,base); });
}
inline void STDMETHODCALLTYPE onInstanced(ID3D11DeviceContext* c,UINT n,UINT instances,UINT start,UINT first) {
 draw(c,[&]{ originalInstanced(c,n,instances,start,first); });
}
inline void STDMETHODCALLTYPE onIndexedInstanced(ID3D11DeviceContext* c,UINT n,UINT instances,UINT start,INT base,UINT first) {
 draw(c,[&]{ originalIndexedInstanced(c,n,instances,start,base,first); });
}
inline void STDMETHODCALLTYPE onAuto(ID3D11DeviceContext* c) { draw(c,[&]{ originalAuto(c); }); }
inline void STDMETHODCALLTYPE onIndirect(ID3D11DeviceContext* c,ID3D11Buffer* b,UINT off) {
 draw(c,[&]{ originalIndirect(c,b,off); });
}
inline void STDMETHODCALLTYPE onIndexedIndirect(ID3D11DeviceContext* c,ID3D11Buffer* b,UINT off) {
 draw(c,[&]{ originalIndexedIndirect(c,b,off); });
}
template<class Create,class Enable,class Disable,class Remove>
bool install(ID3D11DeviceContext* ctx,Create create,Enable enable,Disable disable,Remove remove) {
 if (!ctx) return false;
 auto** vtable=*reinterpret_cast<void***>(ctx);
 if (!vtable) return false;
 // ID3D11DeviceContext vtable indices from the D3D11 interface declaration.
 visual_hooks::Hook hooks[]={
  {vtable[12],reinterpret_cast<void*>(&onIndexed),reinterpret_cast<void**>(&originalIndexed)},
  {vtable[13],reinterpret_cast<void*>(&onDraw),reinterpret_cast<void**>(&originalDraw)},
  {vtable[20],reinterpret_cast<void*>(&onIndexedInstanced),reinterpret_cast<void**>(&originalIndexedInstanced)},
  {vtable[21],reinterpret_cast<void*>(&onInstanced),reinterpret_cast<void**>(&originalInstanced)},
  {vtable[38],reinterpret_cast<void*>(&onAuto),reinterpret_cast<void**>(&originalAuto)},
  {vtable[39],reinterpret_cast<void*>(&onIndexedIndirect),reinterpret_cast<void**>(&originalIndexedIndirect)},
  {vtable[40],reinterpret_cast<void*>(&onIndirect),reinterpret_cast<void**>(&originalIndirect)}
 };
 for (std::size_t i=0;i<7;++i) {
  if (!hooks[i].target) return false;
  for (std::size_t j=0;j<i;++j) if (hooks[i].target==hooks[j].target) return false;
 }
 if (!visual_hooks::install(hooks,7,create,enable,disable,remove)) return false;
 for (std::size_t i=0;i<7;++i) installedTargets[i]=hooks[i].target;
 return true;
}
inline bool matches(ID3D11DeviceContext* ctx) {
 if (!ctx) return false;
 auto** vtable=*reinterpret_cast<void***>(ctx);
 if (!vtable) return false;
 for (std::size_t i=0;i<7;++i) if (!installedTargets[i] || vtable[drawSlots[i]]!=installedTargets[i]) return false;
 return true;
}
}
