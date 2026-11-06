#include "../src/visual_depth_policy.h"
#include <cassert>
#include <initializer_list>
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
 assert(playerHat(true,true,true,true,true,false));
 assert(!playerHat(false,true,true,true,true,false));
 assert(!playerHat(true,false,true,true,true,false));
 assert(!playerHat(true,true,false,true,true,false));
 assert(!playerHat(true,true,true,false,true,false));
 assert(!playerHat(true,true,true,true,false,false));
 assert(!playerHat(true,true,true,true,true,true));
 assert(entitiesHat(false) && !entitiesHat(true));
 // Exactly one selected hat path, and trial failure never silently falls
 // back to depth-disabled screen rendering. Trail remains a separate path.
 for (bool strict:{false,true}) for (bool selected:{false,true}) {
  int player=playerHat(strict,selected,true,true,true,false) ? 1 : 0;
  int world=worldDraw(strict,true,true,true,true,true) && entitiesHat(selected) ? 1 : 0;
  int overlay=overlayDraw(strict) ? 1 : 0;
  assert(player+world+overlay==1);
 }
 assert(!playerHat(true,true,true,true,false,false) && !entitiesHat(true) && !overlayDraw(true));
}
