#pragma once
#include <cstdint>
namespace katana_evidence {
// Session aggregate, not a per-frame rendering result or proof of visibility.
// Occlusion-verified bits are the only ones that imply visible pixels.
enum : std::uint32_t {
 firstMatch=1u, firstNonzero=2u, firstStream=4u, thirdStream=8u,
 thirdIdentity=16u, cachedCall=32u, thirdCached=64u, worldDraw=128u,
 rotationRead=256u, rotationAgree=512u, yawFlipped=1024u,
 occlusionVisible=2048u, suppressedFirst=4096u, suppressedThird=8192u,
 bridgeWindow=16384u, fingerprintLearned=32768u
};
inline std::uint32_t addFirst(std::uint32_t current,bool nonzero) {
 return current | firstMatch | (nonzero?firstNonzero:0u);
}
}
