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
}
