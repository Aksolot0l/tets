#include "../src/visual_capture_policy.h"
#include <cassert>
#include <cstdint>
int main() {
 using namespace visual_capture_policy;
 assert(supported(1920,1080,1,1)); assert(supported(3840,2160,1,1));
 assert(!supported(0,1080,1,1)); assert(!supported(1920,0,1,1));
 assert(!supported(1920,1080,2,1)); assert(!supported(1920,1080,1,2));
 assert(!supported(UINT32_MAX,UINT32_MAX,1,1)); assert(!supported(8000,8000,1,1));
}
