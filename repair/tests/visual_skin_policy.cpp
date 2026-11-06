#include "../src/visual_skin_policy.h"
#include <cassert>
int main() {
 using namespace visual_skin_policy;
 assert(dimensions(64,64,1,1) && dimensions(64,32,1,1));
 assert(dimensions(128,128,1,1) && dimensions(128,64,1,1));
 assert(dimensions(512,512,1,1) && !dimensions(1024,1024,1,1));
 assert(!dimensions(32,32,1,1) && !dimensions(65,65,1,1));
 assert(!dimensions(64,0x80000020u,1,1));
 assert(!dimensions(64,48,1,1) && !dimensions(64,64,4,1) && !dimensions(64,64,1,6));
 assert(vScale(64,64)==1.f && vScale(64,32)==2.f && vScale(0,0)==1.f);
}
