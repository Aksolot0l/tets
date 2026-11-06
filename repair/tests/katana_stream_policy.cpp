#include "../src/katana_stream_policy.h"
#include <cassert>
using katana_stream_policy::eligible;
int main() {
 constexpr std::uintptr_t b=0x10000000;
 assert(eligible(true,true,false,b+0x59fcbf,b,true));
 assert(eligible(true,false,true,b+0x56cd2f,b,true));
 assert(!eligible(false,true,true,b+0x59fcbf,b,true));
 assert(!eligible(true,false,true,b+0x59fcbf,b,true));
 assert(!eligible(true,true,false,b+0x56cd2f,b,true));
 assert(!eligible(true,true,true,b+0x56cd2f,b,false));
 assert(!eligible(true,true,true,b+0x59fd1e,b,true));
 assert(!eligible(true,true,true,1,b,true));
 assert(!eligible(true,true,true,b+0x59fcbf,0,true));
}
