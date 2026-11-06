#pragma once
#include <cstdint>
namespace katana_evidence {
// Session aggregate, not a per-frame rendering result or proof of visibility.
enum : std::uint32_t {
 firstMatch=1u, firstNonzero=2u, firstStream=4u, thirdStream=8u,
 prepared=16u, blocked=32u, cachedCall=64u, submitted=128u
};
inline std::uint32_t addFirst(std::uint32_t current,bool nonzero) {
 return current | firstMatch | (nonzero?firstNonzero:0u);
}
}
