#pragma once
#include <cmath>
#include <cstdint>
#include <cstddef>

// Read-only rotation sources for the exact 1.1.5 executable (EXE SHA-256
// 2883b72a3579d95bfec73330c5e06f2463253b7ca530dd59f50885a71a724732).
//
// Angle semantics, cross-checked against the published cheat table for this
// exact build: the yaw-like float ("XRotation") and the pitch-like float
// ("YRotation", positive = looking up) are radians, and the horizontal look
// direction is (-sin(yaw), -cos(yaw)) -- the table's JetPack script drives
// movement with exactly those terms, and its velocity fields (+0xAC/+0xB0/
// +0xB4) sit on the same player object this adapter already proves behind the
// tick-observed ClientInstance's +0x60 slot.
//
// The table reaches the angles through [globalA]+0x18 / [globalB]+0x10 ->
// +0x58 -> +0xB8/+0xBC. Because the velocity block ends exactly at +0xB8, the
// same angle offsets are also tried on the player object itself, which does
// not depend on the published globals at all. The +0x58 hop is additionally
// tried one step from the tick-observed ClientInstance.
//
// The table is silent about yaw wrapping (its math is periodic), so a yaw of
// any finite magnitude is accepted and wrapped into [-pi,pi]; the pitch stays
// strictly bounded because it cannot wrap physically. Every read is validated
// (aligned pointers, finite floats, re-read against torn updates) and any
// failure simply leaves the reading invalid, so the caller can keep the
// original item untouched.
//
// These are observed offsets for one build, not a stable API.
namespace katana_rotation {
constexpr float pi=3.14159265f;
constexpr float maxYaw=3.35f;               // radians; wrapped yaw stays within +-pi
constexpr float maxPitch=1.75f;             // pitch is bounded to +-pi/2
constexpr float maxUnwrappedYaw=100000.f;   // finite bound before wrapping
constexpr std::uintptr_t chainGlobalA=0x1921DF8;  // root -> +0x18 -> +0x58 -> angles
constexpr std::uintptr_t chainGlobalB=0x1921888;  // root -> +0x10 -> +0x58 -> angles
constexpr std::uintptr_t rootHopA=0x18;
constexpr std::uintptr_t rootHopB=0x10;
constexpr std::uintptr_t rotationSlot=0x58;       // intermediate -> angles holder
constexpr std::uintptr_t playerSlot=0x60;         // intermediate -> player (adapter-proven)
constexpr std::uintptr_t yawOffset=0xB8;
constexpr std::uintptr_t pitchOffset=0xBC;
constexpr std::uintptr_t headOffset=0x160;        // "Head+" entry of the same table

struct Reading {
 float yaw=0.f, pitchUp=0.f;
 unsigned sources=0;  // bit 1 player, bit 2 global A, bit 4 global B, bit 8 client +0x58
 bool agree=false;    // two independent sources within 0.02 rad
 bool wrapped=false;  // yaw magnitude exceeded +-pi and was wrapped
};

// Raw pointer/float trail for the diagnostics panel. Nothing here is validated
// or used for rendering; it exists so one screenshot explains a dead chain.
struct Diagnostics {
 std::uintptr_t clientInstance=0;
 std::uintptr_t player=0, rotationHolder=0;      // [CI+0x60], [CI+0x58]
 std::uintptr_t rootA=0, interA=0, holderA=0;    // [base+A], [rootA+0x18], [interA+0x58]
 std::uintptr_t rootB=0, interB=0, holderB=0;    // [base+B], [rootB+0x10], [interB+0x58]
 float playerAngles[2]{}, holderAngles[2]{}, holderAAngles[2]{}, holderBAngles[2]{};
 float playerHead=0;
 bool playerAnglesRead=false, holderAnglesRead=false, holderAAnglesRead=false,
      holderBAnglesRead=false, playerHeadRead=false;
};

inline bool validPointer(std::uint64_t value) {
 return value>=0x10000 && value<0x0000800000000000ULL && (value&7)==0;
}
inline bool validAngle(float v, float bound) {
 return std::isfinite(v) && std::fabs(v)<=bound;
}

// Wrap any finite radian yaw into [-pi,pi]. The published movement math is
// periodic in yaw, so wrapping preserves the look direction for both wrapped
// and unwrapped storage conventions.
inline float wrapYaw(float v) {
 if (!std::isfinite(v)) return v;
 const float turns=std::nearbyint(v/(2.f*pi));
 return v-turns*2.f*pi;
}

// Read helper signature: bool(std::uintptr_t address, void* out, std::size_t n)
template<class Read>
bool readAnglesAt(std::uintptr_t object, Read read, float& yaw, float& pitchUp, bool* wrappedOut=nullptr) {
 if (!validPointer(object)) return false;
 float values[2]{};
 if (!read(object+yawOffset,&values,sizeof(values))) return false;
 if (!std::isfinite(values[0]) || !std::isfinite(values[1])) return false;
 if (std::fabs(values[0])>maxUnwrappedYaw) return false;
 if (!validAngle(values[1],maxPitch)) return false;
 // Re-read to reject torn mid-update values.
 float again[2]{};
 if (!read(object+yawOffset,&again,sizeof(again))) return false;
 if (again[0]!=values[0] || again[1]!=values[1]) return false;
 const bool wrapped=std::fabs(values[0])>maxYaw;
 yaw=wrapped?wrapYaw(values[0]):values[0];
 pitchUp=values[1];
 if (wrappedOut) *wrappedOut=wrapped;
 return true;
}
template<class Read>
bool follow(std::uintptr_t from, std::uintptr_t offset, Read read, std::uintptr_t& out) {
 if (!from || offset>~std::uintptr_t(0)-from) return false;
 std::uintptr_t value=0;
 if (!read(from+offset,&value,sizeof(value)) || !validPointer(value)) return false;
 out=value; return true;
}
// Plain pointer dereference without an offset hop.
template<class Read>
bool deref(std::uintptr_t address, Read read, std::uintptr_t& out) {
 std::uintptr_t value=0;
 if (!read(address,&value,sizeof(value)) || !validPointer(value)) return false;
 out=value; return true;
}

template<class Read>
bool readRotation(std::uintptr_t base, std::uintptr_t clientInstance, Read read, Reading& out) {
 out=Reading{};
 bool have=false;
 auto accept=[&](float yaw, float pitchUp, bool wrapped, unsigned bit) {
  if (!have) { out.yaw=yaw; out.pitchUp=pitchUp; out.wrapped=wrapped; have=true; }
  else if (std::fabs(yaw-out.yaw)<=0.02f) out.agree=true;
  out.sources|=bit;
 };
 // Source 1: the player object itself. The adapter-proven ClientInstance+0x60
 // slot does not depend on the published globals, and the table's velocity
 // block ends exactly where the angle offsets begin.
 if (clientInstance) {
  std::uintptr_t player=0, playerAgain=0;
  if (follow(clientInstance,playerSlot,read,player) &&
      follow(clientInstance,playerSlot,read,playerAgain) && player==playerAgain) {
   float yaw=0, pitchUp=0; bool wrapped=false;
   if (readAnglesAt(player,read,yaw,pitchUp,&wrapped)) accept(yaw,pitchUp,wrapped,1u);
  }
 }
 // Source 2: published global chain A: *global -> +0x18 -> +0x58 -> angles.
 std::uintptr_t root=0, a=0, holder=0;
 if (base && deref(base+chainGlobalA,read,root) && follow(root,rootHopA,read,a) && follow(a,rotationSlot,read,holder)) {
  float yaw=0, pitchUp=0; bool wrapped=false;
  if (readAnglesAt(holder,read,yaw,pitchUp,&wrapped)) accept(yaw,pitchUp,wrapped,2u);
 }
 // Source 3: published global chain B: *global -> +0x10 -> +0x58 -> angles.
 if (base && deref(base+chainGlobalB,read,root) && follow(root,rootHopB,read,a) && follow(a,rotationSlot,read,holder)) {
  float yaw=0, pitchUp=0; bool wrapped=false;
  if (readAnglesAt(holder,read,yaw,pitchUp,&wrapped)) accept(yaw,pitchUp,wrapped,4u);
 }
 // Source 4: the same +0x58 hop one step from the tick-observed ClientInstance
 // (the published chains suggest their intermediate is this instance; if it is
 // not, this source simply stays invalid).
 if (clientInstance && follow(clientInstance,rotationSlot,read,holder)) {
  float yaw=0, pitchUp=0; bool wrapped=false;
  if (readAnglesAt(holder,read,yaw,pitchUp,&wrapped)) accept(yaw,pitchUp,wrapped,8u);
 }
 return have;
}

// Collect the raw pointer trail and candidate angle slots for display only.
template<class Read>
void snapshot(std::uintptr_t base, std::uintptr_t clientInstance, Read read, Diagnostics& out) {
 out=Diagnostics{};
 out.clientInstance=clientInstance;
 if (clientInstance) {
  read(clientInstance+playerSlot,&out.player,sizeof(out.player));
  read(clientInstance+rotationSlot,&out.rotationHolder,sizeof(out.rotationHolder));
 }
 if (base) {
  if (read(base+chainGlobalA,&out.rootA,sizeof(out.rootA)) && validPointer(out.rootA))
   read(out.rootA+rootHopA,&out.interA,sizeof(out.interA));
  if (read(base+chainGlobalB,&out.rootB,sizeof(out.rootB)) && validPointer(out.rootB))
   read(out.rootB+rootHopB,&out.interB,sizeof(out.interB));
 }
 if (validPointer(out.interA)) read(out.interA+rotationSlot,&out.holderA,sizeof(out.holderA));
 if (validPointer(out.interB)) read(out.interB+rotationSlot,&out.holderB,sizeof(out.holderB));
 out.playerAnglesRead=out.player && read(out.player+yawOffset,out.playerAngles,sizeof(out.playerAngles));
 out.holderAnglesRead=out.rotationHolder && read(out.rotationHolder+yawOffset,out.holderAngles,sizeof(out.holderAngles));
 out.holderAAnglesRead=out.holderA && read(out.holderA+yawOffset,out.holderAAngles,sizeof(out.holderAAngles));
 out.holderBAnglesRead=out.holderB && read(out.holderB+yawOffset,out.holderBAngles,sizeof(out.holderBAngles));
 out.playerHeadRead=out.player && read(out.player+headOffset,&out.playerHead,sizeof(out.playerHead));
}

// Majority vote for the yaw direction convention. When the player moves, the
// horizontal velocity should mostly align with the look direction (players
// walk forward far more often than backwards). A sustained strong majority of
// anti-aligned samples flips the sign once; undecided states change nothing.
struct YawSignVote {
 int votes=0, positive=0;
 void sample(float alignment) {
  if (!std::isfinite(alignment) || std::fabs(alignment)<0.7f) return;  // strafing excluded
  ++votes; if (alignment>0) ++positive;
 }
 // +1 confirmed as-is, -1 confirmed flipped, 0 not enough evidence yet.
 int result() const {
  if (votes<6) return 0;
  const int positiveMajority=positive*4>=votes*3;       // >= 75%
  const int negativeMajority=(votes-positive)*4>=votes*3;
  return positiveMajority ? 1 : negativeMajority ? -1 : 0;
 }
 void reset() { votes=0; positive=0; }
};
}
