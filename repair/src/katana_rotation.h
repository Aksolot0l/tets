#pragma once
#include <cmath>
#include <cstdint>
#include <cstddef>

// Read-only rotation sources for the exact 1.1.5 executable (EXE SHA-256
// 2883b72a3579d95bfec73330c5e06f2463253b7ca530dd59f50885a71a724732).
//
// The yaw-like float (player+layout "XRotation") and the pitch-like float
// ("YRotation", positive = looking up) were published for this exact build in
// a working third-party cheat table, alongside velocity fields that agree with
// the player layout already proven by this adapter (velocities at +0xAC/+0xB0/
// +0xB4 behind the local player slot). The table reaches them through two
// pointer chains; this adapter additionally tries the same offsets one hop
// from its own tick-observed ClientInstance. Every read is validated (aligned
// pointers, finite floats, bounded angles) and any failure simply leaves the
// reading invalid, so the caller can keep the original item untouched.
//
// These are observed offsets for one build, not a stable API.
namespace katana_rotation {
constexpr float maxYaw=3.35f;        // radians; wrapped yaw stays within +-pi
constexpr float maxPitch=1.75f;      // pitch is bounded to +-pi/2
constexpr std::uintptr_t chainGlobalA=0x1921DF8;  // -> +0x18 -> +0x58 -> angles
constexpr std::uintptr_t chainGlobalB=0x1921888;  // -> +0x10 -> +0x58 -> angles
constexpr std::uintptr_t clientHop=0x58;          // ClientInstance -> angles holder
constexpr std::uintptr_t yawOffset=0xB8;
constexpr std::uintptr_t pitchOffset=0xBC;

struct Reading {
 float yaw=0.f, pitchUp=0.f;
 unsigned sources=0;  // bit 1 client hop, bit 2 global A, bit 4 global B
 bool agree=false;    // two independent sources within 0.02 rad
};

inline bool validPointer(std::uint64_t value) {
 return value>=0x10000 && value<0x0000800000000000ULL && (value&7)==0;
}
inline bool validAngle(float v, float bound) {
 return std::isfinite(v) && std::fabs(v)<=bound;
}

// Read helper signature: bool(std::uintptr_t address, void* out, std::size_t n)
template<class Read>
bool readAnglesAt(std::uintptr_t object, Read read, float& yaw, float& pitchUp) {
 if (!validPointer(object)) return false;
 float values[2]{};
 if (!read(object+yawOffset,&values,sizeof(values))) return false;
 if (!validAngle(values[0],maxYaw) || !validAngle(values[1],maxPitch)) return false;
 // Re-read to reject torn mid-update values.
 float again[2]{};
 if (!read(object+yawOffset,&again,sizeof(again))) return false;
 if (again[0]!=values[0] || again[1]!=values[1]) return false;
 yaw=values[0]; pitchUp=values[1];
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
 float yaw=0, pitchUp=0;
 bool have=false;
 std::uintptr_t holder=0;
 // Source 1: one hop from the tick-observed ClientInstance.
 if (clientInstance && follow(clientInstance,clientHop,read,holder)) {
  float y=0,p=0;
  if (readAnglesAt(holder,read,y,p)) {
   out.yaw=y; out.pitchUp=p; out.sources|=1; have=true;
  }
 }
 // Source 2: published global chain A: *global -> +0x18 -> +0x58 -> angles.
 std::uintptr_t root=0, a=0;
 if (base && deref(base+chainGlobalA,read,root) && follow(root,0x18,read,a) && follow(a,clientHop,read,holder)) {
  float y=0,p=0;
  if (readAnglesAt(holder,read,y,p)) {
   if (!have) { out.yaw=y; out.pitchUp=p; have=true; }
   else if (std::fabs(y-out.yaw)<=0.02f) out.agree=true;
   out.sources|=2;
  }
 }
 // Source 3: published global chain B: *global -> +0x10 -> +0x58 -> angles.
 if (base && deref(base+chainGlobalB,read,root) && follow(root,0x10,read,a) && follow(a,clientHop,read,holder)) {
  float y=0,p=0;
  if (readAnglesAt(holder,read,y,p)) {
   if (!have) { out.yaw=y; out.pitchUp=p; have=true; }
   else if (std::fabs(y-out.yaw)<=0.02f) out.agree=true;
   out.sources|=4;
  }
 }
 return have;
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
