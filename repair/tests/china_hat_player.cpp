#include "../src/china_hat_player.h"
#include <cassert>
#include <cstdint>
int main() {
 using namespace hat_player;
 std::uintptr_t local=0;
 auto read=[](std::uintptr_t at,std::uintptr_t& out) { if (at!=0x20060) return false; out=0x30000; return true; };
 assert(fromClient(0x20000,1000,1500,read,local) && local==0x30000);
 assert(!fromClient(0x20000,1000,3001,read,local) && !local);
 assert(!fromClient(0x20000,0,1000,read,local));
 assert(!fromClient(0x20000,1000,999,read,local));
 assert(!fromClient(0x20001,1000,1500,read,local));
 assert(!fromClient(0x20000,1000,1500,[](auto,auto& v){v=0;return true;},local));
 assert(!fromClient(0x20000,1000,1500,[](auto,auto& v){v=0x30001;return true;},local));
 assert(!fromClient(0x20000,1000,1500,[](auto,auto&){return false;},local));
 int count=0;
 auto moving=[&](auto,auto& v) { v=(++count==1)?0x30000:0x40000; return true; };
 assert(!fromClient(0x20000,1000,1500,moving,local) && !local);
}
