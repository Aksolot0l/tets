#pragma once
// Opt-in, best-effort copy of *GPU draws* made by the original local-player
// renderer. It does not call that renderer again. The image has the game's
// current camera, not an independent front view. Experimental, unverified.
#ifdef VISUAL_PLAYER_REPLAY_TEST_TYPES
#include VISUAL_PLAYER_REPLAY_TEST_TYPES
#else
#include <windows.h>
#include <d3d11.h>
#endif
#include <atomic>
#include <cstdint>
#include "visual_model_capture_policy.h"
namespace visual_player_replay {
template<class T> inline void release(T*& p) { if (p) { p->Release(); p=nullptr; } }
struct Target { ID3D11Texture2D* color=nullptr; ID3D11RenderTargetView* rtv=nullptr; ID3D11ShaderResourceView* srv=nullptr; ID3D11Texture2D* depth=nullptr; ID3D11DepthStencilView* dsv=nullptr; };
inline void release(Target& t) { release(t.rtv); release(t.srv); release(t.color); release(t.dsv); release(t.depth); }
inline constexpr UINT size=256;
inline std::atomic<bool> requested{false};
inline SRWLOCK lock=SRWLOCK_INIT;
inline Target targets[2]{};
inline ID3D11Device* device=nullptr;
inline std::uintptr_t actor=0;
inline std::uint64_t generation=0;
inline int front=-1;
inline std::uint64_t lastFrameTick=0;
inline bool creationFailed=false;
inline std::atomic<unsigned> lastDraws{0}, skipped{0};
inline void clearLocked() { for (auto& t:targets) release(t); release(device); actor=0; front=-1; lastFrameTick=0; creationFailed=false; ++generation; lastDraws.store(0); }
inline void setRequested(bool value) {
 bool old=requested.exchange(value,std::memory_order_acq_rel);
 if (old && !value) { AcquireSRWLockExclusive(&lock); clearLocked(); ReleaseSRWLockExclusive(&lock); }
}
inline bool isRequested() { return requested.load(std::memory_order_acquire); }
inline bool createTarget(ID3D11Device* dev,Target& t) {
 D3D11_TEXTURE2D_DESC d{}; d.Width=size; d.Height=size; d.MipLevels=1; d.ArraySize=1;
 d.Format=DXGI_FORMAT_R8G8B8A8_UNORM; d.SampleDesc.Count=1; d.Usage=D3D11_USAGE_DEFAULT; d.BindFlags=D3D11_BIND_RENDER_TARGET|D3D11_BIND_SHADER_RESOURCE;
 if (FAILED(dev->CreateTexture2D(&d,nullptr,&t.color)) || FAILED(dev->CreateRenderTargetView(t.color,nullptr,&t.rtv)) || FAILED(dev->CreateShaderResourceView(t.color,nullptr,&t.srv))) { release(t); return false; }
 d.Format=DXGI_FORMAT_D24_UNORM_S8_UINT; d.BindFlags=D3D11_BIND_DEPTH_STENCIL;
 if (FAILED(dev->CreateTexture2D(&d,nullptr,&t.depth)) || FAILED(dev->CreateDepthStencilView(t.depth,nullptr,&t.dsv))) { release(t); return false; }
 return true;
}
// Our private SRVs must not be bound for reading when used as render targets.
// Check all six shader stages once per player pass, before any clear or replay.
inline bool srvBound(ID3D11DeviceContext* c,ID3D11ShaderResourceView* target) {
 using Get = void (STDMETHODCALLTYPE ID3D11DeviceContext::*)(UINT,UINT,ID3D11ShaderResourceView**);
 const Get getters[]={&ID3D11DeviceContext::VSGetShaderResources,&ID3D11DeviceContext::HSGetShaderResources,&ID3D11DeviceContext::DSGetShaderResources,&ID3D11DeviceContext::GSGetShaderResources,&ID3D11DeviceContext::PSGetShaderResources,&ID3D11DeviceContext::CSGetShaderResources};
 bool found=false;
 for (auto get:getters) {
  ID3D11ShaderResourceView* views[D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT]{};
  (c->*get)(0,D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT,views);
  for (auto*& v:views) { if (v==target) found=true; release(v); }
 }
 return found;
}
struct Scope { ID3D11DeviceContext* ctx=nullptr; ID3D11RenderTargetView* rtv=nullptr; ID3D11DepthStencilView* dsv=nullptr; ID3D11ShaderResourceView* srv=nullptr; int index=-1; std::uint64_t gen=0; unsigned draws=0; bool depthCleared=false, reversedDepth=false; };
inline thread_local Scope scope;
inline void end();
inline bool outputBound(ID3D11DeviceContext* c,ID3D11RenderTargetView* target,ID3D11DepthStencilView* depth) {
 ID3D11RenderTargetView* views[D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT]{}; ID3D11DepthStencilView* dsv=nullptr;
 c->OMGetRenderTargets(D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT,views,&dsv);
 bool found=dsv==depth; for (auto*& v:views) { found|=v==target; release(v); } release(dsv); return found;
}
inline void begin(std::uintptr_t player,ID3D11DeviceContext* ctx) {
 end(); if (!isRequested() || !player || !ctx) return;
 ID3D11Device* current=nullptr; ctx->GetDevice(&current); if (!current) return;
 AcquireSRWLockExclusive(&lock);
 if (isRequested()) {
  if (device!=current || actor!=player) { clearLocked(); device=current; current=nullptr; actor=player; }
  if (!creationFailed && (!targets[0].rtv || !targets[1].rtv)) {
   release(targets[0]); release(targets[1]);
   if (!createTarget(device,targets[0]) || !createTarget(device,targets[1])) { release(targets[0]); release(targets[1]); creationFailed=true; }
  }
  if (targets[0].rtv && targets[1].rtv) {
   const int index=front==0?1:0; auto& t=targets[index];
   scope.ctx=ctx; scope.ctx->AddRef(); scope.rtv=t.rtv; scope.dsv=t.dsv; scope.srv=t.srv; scope.index=index; scope.gen=generation;
   scope.rtv->AddRef(); scope.dsv->AddRef(); scope.srv->AddRef();
  }
 }
 ReleaseSRWLockExclusive(&lock); release(current);
 if (!scope.ctx) return;
 if (srvBound(ctx,scope.srv) || outputBound(ctx,scope.rtv,scope.dsv)) { end(); return; }
 const FLOAT transparent[4]={0,0,0,0}; ctx->ClearRenderTargetView(scope.rtv,transparent);
}
inline void end() {
 if (scope.ctx && scope.draws) {
  AcquireSRWLockExclusive(&lock);
  if (isRequested() && scope.gen==generation) { front=scope.index; lastFrameTick=GetTickCount64(); lastDraws.store(scope.draws,std::memory_order_relaxed); }
  ReleaseSRWLockExclusive(&lock);
 }
 release(scope.rtv); release(scope.dsv); release(scope.srv); release(scope.ctx); scope={};
}
inline ID3D11ShaderResourceView* acquire(ID3D11Device* expected) {
 ID3D11ShaderResourceView* result=nullptr; AcquireSRWLockShared(&lock);
 const auto now=GetTickCount64();
 if (isRequested() && expected && device==expected && front>=0 && lastFrameTick && now>=lastFrameTick && now-lastFrameTick<=2000) { result=targets[front].srv; if(result)result->AddRef(); }
 ReleaseSRWLockShared(&lock); return result;
}
struct DrawState {
 ID3D11DeviceContext* ctx=nullptr;
 ID3D11RenderTargetView* rtv[D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT]{};
 ID3D11DepthStencilView* dsv=nullptr;
 D3D11_VIEWPORT viewport{};
 D3D11_RECT scissors[D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE]{};
 UINT scissorCount=0;
 bool active=false;
};
inline thread_local DrawState saved;
inline bool beginDraw(ID3D11DeviceContext* ctx) {
 if (!isRequested() || !scope.ctx || ctx!=scope.ctx || saved.active || scope.draws>=128) return false;
 UINT count=0; ctx->RSGetViewports(&count,nullptr);
 D3D11_VIEWPORT vp{}; if (count==1) { UINT one=1; ctx->RSGetViewports(&one,&vp); if(one!=1)count=0; }
 visual_model_capture_policy::Viewport input{vp.TopLeftX,vp.TopLeftY,vp.Width,vp.Height,vp.MinDepth,vp.MaxDepth}, output{};
 const bool validVp=count==1 && visual_model_capture_policy::viewport(input,static_cast<float>(size),output);
 ID3D11DepthStencilState* depth=nullptr; UINT stencil=0; ctx->OMGetDepthStencilState(&depth,&stencil);
 D3D11_DEPTH_STENCIL_DESC d{}; if (depth) depth->GetDesc(&d); else { d.DepthEnable=TRUE; d.DepthFunc=D3D11_COMPARISON_LESS; }
 release(depth);
 const bool depthOk=d.DepthEnable && (d.DepthFunc==D3D11_COMPARISON_LESS || d.DepthFunc==D3D11_COMPARISON_LESS_EQUAL || d.DepthFunc==D3D11_COMPARISON_GREATER || d.DepthFunc==D3D11_COMPARISON_GREATER_EQUAL);
 ID3D11RenderTargetView* old[D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT]{}; ID3D11DepthStencilView* oldDsv=nullptr;
 ctx->OMGetRenderTargets(D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT,old,&oldDsv);
 ID3D11Buffer* streams[D3D11_SO_BUFFER_SLOT_COUNT]{}; ctx->SOGetTargets(D3D11_SO_BUFFER_SLOT_COUNT,streams);
 bool so=false; for(auto*& p:streams) { so|=p!=nullptr; release(p); }
 ID3D11Predicate* pred=nullptr; BOOL predValue=FALSE; ctx->GetPredication(&pred,&predValue); const bool predicate=pred!=nullptr; release(pred);
 ID3D11Device* dev=nullptr; ctx->GetDevice(&dev); const bool deviceReady=dev!=nullptr;
 UINT slots=dev && dev->GetFeatureLevel()>=D3D_FEATURE_LEVEL_11_1 ? 64u : 8u; release(dev);
 ID3D11UnorderedAccessView* uavs[64]{}; ctx->OMGetRenderTargetsAndUnorderedAccessViews(0,nullptr,nullptr,0,slots,uavs);
 bool uav=false; for (UINT i=0;i<slots;++i) { uav|=uavs[i]!=nullptr; release(uavs[i]); }
 const bool safe=visual_model_capture_policy::safeDraw(validVp && deviceReady,depthOk && oldDsv!=nullptr,d.StencilEnable!=FALSE,uav,so,predicate,count);
 if (!safe) { for(auto*& p:old)release(p); release(oldDsv); skipped.fetch_add(1,std::memory_order_relaxed); return false; }
 const bool reversed=d.DepthFunc==D3D11_COMPARISON_GREATER || d.DepthFunc==D3D11_COMPARISON_GREATER_EQUAL;
 if (scope.depthCleared && reversed!=scope.reversedDepth) {
  for (auto*& p:old) release(p);
  release(oldDsv); skipped.fetch_add(1,std::memory_order_relaxed); return false;
 }
 if (!scope.depthCleared) {
  ctx->ClearDepthStencilView(scope.dsv,D3D11_CLEAR_DEPTH,reversed?0.f:1.f,0); scope.depthCleared=true; scope.reversedDepth=reversed;
 }
 saved.ctx=ctx; for (UINT i=0;i<D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT;++i) saved.rtv[i]=old[i]; saved.dsv=oldDsv; saved.viewport=vp;
 saved.scissorCount=D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE; ctx->RSGetScissorRects(&saved.scissorCount,saved.scissors);
 D3D11_VIEWPORT dst{output.x,output.y,output.width,output.height,output.minDepth,output.maxDepth};
 const D3D11_RECT rect{0,0,static_cast<LONG>(size),static_cast<LONG>(size)};
 ctx->OMSetRenderTargets(1,&scope.rtv,scope.dsv); ctx->RSSetViewports(1,&dst); ctx->RSSetScissorRects(1,&rect); saved.active=true;
 return true;
}
inline void endDraw(ID3D11DeviceContext* ctx) {
 if (!saved.active || ctx!=saved.ctx) return;
 ctx->OMSetRenderTargets(D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT,saved.rtv,saved.dsv);
 ctx->RSSetViewports(1,&saved.viewport); ctx->RSSetScissorRects(saved.scissorCount,saved.scissorCount?saved.scissors:nullptr);
 for (auto*& p:saved.rtv) release(p);
 release(saved.dsv); saved={}; ++scope.draws;
}
}
