#pragma once
// Read-only candidate discovery while the original local-player renderer is
// executing. Does NOT invoke the renderer again, alter D3D state or read pixels
// back to the CPU. The selected view remains referenced until the UI closes.
#ifdef VISUAL_PLAYER_SKIN_TEST_TYPES
#include VISUAL_PLAYER_SKIN_TEST_TYPES
#else
#include <windows.h>
#include <d3d11.h>
#endif
#include <atomic>
#include <cstddef>
#include <cstdint>
#include "visual_skin_policy.h"
namespace visual_player_skin {
struct Candidate { ID3D11ShaderResourceView* view=nullptr; ID3D11Resource* resource=nullptr; UINT width=0,height=0; };
struct Snapshot { ID3D11ShaderResourceView* view=nullptr; UINT width=0,height=0,index=0,count=0; };
inline std::atomic<bool> requested{false};
inline SRWLOCK lock=SRWLOCK_INIT;
inline Candidate candidates[8]{};
inline UINT count=0,selected=0;
inline std::uintptr_t actor=0;
inline ID3D11Device* device=nullptr;
inline std::uint64_t generation=0;
inline thread_local std::uint64_t scopeGeneration=0;
inline thread_local ID3D11DeviceContext* scopeContext=nullptr;
inline void clearLocked() {
 for (auto& c:candidates) { if(c.view)c.view->Release(); if(c.resource)c.resource->Release(); c={}; }
 count=selected=0; actor=0; ++generation; if(device)device->Release(); device=nullptr;
}
inline void setRequested(bool value) {
 bool was=requested.exchange(value,std::memory_order_acq_rel);
 if (was && !value) { AcquireSRWLockExclusive(&lock); clearLocked(); ReleaseSRWLockExclusive(&lock); }
}
inline bool isRequested() { return requested.load(std::memory_order_acquire); }
inline void begin(std::uintptr_t player,ID3D11DeviceContext* ctx) {
 scopeContext=nullptr;
 if (!isRequested() || !ctx || !player) return;
 ID3D11Device* current=nullptr; ctx->GetDevice(&current); if (!current) return;
 bool accepted=false;
 AcquireSRWLockExclusive(&lock);
 if (isRequested()) {
  if (actor!=player || device!=current) { clearLocked(); actor=player; device=current; current=nullptr; }
  scopeGeneration=generation; accepted=true;
 }
 ReleaseSRWLockExclusive(&lock);
 if(current)current->Release();
 if (accepted) scopeContext=ctx;
}
inline void end() { scopeContext=nullptr; }
inline bool colorFormat(DXGI_FORMAT f) {
 switch (f) {
 case DXGI_FORMAT_R8G8B8A8_UNORM: case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
 case DXGI_FORMAT_B8G8R8A8_UNORM: case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
 case DXGI_FORMAT_BC1_UNORM: case DXGI_FORMAT_BC1_UNORM_SRGB:
 case DXGI_FORMAT_BC2_UNORM: case DXGI_FORMAT_BC2_UNORM_SRGB:
 case DXGI_FORMAT_BC3_UNORM: case DXGI_FORMAT_BC3_UNORM_SRGB: return true;
 default: return false;
 }
}
inline void observe(ID3D11DeviceContext* ctx) {
 if (!isRequested() || !ctx || ctx!=scopeContext) return;
 ID3D11ShaderResourceView* views[4]{}; ctx->PSGetShaderResources(0,4,views);
 for (auto* view:views) {
  if (!view) continue;
  D3D11_SHADER_RESOURCE_VIEW_DESC vd{}; view->GetDesc(&vd);
  ID3D11Resource* res=nullptr; view->GetResource(&res);
  ID3D11Texture2D* tex=nullptr;
  if (res && vd.ViewDimension==D3D11_SRV_DIMENSION_TEXTURE2D && vd.Texture2D.MostDetailedMip==0 && colorFormat(vd.Format))
   res->QueryInterface(__uuidof(ID3D11Texture2D),reinterpret_cast<void**>(&tex));
  if (tex) {
   D3D11_TEXTURE2D_DESC d{}; tex->GetDesc(&d); tex->Release();
   if (visual_skin_policy::dimensions(d.Width,d.Height,d.SampleDesc.Count,d.ArraySize) && !(d.BindFlags&D3D11_BIND_RENDER_TARGET)) {
    AcquireSRWLockExclusive(&lock);
    bool exists=false; for (UINT i=0;i<count;i++) if (candidates[i].resource==res) { exists=true; break; }
    if (isRequested() && scopeGeneration==generation && !exists && count<8) {
     view->AddRef(); res->AddRef(); candidates[count++]={view,res,d.Width,d.Height};
    }
    ReleaseSRWLockExclusive(&lock);
   }
  }
  if (res) res->Release();
  view->Release();
 }
}
// Caller must Release snapshot.view after ImGui has rendered the frame.
inline Snapshot acquire(ID3D11Device* expected) {
 Snapshot s{}; AcquireSRWLockShared(&lock);
 if (isRequested() && device && device==expected && count) {
  const auto& c=candidates[selected%count]; s={c.view,c.width,c.height,selected%count,count};
  if(s.view)s.view->AddRef();
 }
 ReleaseSRWLockShared(&lock); return s;
}
inline void next() { AcquireSRWLockExclusive(&lock); if (count) selected=(selected+1)%count; ReleaseSRWLockExclusive(&lock); }
}
