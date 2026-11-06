#pragma once
#include <cstdint>
namespace katana_stream_policy {
// 1.1.5 observed call sites only. A local-player render scope is mandatory for
// third person; the first-person outer scope already verifies actor and item.
inline bool eligible(bool enabled,bool firstScope,bool localPlayerPass,std::uintptr_t caller,std::uintptr_t base,bool matchedSword) {
 if (!enabled || !matchedSword || !base || caller<base) return false;
 auto rva=caller-base;
 return (rva==0x59fcbf && firstScope) || (rva==0x56cd2f && localPlayerPass);
}
}
