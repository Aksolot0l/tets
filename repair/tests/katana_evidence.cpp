#include "../src/katana_evidence.h"
#include <cassert>
using namespace katana_evidence;
int main() {
 std::uint32_t bits=0;
 bits=addFirst(bits,false); assert(bits==firstMatch);
 bits|=firstStream|thirdIdentity; bits=addFirst(bits,false);
 assert((bits & (firstMatch|firstStream|thirdIdentity))==(firstMatch|firstStream|thirdIdentity));
 bits=addFirst(bits,true); assert(bits & firstNonzero);
 // Runtime wiring this revision relies on: cached/third calls, world draw,
 // rotation, occlusion-verified visibility and both suppression paths.
 bits|=thirdStream|thirdCached|cachedCall|worldDraw|rotationRead|rotationAgree|
       yawFlipped|occlusionVisible|suppressedFirst|suppressedThird|bridgeWindow|fingerprintLearned;
 assert(bits==0xFFFFu);
}
