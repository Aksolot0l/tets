#include "../src/visual_model_capture_policy.h"
#include <cassert>
#include <cmath>
#include <limits>
int main() {
 using namespace visual_model_capture_policy;
 Viewport in{0,0,1920,1080,0,1},out{};
 assert(viewport(in,256,out)); assert(out.width==1365.333374f && out.height==768.f);
 assert(std::fabs(out.x+out.width*.5f-128)<.001f && out.y+out.height*.5f==128);
 assert(safeDraw(true,true,false,false,false,false,1));
 assert(!safeDraw(false,true,false,false,false,false,1));
 assert(!safeDraw(true,false,false,false,false,false,1));
 assert(!safeDraw(true,true,true,false,false,false,1));
 assert(!safeDraw(true,true,false,true,false,false,1));
 assert(!safeDraw(true,true,false,false,true,false,1));
 assert(!safeDraw(true,true,false,false,false,true,1));
 assert(!safeDraw(true,true,false,false,false,false,2));
 in.width=std::numeric_limits<float>::quiet_NaN(); assert(!viewport(in,256,out));
 in.width=0; assert(!viewport(in,256,out));
}
