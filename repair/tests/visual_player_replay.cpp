#define VISUAL_PLAYER_REPLAY_TEST_TYPES "../tests/visual_replay_mock_types.h"
#include "../src/visual_player_replay.h"
#include <cassert>
#include <cstring>
#include <vector>
static int liveObjects=0;
template<class T> struct Obj:T { int refs=1; Obj(){++liveObjects;} ~Obj(){--liveObjects;} ULONG AddRef() override {return ++refs;} ULONG Release() override {auto r=--refs;if(!r)delete this;return r;} };
struct Device:Obj<ID3D11Device> {
 bool fail=false;
 HRESULT CreateTexture2D(const D3D11_TEXTURE2D_DESC*,void*,ID3D11Texture2D** out) override {if(fail)return -1;*out=new Obj<ID3D11Texture2D>;return 0;}
 HRESULT CreateRenderTargetView(ID3D11Texture2D*,void*,ID3D11RenderTargetView** out) override {*out=new Obj<ID3D11RenderTargetView>;return 0;}
 HRESULT CreateShaderResourceView(ID3D11Texture2D*,void*,ID3D11ShaderResourceView** out) override {*out=new Obj<ID3D11ShaderResourceView>;return 0;}
 HRESULT CreateDepthStencilView(ID3D11Texture2D*,void*,ID3D11DepthStencilView** out) override {*out=new Obj<ID3D11DepthStencilView>;return 0;}
 D3D_FEATURE_LEVEL GetFeatureLevel() override {return D3D_FEATURE_LEVEL_11_0;}
};
struct Depth:Obj<ID3D11DepthStencilState> {D3D11_DEPTH_STENCIL_DESC desc{TRUE,D3D11_COMPARISON_LESS,FALSE}; void GetDesc(D3D11_DEPTH_STENCIL_DESC* d) override {*d=desc;} };
struct Context:ID3D11DeviceContext {
 Device* device=new Device; Depth* depth=new Depth;
 ID3D11RenderTargetView* target=new Obj<ID3D11RenderTargetView>;
 ID3D11DepthStencilView* dsv=new Obj<ID3D11DepthStencilView>;
 ID3D11RenderTargetView* curTarget=target; ID3D11DepthStencilView* curDsv=dsv;
 ID3D11ShaderResourceView* bound=nullptr; ID3D11UnorderedAccessView* uav=nullptr; ID3D11Buffer* so=nullptr; ID3D11Predicate* pred=nullptr;
 D3D11_VIEWPORT viewport{0,0,1920,1080,0,1}; D3D11_RECT scissors{1,2,3,4}; UINT views=1,scissorCount=1; int sets=0,clears=0,depthClears=0;
 ~Context(){device->Release();depth->Release();target->Release();dsv->Release();if(uav)uav->Release();if(so)so->Release();if(pred)pred->Release();}
 void shader(UINT,UINT n,ID3D11ShaderResourceView** out) {for(UINT i=0;i<n;i++)out[i]=nullptr;if(bound){out[0]=bound;bound->AddRef();}}
 void VSGetShaderResources(UINT a,UINT b,ID3D11ShaderResourceView** c) override {shader(a,b,c);}
 void HSGetShaderResources(UINT a,UINT b,ID3D11ShaderResourceView** c) override {shader(a,b,c);}
 void DSGetShaderResources(UINT a,UINT b,ID3D11ShaderResourceView** c) override {shader(a,b,c);}
 void GSGetShaderResources(UINT a,UINT b,ID3D11ShaderResourceView** c) override {shader(a,b,c);}
 void PSGetShaderResources(UINT a,UINT b,ID3D11ShaderResourceView** c) override {shader(a,b,c);}
 void CSGetShaderResources(UINT a,UINT b,ID3D11ShaderResourceView** c) override {shader(a,b,c);}
 void GetDevice(ID3D11Device** o) override {*o=device;device->AddRef();}
 void ClearRenderTargetView(ID3D11RenderTargetView*,const FLOAT[4]) override {clears++;}
 void RSGetViewports(UINT* n,D3D11_VIEWPORT* o) override {if(o && *n>0)*o=viewport;*n=views;}
 void OMGetDepthStencilState(ID3D11DepthStencilState** o,UINT* s) override {*o=depth;depth->AddRef();*s=0;}
 void OMGetRenderTargets(UINT n,ID3D11RenderTargetView** o,ID3D11DepthStencilView** v) override {for(UINT i=0;i<n;i++)o[i]=nullptr;o[0]=curTarget;if(curTarget)curTarget->AddRef();*v=curDsv;if(curDsv)curDsv->AddRef();}
 void SOGetTargets(UINT n,ID3D11Buffer** o) override {for(UINT i=0;i<n;i++)o[i]=nullptr;if(so){o[0]=so;so->AddRef();}}
 void GetPredication(ID3D11Predicate** o,BOOL* b) override {*o=pred;*b=FALSE;if(pred)pred->AddRef();}
 void OMGetRenderTargetsAndUnorderedAccessViews(UINT,ID3D11RenderTargetView**,ID3D11DepthStencilView**,UINT,UINT n,ID3D11UnorderedAccessView** o) override {for(UINT i=0;i<n;i++)o[i]=nullptr;if(uav){o[0]=uav;uav->AddRef();}}
 void ClearDepthStencilView(ID3D11DepthStencilView*,UINT,float,UINT) override {depthClears++;}
 void RSGetScissorRects(UINT* n,D3D11_RECT* o) override {if(*n>0 && scissorCount)*o=scissors;*n=scissorCount;}
 void OMSetRenderTargets(UINT n,ID3D11RenderTargetView* const* o,ID3D11DepthStencilView* v) override {assert(n==1 || n==8);assert(o[0] && v);curTarget=o[0];curDsv=v;sets++;}
 void RSSetViewports(UINT n,const D3D11_VIEWPORT* v) override {assert(n==1);viewport=*v;sets++;}
 void RSSetScissorRects(UINT n,const D3D11_RECT* r) override {assert(n==1 || n==0);scissorCount=n;if(n)scissors=*r;sets++;}
};
int main() {
 using namespace visual_player_replay;
 {
  Context c;
  begin(1,&c); assert(!scope.ctx && c.clears==0);
  setRequested(true); begin(1,&c); assert(scope.ctx==&c && c.clears==1 && !acquire(c.device));
  const auto oldViewport=c.viewport; const auto oldScissor=c.scissors;
  assert(beginDraw(&c) && saved.active && c.sets==3 && c.depthClears==1);
  assert(c.curTarget==scope.rtv && c.curDsv==scope.dsv && c.viewport.Height==768 && c.scissors.right==256);
  endDraw(&c); assert(!saved.active && c.sets==6 && scope.draws==1);
  assert(c.curTarget==c.target && c.curDsv==c.dsv && std::memcmp(&oldViewport,&c.viewport,sizeof(oldViewport))==0 && std::memcmp(&oldScissor,&c.scissors,sizeof(oldScissor))==0);
  end(); auto* image=acquire(c.device); assert(image); image->Release();
  mockTick+=2001; assert(!acquire(c.device)); mockTick=100;
  // A view bound for reading prevents any clear or target mutation.
  c.bound=targets[1].srv; const int clears=c.clears; begin(1,&c); assert(!scope.ctx && c.clears==clears); c.bound=nullptr;
  begin(1,&c); assert(scope.index==1); c.depth->desc.StencilEnable=TRUE;
  assert(!beginDraw(&c) && c.sets==6); c.depth->desc.StencilEnable=FALSE;
  c.uav=new Obj<ID3D11UnorderedAccessView>; assert(!beginDraw(&c)); c.uav->Release(); c.uav=nullptr;
  c.so=new Obj<ID3D11Buffer>; assert(!beginDraw(&c)); c.so->Release(); c.so=nullptr;
  c.pred=new Obj<ID3D11Predicate>; assert(!beginDraw(&c)); c.pred->Release(); c.pred=nullptr;
  c.views=2; assert(!beginDraw(&c)); c.views=1;
  assert(beginDraw(&c)); endDraw(&c);
  c.depth->desc.DepthFunc=D3D11_COMPARISON_GREATER; assert(!beginDraw(&c)); c.depth->desc.DepthFunc=D3D11_COMPARISON_LESS; end();
  auto* held=acquire(c.device); assert(held);
  // Closing while a frame holds a view leaves that view alive, but removes it
  // from future snapshots; reopening cannot publish an old scoped capture.
  begin(1,&c); assert(beginDraw(&c)); endDraw(&c); setRequested(false); setRequested(true); end();
  assert(!acquire(c.device)); held->Release();
  // Device creation failure is fail-closed and isn't retried each frame.
  setRequested(false); c.device->fail=true; setRequested(true); begin(1,&c); assert(!scope.ctx && creationFailed);
  c.device->fail=false; begin(1,&c); assert(!scope.ctx && creationFailed);
  setRequested(false); setRequested(true); begin(2,&c); assert(scope.ctx); end(); setRequested(false);
  assert(liveObjects==4); // context-owned device, depth, world render target and DSV
 }
 assert(liveObjects==0);
}
