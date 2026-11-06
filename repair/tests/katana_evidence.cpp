#include "../src/katana_evidence.h"
#include <cassert>
using namespace katana_evidence;
int main() {
 std::uint32_t bits=0;
 bits=addFirst(bits,false); assert(bits==firstMatch);
 bits|=firstStream|prepared; bits=addFirst(bits,false);
 assert((bits & (firstMatch|firstStream|prepared))==(firstMatch|firstStream|prepared));
 bits=addFirst(bits,true); assert(bits & firstNonzero);
 bits|=thirdStream|blocked|cachedCall|submitted;
 assert(bits==255);
}
