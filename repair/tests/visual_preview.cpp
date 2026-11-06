#include "../src/visual_preview.h"
#include <cassert>
#include <cmath>
int main() {
 using namespace visual_preview;
 auto a=hatRim(0,.6f,100), b=hatRim(.25f,.6f,100);
 assert(a.x>b.x && a.y<b.y);
 assert(hatRim(0,1.f,100).x>hatRim(0,.3f,100).x);
 assert(hatY(800,0,.1f,.3125f)<hatY(0,0,.1f,.3125f));
 assert(hatY(0,.2f,0,.3f)<hatY(0,0,0,.3f));
 assert(trailPoint(1,0,6).x<trailPoint(1,0,1).x);
 assert(trailPoint(.5f,0,3).y!=trailPoint(.5f,1000,3).y);
 for (unsigned i=0;i<100;i++) {
  auto p=trailPoint(i/99.f,1000,3); assert(std::isfinite(p.x) && std::isfinite(p.y));
 }
}
