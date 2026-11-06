#define VISUAL_GPU_DEPTH_TEST_TYPES "../tests/visual_gpu_mock_types.h"
#include "../src/visual_gpu_depth.h"
#include <cassert>
#include <cmath>
struct View:ID3D11DepthStencilView { unsigned refs=1; ULONG AddRef() override{return ++refs;} ULONG Release() override{return --refs;} };
struct Depth:ID3D11DepthStencilState {
 unsigned refs=1; D3D11_DEPTH_STENCIL_DESC desc{}; bool owned=false;
 void GetDesc(D3D11_DEPTH_STENCIL_DESC* d) override {*d=desc;} ULONG AddRef() override{return ++refs;}
 ULONG Release() override {auto n=--refs; if (!n && owned) delete this; return n;}
};
struct Blend:ID3D11BlendState {
 unsigned refs=1; D3D11_BLEND_DESC desc{}; bool owned=false;
 void GetDesc(D3D11_BLEND_DESC* d) override {*d=desc;} ULONG AddRef() override{return ++refs;}
 ULONG Release() override {auto n=--refs; if (!n && owned) delete this; return n;}
};
struct Device:ID3D11Device {
 bool failDepth=false, failBlend=false;
 HRESULT CreateDepthStencilState(const D3D11_DEPTH_STENCIL_DESC* d,ID3D11DepthStencilState** out) override {
  if (failDepth) return E_FAIL;
  auto* p=new Depth; p->desc=*d; p->owned=true; *out=p; return 0;
 }
 HRESULT CreateBlendState(const D3D11_BLEND_DESC* d,ID3D11BlendState** out) override {
  if (failBlend) return E_FAIL;
  auto* p=new Blend; p->desc=*d; p->owned=true; *out=p; return 0;
 }
 ULONG Release() override{return 1;}
};
struct Context:ID3D11DeviceContext {
 View view, other; Depth depth; Blend blend; Device device;
 ID3D11DepthStencilView* dsv=&view; ID3D11DepthStencilState* curDepth=&depth; ID3D11BlendState* curBlend=&blend;
 UINT stencil=7,mask=0xfffffffe; FLOAT factor[4]{.2f,.3f,.4f,.5f}; int sets=0; bool hasDevice=true;
 void OMGetRenderTargets(UINT,void*,ID3D11DepthStencilView** o) override {*o=dsv;if(*o)(*o)->AddRef();}
 void OMGetDepthStencilState(ID3D11DepthStencilState** o,UINT* r) override {*o=curDepth;if(*o)(*o)->AddRef();*r=stencil;}
 void OMGetBlendState(ID3D11BlendState** o,FLOAT* f,UINT* m) override {*o=curBlend;if(*o)(*o)->AddRef();for(int i=0;i<4;i++)f[i]=factor[i];*m=mask;}
 void GetDevice(ID3D11Device** o) override {*o=hasDevice?&device:nullptr;}
 void OMSetDepthStencilState(ID3D11DepthStencilState* p,UINT r) override {curDepth=p;stencil=r;sets++;}
 void OMSetBlendState(ID3D11BlendState* p,const FLOAT* f,UINT m) override {curBlend=p;mask=m;for(int i=0;i<4;i++)factor[i]=f[i];sets++;}
};
static int observed=0;
static void observePlayer(ID3D11DeviceContext*) { ++observed; }
int main() {
 using namespace visual_gpu_depth;
 Context ctx; int calls=0; inVisualDraw=false; playerObserver.store(&observePlayer);
 draw(&ctx,[&]{++calls;}); assert(calls==1 && ctx.sets==0 && observed==0);
 inPlayerCapture=true; draw(&ctx,[&]{++calls;}); assert(calls==2 && ctx.sets==0 && observed==1);
 inPlayerCapture=false; playerObserver.store(nullptr); calls=1;
 expectedContext=&ctx; expectedDepth=&ctx.view; worldCompare=D3D11_COMPARISON_LESS_EQUAL; opacity=.25f; inVisualDraw=true;
 draw(&ctx,[&]{
  ++calls; D3D11_DEPTH_STENCIL_DESC d{}; ctx.curDepth->GetDesc(&d);
  assert(d.DepthEnable && d.DepthWriteMask==D3D11_DEPTH_WRITE_MASK_ZERO && d.DepthFunc==D3D11_COMPARISON_LESS_EQUAL && !d.StencilEnable);
  D3D11_BLEND_DESC b{}; ctx.curBlend->GetDesc(&b); assert(b.RenderTarget[0].BlendEnable && b.RenderTarget[0].SrcBlend==D3D11_BLEND_BLEND_FACTOR);
  assert(std::fabs(ctx.factor[0]-.25f)<.00001f);
 });
 assert(calls==2 && ctx.curDepth==&ctx.depth && ctx.curBlend==&ctx.blend && ctx.stencil==7 && ctx.mask==0xfffffffe);
 assert(ctx.factor[0]==.2f && ctx.factor[3]==.5f && ctx.depth.refs==1 && ctx.blend.refs==1 && ctx.view.refs==1);
 int before=ctx.sets; ctx.dsv=&ctx.other; draw(&ctx,[&]{++calls;}); assert(calls==2 && ctx.sets==before && ctx.other.refs==1);
 ctx.dsv=&ctx.view; ctx.device.failBlend=true; draw(&ctx,[&]{++calls;}); assert(calls==2 && ctx.sets==before);
 assert(ctx.curDepth==&ctx.depth && ctx.curBlend==&ctx.blend && ctx.depth.refs==1 && ctx.blend.refs==1 && ctx.view.refs==1);
 ctx.device.failBlend=false; ctx.device.failDepth=true; draw(&ctx,[&]{++calls;}); assert(calls==2 && ctx.sets==before);
 ctx.device.failDepth=false; ctx.curDepth=nullptr; ctx.curBlend=nullptr;
 draw(&ctx,[&]{
  ++calls; D3D11_DEPTH_STENCIL_DESC d{}; ctx.curDepth->GetDesc(&d);
  assert(d.FrontFace.StencilFunc==D3D11_COMPARISON_ALWAYS && d.BackFace.StencilFunc==D3D11_COMPARISON_ALWAYS);
  D3D11_BLEND_DESC b{}; ctx.curBlend->GetDesc(&b);
  assert(b.RenderTarget[0].BlendEnable && b.RenderTarget[1].BlendOp==D3D11_BLEND_OP_ADD);
 });
 assert(calls==3 && !ctx.curDepth && !ctx.curBlend && ctx.view.refs==1);
 Context wrong; draw(&wrong,[&]{++calls;}); assert(calls==3 && wrong.sets==0);
 ctx.hasDevice=false; int sets=ctx.sets; draw(&ctx,[&]{++calls;}); assert(calls==3 && ctx.sets==sets);
 inVisualDraw=false; expectedContext=nullptr; expectedDepth=nullptr;
 // Check all seven expected D3D slots and reject ambiguous aliases before
 // creating any hooks. No fake COM method is invoked in this test.
 void* table[41]{};
 for (std::size_t i=0;i<7;++i) table[drawSlots[i]]=reinterpret_cast<void*>(static_cast<std::uintptr_t>(100+i));
 struct FakeObject { void** vtable; } fake{table};
 int created=0,enabled=0;
 auto create=[&](void*,void*,void** out) { ++created; *out=reinterpret_cast<void*>(123); return 0; };
 auto enable=[&](void*) {++enabled; return 0;};
 auto disable=[](void*) {return 0;}; auto remove=[](void*) {return 0;};
 auto* fakeCtx=reinterpret_cast<ID3D11DeviceContext*>(&fake);
 assert(install(fakeCtx,create,enable,disable,remove) && created==7 && enabled==7 && matches(fakeCtx));
 table[drawSlots[1]]=table[drawSlots[0]]; created=0; enabled=0;
 assert(!matches(fakeCtx) && !install(fakeCtx,create,enable,disable,remove) && created==0 && enabled==0);
}
