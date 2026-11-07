#include "../src/visual_color.h"
#include <cassert>
#include <cmath>
int main() {
 using namespace visual_color;
 for (auto effect:{Effect::Hat,Effect::Trail}) {
  auto original=material(false,effect);
  for (float x:original) assert(x==1.f);
  auto color=material(true,effect);
  for (float x:color) assert(std::isfinite(x) && x>=0.f && x<=1.f);
  assert(color[3]==1.f && (color[0]<.9f || color[1]<.9f || color[2]<.9f));
 }
 assert(material(true,Effect::Hat)!=material(true,Effect::Trail));
 std::array<float,3> custom{{.4f,.5f,.6f}};
 auto white=animated(false,true,custom,.2f,1000); assert(white[0]==1.f && white[1]==1.f && white[2]==1.f);
 auto fixed=animated(true,false,custom,.2f,1000); assert(fixed[0]==.4f && fixed[1]==.5f && fixed[2]==.6f);
 auto first=animated(true,true,custom,.5f,0), second=animated(true,true,custom,.5f,1000);
 assert(first!=second && first==animated(true,true,custom,.5f,2000));
 for (std::uint64_t t=0;t<10000;t+=83) for (float x:animated(true,true,custom,.15f,t)) assert(std::isfinite(x) && x>=0.f && x<=1.f);
 // v11: the two-color gradient. Phase 0 is pure A, 0.5 is pure B, 1 wraps
 // back to A; every phase stays inside the A..B box and the cycle repeats.
 std::array<float,3> a{{.1f,.2f,.3f}}, b{{.9f,.8f,.7f}};
 auto at0=gradient(a,b,0.f);  assert(at0[0]==a[0] && at0[1]==a[1] && at0[2]==a[2] && at0[3]==1.f);
 auto half=gradient(a,b,.5f); assert(half[0]==b[0] && half[1]==b[1] && half[2]==b[2]);
 auto wrap=gradient(a,b,1.f); assert(wrap[0]==a[0] && wrap[1]==a[1] && wrap[2]==a[2]);
 auto quarter=gradient(a,b,.25f);
 for (int i=0;i<3;++i) assert(quarter[i]>=a[i]-1e-6f && quarter[i]<=b[i]+1e-6f);
 assert(quarter[0]>a[0] && quarter[0]<b[0]);
 for (int i=0;i<=40;++i) {
  auto c=gradient(a,b,i*.137f);
  for (float x:c) assert(std::isfinite(x) && x>=0.f && x<=1.f);
 }
 assert(gradient(a,b,.0f)==gradient(a,b,2.f));  // periodic
}
