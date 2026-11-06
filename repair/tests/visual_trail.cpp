#include "../src/visual_trail.h"
#include <cassert>
#include <limits>
using namespace visual_trail;
int main() {
 History h; h.update(100,{0,0,0},100); assert(h.size()==1);
 h.update(100,{.01f,0,0},110); assert(h.size()==1); // stationary
 h.update(100,{.1f,0,0},150); assert(h.size()==2);
 h.update(100,{.2f,0,0},160); assert(h.size()==2); // sample rate limit
 h.update(100,{.2f,0,0},250); assert(h.size()==3);
 h.update(100,{40,0,0},300); assert(h.size()==1); // teleport
 h.update(200,{40.1f,0,0},360); assert(h.size()==1); // changed player
 h.update(200,{40.2f,0,0},1500); assert(h.size()==1); // gap
 h.update(200,{std::numeric_limits<float>::quiet_NaN(),0,0},1600); assert(h.size()==0);
 h.update(200,{0,0,0},2000); h.update(200,{.1f,0,0},2100);
 h.expire(5001); assert(h.size()==1); h.expire(6000); assert(h.size()==0);
 h.clear(); h.update(3,{0,0,0},1000,6000); h.update(3,{.2f,0,0},1100,6000);
 h.expire(4100,6000); assert(h.size()==2); h.expire(4100,1000); assert(h.size()==0);
 h.clear(); for (int i=0;i<180;i++) h.update(1,{float(i)*.1f,0,0},1000+i*51);
 assert(h.size()<=History::capacity && h.size()>2);
 h.clear(); assert(!h.size());
}
