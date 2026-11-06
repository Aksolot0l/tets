#include "../research/item_render/cached_material.h"
#include <cassert>
using namespace cached_material_research;
int main() {
 for (int e8=0;e8<2;++e8) for (int e9=0;e9<2;++e9)
 for (int ea=0;ea<2;++ea) for (int mode=0;mode<2;++mode) {
  const auto p=plan(e8,e9,ea,mode);
  if(mode) {
   assert(p.call==Call::dualTextured && p.texture==0x90 && p.auxiliary==0x2a0);
   assert(p.material==(e8?0x240u:e9?0x210u:0x1e0u));
  } else if(e8) {
   assert(p.call==Call::mesh && p.material==0x180 && !p.texture && !p.auxiliary);
  } else {
   assert(p.call==Call::textured && p.texture==0x90 && !p.auxiliary);
   assert(p.material==(ea?0x1b0u:e9?0x150u:0x120u));
  }
 }
}
