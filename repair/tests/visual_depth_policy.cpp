#include "../src/visual_depth_policy.h"
#include <cassert>
int main() {
 using namespace visual_depth_policy;
 assert(!overlayDraw(true) && overlayDraw(false));
 assert(worldDraw(true,true,true,true,true,true));
 assert(!worldDraw(false,true,true,true,true,true));
 assert(!worldDraw(true,false,true,true,true,true));
 assert(!worldDraw(true,true,false,true,true,true));
 assert(!worldDraw(true,true,true,false,true,true));
 assert(!worldDraw(true,true,true,true,false,true));
 assert(!worldDraw(true,true,true,true,true,false));
}
