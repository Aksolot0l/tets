#include "../src/diamond_sword_candidate.h"
#include <cassert>
#include <cstring>
#include <limits>
#include <map>
#include <vector>
using namespace diamond_sword_candidate;
int main() {
 std::map<std::uintptr_t,std::vector<unsigned char>> memory;
 auto put=[&](std::uintptr_t at,const auto& value) { auto p=reinterpret_cast<const unsigned char*>(&value); memory[at]=std::vector<unsigned char>(p,p+sizeof(value)); };
 auto read=[&](std::uintptr_t at,void* out,std::size_t size) { auto it=memory.find(at); if(it==memory.end()||it->second.size()!=size)return false; std::memcpy(out,it->second.data(),size); return true; };
 const std::uintptr_t base=0x10000000,stack=0x20000000,item=0x30000000;
 put(stack+4,std::uint8_t(1)); put(stack+0x10,item); put(item+0x36,legacyId);
 auto table=base+itemTableRva+std::uintptr_t(legacyId)*sizeof(std::uintptr_t); put(table,item);
 assert(matches(base,stack,read));
 put(item+0x36,std::int16_t(267)); assert(!matches(base,stack,read)); put(item+0x36,legacyId);
 put(table,std::uintptr_t(0x40000000)); assert(!matches(base,stack,read)); put(table,item);
 put(stack+4,std::uint8_t(0)); assert(!matches(base,stack,read)); put(stack+4,std::uint8_t(1));
 put(stack+0x10,std::uintptr_t(0)); assert(!matches(base,stack,read)); put(stack+0x10,item);
 memory.erase(table); assert(!matches(base,stack,read));
 assert(!matches(0,stack,read)); assert(!matches(std::numeric_limits<std::uintptr_t>::max()-1,stack,read));
 assert(!matches(base,std::numeric_limits<std::uintptr_t>::max()-1,read));
}
