#define VISUAL_PLAYER_SKIN_TEST_TYPES "../tests/visual_skin_mock_types.h"
#include "../src/visual_player_skin.h"
#include <cassert>
struct Device:ID3D11Device { int refs=1; ULONG AddRef() override{return ++refs;} ULONG Release() override{return --refs;} };
struct Texture:ID3D11Texture2D {
 int refs=1; D3D11_TEXTURE2D_DESC d{};
 Texture(unsigned w=64,unsigned h=64) {d.Width=w;d.Height=h;d.ArraySize=1;d.SampleDesc.Count=1;}
 ULONG AddRef() override{return ++refs;} ULONG Release() override{return --refs;}
 HRESULT QueryInterface(int,void** out) override {AddRef();*out=this;return 0;}
 void GetDesc(D3D11_TEXTURE2D_DESC* out) override{*out=d;}
};
struct View:ID3D11ShaderResourceView {
 int refs=1; Texture* texture; D3D11_SHADER_RESOURCE_VIEW_DESC d{};
 explicit View(Texture* t):texture(t) {d.Format=DXGI_FORMAT_R8G8B8A8_UNORM;d.ViewDimension=D3D11_SRV_DIMENSION_TEXTURE2D;}
 ULONG AddRef() override{return ++refs;} ULONG Release() override{return --refs;}
 void GetDesc(D3D11_SHADER_RESOURCE_VIEW_DESC* out) override{*out=d;}
 void GetResource(ID3D11Resource** out) override{texture->AddRef();*out=texture;}
};
struct Context:ID3D11DeviceContext {
 Device* device; View* views[4]{};
 explicit Context(Device* d):device(d){}
 void GetDevice(ID3D11Device** out) override {device->AddRef();*out=device;}
 void PSGetShaderResources(UINT start,UINT count,ID3D11ShaderResourceView** out) override {
  assert(start==0 && count==4); for(unsigned i=0;i<count;i++){out[i]=views[i];if(out[i])out[i]->AddRef();}
 }
};
int main() {
 using namespace visual_player_skin;
 Device d,other; Context ctx(&d),wrong(&other);
 Texture t1,t2(64,32),invalid(256,128),atlas(1024,1024); invalid.d.BindFlags=D3D11_BIND_RENDER_TARGET;
 View v1(&t1),v2(&t2),bad(&invalid),big(&atlas); ctx.views[0]=&v1;ctx.views[1]=&v2;ctx.views[2]=&bad;ctx.views[3]=&big;
 observe(&ctx); assert(count==0);
 setRequested(true); begin(42,&ctx); assert(scopeContext==&ctx && d.refs==2);
 observe(&wrong); assert(count==0); observe(&ctx); assert(count==2);
 assert(v1.refs==2 && t1.refs==2 && v2.refs==2 && t2.refs==2 && bad.refs==1 && invalid.refs==1 && big.refs==1);
 observe(&ctx); assert(count==2 && v1.refs==2 && t1.refs==2);
 auto snap=acquire(&d); assert(snap.view==&v1 && snap.width==64 && snap.height==64 && snap.count==2 && v1.refs==3);
 assert(!acquire(&other).view); next(); auto second=acquire(&d); assert(second.view==&v2 && second.height==32 && second.index==1);
 second.view->Release(); end();
 // Changing player clears the old views while the frame-held view stays valid.
 begin(43,&ctx); assert(count==0 && v1.refs==2 && t1.refs==1 && v2.refs==1 && t2.refs==1);
 snap.view->Release(); assert(v1.refs==1); observe(&ctx); assert(count==2); end();
 // Device changes invalidate prior views; they may never be shown to another device.
 begin(43,&wrong); assert(count==0 && d.refs==1 && other.refs==2 && !acquire(&d).view); end();
 setRequested(false); assert(other.refs==1 && !isRequested() && count==0);
 // A capture started before a close/reopen cannot reinsert its old texture.
 setRequested(true); begin(44,&ctx); setRequested(false); setRequested(true);
 observe(&ctx); assert(count==0); end(); begin(44,&ctx); observe(&ctx); assert(count==2); end(); setRequested(false);
 assert(v1.refs==1 && v2.refs==1 && t1.refs==1 && t2.refs==1);
}
