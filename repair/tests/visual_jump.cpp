#include "../src/visual_jump.h"
#include <cassert>
#include <cmath>
#include <limits>
int main() {
 using namespace visual_jump;
 History h; china_hat::Point p{1,64,2};
 h.update(1,p,100,1000); h.update(1,p,220,1000);
 p.x=1.2f; p.y+=.04f; h.update(1,p,240,1000); assert(h.size()==0);
 p.y+=.06f; h.update(1,p,260,1000); assert(h.size()==1);
 assert(std::fabs(h[0].center.y-64.f)<.001f && h[0].center.x==1.f);
 p.y+=.4f; h.update(1,p,300,1000); assert(h.size()==1);
 p.y=64; h.update(1,p,400,1000); h.update(1,p,520,1000);
 p.y=64.12f; h.update(1,p,540,1000); assert(h.size()==2);
 h.expire(1260,1000); assert(h.size()==1); h.expire(1540,1000); assert(h.size()==0);
 h.update(2,p,1550,1000); assert(h.size()==0);
 p.y=std::numeric_limits<float>::quiet_NaN(); h.update(2,p,1600,1000); assert(h.size()==0);
 p={0,64,0}; h.update(1,p,100,1000); h.update(1,p,220,1000); p.x=30; p.y=70; h.update(1,p,240,1000); assert(h.size()==0);

 // Running and holding jump: no pause on the ground between takeoffs.
 History run; std::uint64_t time=1000; p={0,64,0}; run.update(1,p,time,5000);
 for (int jump=0;jump<4;++jump) {
  p.x+=.2f; p.y=64.04f; run.update(1,p,time+=16,5000);
  p.x+=.2f; p.y=64.12f; run.update(1,p,time+=16,5000);
  assert(run.size()==static_cast<std::size_t>(jump+1));
  p.x+=.4f; p.y=64.8f; run.update(1,p,time+=50,5000);
  p.x+=.4f; p.y=65.0f; run.update(1,p,time+=50,5000);
  p.x+=.4f; p.y=64.7f; run.update(1,p,time+=50,5000);
  p.x+=.4f; p.y=64.0f; run.update(1,p,time+=50,5000);
 }
 assert(std::fabs(run[1].center.y-64.f)<.001f);
 assert(run[1].center.x>run[0].center.x);
 // Level movement, sub-threshold jitter, and a continuous ascent must not
 // produce repeated rings. Rising begins immediately without a warm-up delay.
 History jitter; time=100; p={0,64,0}; jitter.update(1,p,time,5000);
 for(int i=0;i<40;++i) { p.x+=.1f; p.y=64.f+(i%2?.01f:0.f); jitter.update(1,p,time+=16,5000); }
 assert(jitter.size()==0);
 p.y=64.09f; jitter.update(1,p,time+=16,5000); assert(jitter.size()==1);
 for(int i=1;i<20;++i) { p.y+=.1f; jitter.update(1,p,time+=16,5000); } assert(jitter.size()==1);
 // Interpolated low-amplitude noise must not rearm after a jump.
 p.y-=.03f; jitter.update(1,p,time+=16,5000);
 p.y+=.025f; jitter.update(1,p,time+=16,5000); assert(jitter.size()==1);
 // No observation for half a second, or time going backwards, resets state.
 History stale; p={0,64,0}; stale.update(1,p,100,5000);
 p.y+=.12f; stale.update(1,p,120,5000); assert(stale.size()==1);
 stale.update(1,p,800,5000); assert(stale.size()==0);
 p.y+=.12f; stale.update(1,p,820,5000); assert(stale.size()==1);
 stale.update(1,p,10,5000); assert(stale.size()==0);
 // A teleport or actor replacement clears old rings instead of drawing across worlds.
 p.x+=30; jitter.update(1,p,time+=16,5000); assert(jitter.size()==0);
 p.y+=.1f; jitter.update(1,p,time+=16,5000); assert(jitter.size()==1);
 jitter.update(2,p,time+=16,5000); assert(jitter.size()==0);
 assert(progress(50,100,1000)==1.f && progress(100,100,0)==1.f);
 assert(std::fabs(radius(0,1.4f)-.2f)<.0001f && std::fabs(radius(1,1.4f)-1.4f)<.0001f);
 assert(alpha(0)==1.f && alpha(1)==0.f);
 int vertices=0; ring({0,0,0},1,.075f,[&](china_hat::Point v){ assert(std::isfinite(v.x) && v.y==0 && std::isfinite(v.z)); ++vertices; }); assert(vertices==48*12);
}
