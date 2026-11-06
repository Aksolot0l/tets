#include "../src/visual_gradient.h"
#include "../src/visual_ribbon.h"
#include <cassert>
#include <cmath>
#include <limits>
#include <vector>
int main() {
 using namespace visual_gradient;
 assert(phase(0,.5f)==0.f && phase(2000,.5f)==0.f);
 assert(std::fabs(phase(1000,.5f)-.5f)<1e-6);
 assert(phase(1000,std::numeric_limits<float>::quiet_NaN())==0);
 auto a=trail(0,11,0,255), b=trail(10,11,0,255), mid=trail(5,11,0,128);
 assert(a.r!=b.r || a.g!=b.g || a.b!=b.b); assert(mid.a==128);
 assert(trail(500,11,0,255).r==b.r);
 china_hat::Vertex x{{0,0,0},a},y{{1,0,0},b};
 std::vector<china_hat::Vertex> output;
 assert(visual_ribbon::segment(x,y,1.f,[&](auto v){output.push_back(v);}));
 assert(output[0].color.r==a.r && output[1].color.b==b.b && output[2].color.b==b.b);
}
