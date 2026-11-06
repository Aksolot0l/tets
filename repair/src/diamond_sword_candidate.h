#pragma once
#include <cstdint>
#include <limits>

// Candidate identity check for the user's exact 1.1.5 executable. The caller
// supplies a checked read function and must validate EXE identity/context.
// The opt-in renderer is experimental and runtime identity is unverified.
namespace diamond_sword_candidate {
constexpr std::uintptr_t itemTableRva=0x190afa0;
constexpr std::int16_t legacyId=276;
template<class Read, class T> bool get(Read& read,std::uintptr_t base,std::uintptr_t offset,T& out) {
 if(!base || offset>std::numeric_limits<std::uintptr_t>::max()-base) return false;
 return read(base+offset,&out,sizeof(out));
}
// Requires the candidate stack's active flag, its item pointer, the candidate
// short ID and agreement with the same table used by diamond-sword registration.
// This is corroboration, not proof of the runtime ABI or drawing boundary.
template<class Read> bool matches(std::uintptr_t module,std::uintptr_t stack,Read read) {
 std::uint8_t active=0; std::uintptr_t item=0, registered=0; std::int16_t id=0;
 if (!get(read,stack,4,active) || !active || !get(read,stack,0x10,item) || !item ||
     !get(read,item,0x36,id) || id!=legacyId ||
     !get(read,module,itemTableRva+std::uintptr_t(legacyId)*sizeof(std::uintptr_t),registered)) return false;
 return registered!=0 && registered==item;
}
}
