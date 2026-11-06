#pragma once
#include <cstdint>

// Resolve only the player slot of a recently observed ClientInstance. A
// plausible unrelated actor or the camera target is not evidence of identity.
namespace hat_player {
constexpr std::uintptr_t playerSlot=0x60;
constexpr std::uint64_t maxTickAge=2000;
inline bool alignedPointer(std::uintptr_t value) {
 return value>=0x10000 && value<0x0000800000000000ULL && (value&7)==0;
}
template<class Read>
bool fromClient(std::uintptr_t client,std::uint64_t tick,std::uint64_t now,Read read,std::uintptr_t& player) {
 player=0;
 if (!alignedPointer(client) || !tick || now<tick || now-tick>maxTickAge) return false;
 std::uintptr_t first=0, again=0;
 if (!read(client+playerSlot,first) || !alignedPointer(first) ||
     !read(client+playerSlot,again) || again!=first) return false;
 player=first; return true;
}
}
