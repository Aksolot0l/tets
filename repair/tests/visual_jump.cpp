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
 assert(progress(50,100,1000)==1.f && progress(100,100,0)==1.f);
 assert(std::fabs(radius(0,1.4f)-.2f)<.0001f && std::fabs(radius(1,1.4f)-1.4f)<.0001f);
 assert(alpha(0)==1.f && alpha(1)==0.f);
 int vertices=0; ring({0,0,0},1,.075f,[&](china_hat::Point v){ assert(std::isfinite(v.x) && v.y==0 && std::isfinite(v.z)); ++vertices; }); assert(vertices==48*12);
}
