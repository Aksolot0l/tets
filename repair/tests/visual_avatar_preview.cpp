#include "../src/visual_avatar_preview.h"
#include <cassert>
#include <cmath>
#include <limits>
int main() {
 using namespace visual_avatar_preview;
 HatSettings s{.58f,.34f,0.f,.07f,.3125f,.15f,false};
 china_hat::Mesh m{}; assert(hat(m,s,0));
 assert(m.triangleVertices==144 && m.outlineVertices==49);
 float base=.64f+.03f+hat_animation::bob(0,s.bobAmount,s.bobSpeed);
 assert(std::fabs(m.outline[0].position.y-base)<1e-6f);
 assert(std::fabs(m.triangles[0].position.y-(base+s.height))<1e-6f);
 assert(std::fabs(m.outline[0].position.x-s.radius)<1e-6f);
 auto y=m.outline[0].position.y; assert(hat(m,s,800)); assert(m.outline[0].position.y>y);
 float defaultScale=hatScale(s);
 // The preview camera and character stay fixed while the hat bobs.
 for (std::uint64_t t=0;t<10000;t+=47) {
  assert(hat(m,s,t));
  assert(hatScale(s)==defaultScale);
  assert(project({0.f,.64f,0.f},hatScale(s)).y==project({0.f,.64f,0.f},defaultScale).y);
  assert(project(m.triangles[0].position,defaultScale).y>=14.f);
 }
 assert(hat(m,s,0));
 s.offset=.2f; s.radius=.9f; s.height=.6f; assert(hat(m,s,0));
 assert(hatScale(s)<defaultScale && hatScale(s)>0);
 assert(std::fabs(m.outline[0].position.y-(base+.2f))<1e-6f);
 assert(std::fabs(m.outline[0].position.x-.9f)<1e-6f);
 assert(std::fabs(m.triangles[0].position.y-(base+.2f+.6f))<1e-6f);
 s.radius=std::numeric_limits<float>::quiet_NaN(); assert(!hat(m,s,0) && m.triangleVertices==0);
 auto front=project({0,0,.5f}), back=project({0,0,-.5f}); assert(front.y>back.y);
 assert(trailPoint(1,0,6).x<trailPoint(1,0,1).x);
 assert(trailPoint(0,0,3).x==trailPoint(0,2000,3).x);
 assert(trailPoint(.5f,0,3).y!=trailPoint(.5f,1000,3).y);
 assert(trailHeight(.8f)>trailHeight(.3f));
}
