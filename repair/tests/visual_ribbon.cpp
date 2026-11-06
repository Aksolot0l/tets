#include "../src/visual_ribbon.h"
#include "../src/hat_animation.h"
#include <cassert>
#include <cmath>
#include <limits>
#include <vector>
int main() {
 using namespace china_hat;
 Vertex a{{2,3,4},{1,2,3,4}}, b{{5,3.1f,6},{5,6,7,8}};
 std::vector<Vertex> out;
 assert(visual_ribbon::segment(a,b,1.4f,[&](Vertex v){out.push_back(v);}));
 assert(out.size()==12);
 assert(out[0].position.y==a.position.y && out[1].position.y==b.position.y);
 assert(std::fabs(out[2].position.y-(b.position.y+1.4f))<1e-5f);
 assert(std::fabs(out[5].position.y-(a.position.y+1.4f))<1e-5f);
 assert(out[6].position.x==out[2].position.x && out[8].position.x==out[0].position.x);
 out.clear(); assert(!visual_ribbon::segment(a,b,0,[&](Vertex v){out.push_back(v);}) && out.empty());
 assert(!visual_ribbon::segment(a,b,2.1f,[&](Vertex v){out.push_back(v);}));
 b.position.x=std::numeric_limits<float>::quiet_NaN();
 assert(!visual_ribbon::segment(a,b,1.f,[&](Vertex v){out.push_back(v);}));
 auto base=hat_animation::bob(0), high=hat_animation::bob(800), low=hat_animation::bob(2400);
 assert(base>.06f && base<.08f && high>base && low<base && low>=0);
 assert(std::fabs(hat_animation::bob(3200)-base)<1e-6);
 for (std::uint64_t t=0;t<3200;t+=17) { float v=hat_animation::bob(t); assert(v>=0 && v<=.141f); }
}
