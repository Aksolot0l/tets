#include "../src/katana_rotation.h"
#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <limits>
#include <unordered_map>
using namespace katana_rotation;

// Tiny mock of the validated process-memory read used by the adapter.
struct MockMemory {
 std::unordered_map<std::uintptr_t,std::uint8_t> bytes;
 // When set, the second and later 8-byte reads at tearAddress return these
 // values instead, simulating a rotation update between the two reads.
 std::uintptr_t tearAddress=0; int tearReads=0; float tearYaw=0.f, tearPitch=0.f;
 bool read(std::uintptr_t address,void* out,std::size_t n) {
  if (address==tearAddress && n==8) {
   ++tearReads;
   if (tearReads>=2) {
    float torn[2]{tearYaw,tearPitch};
    std::memcpy(out,torn,8);
    return true;
   }
  }
  for (std::size_t i=0;i<n;++i) {
   auto it=bytes.find(address+i);
   if (it==bytes.end()) return false;
   static_cast<std::uint8_t*>(out)[i]=it->second;
  }
  return true;
 }
 void put(std::uintptr_t address,const void* data,std::size_t n) {
  const auto* p=static_cast<const std::uint8_t*>(data);
  for (std::size_t i=0;i<n;++i) bytes[address+i]=p[i];
 }
 void putPointer(std::uintptr_t address,std::uintptr_t value) { put(address,&value,sizeof(value)); }
 void putFloat(std::uintptr_t address,float value) { put(address,&value,sizeof(value)); }
};

int main() {
 const std::uintptr_t base=0x500000, client=0x1000000, holder=0x2000000;
 MockMemory memory;
 // Source 1: ClientInstance -> +0x58 -> angles object.
 memory.putPointer(client+clientHop,holder);
 memory.putFloat(holder+yawOffset,0.75f);
 memory.putFloat(holder+pitchOffset,-0.30f);
 Reading r;
 assert(readRotation(base,client,[&](std::uintptr_t a,void* o,std::size_t n){ return memory.read(a,o,n); },r));
 assert(r.sources==1 && std::fabs(r.yaw-0.75f)<1e-6f && std::fabs(r.pitchUp+0.30f)<1e-6f && !r.agree);

 // Source 2: published global chain A agrees -> agreement flag set.
 const std::uintptr_t globalA=0x3000000, midA=0x3100000;
 memory.putPointer(base+chainGlobalA,globalA);
 memory.putPointer(globalA+0x18,midA);
 memory.putPointer(midA+clientHop,holder);
 assert(readRotation(base,client,[&](std::uintptr_t a,void* o,std::size_t n){ return memory.read(a,o,n); },r));
 assert(r.sources==3 && r.agree);

 // Source 3: published global chain B pointing at a stale but in-range object
 // is still a valid fallback when the other sources disappear.
 const std::uintptr_t globalB=0x4000000, midB=0x4100000, other=0x4200000;
 memory.putPointer(base+chainGlobalB,globalB);
 memory.putPointer(globalB+0x10,midB);
 memory.putPointer(midB+clientHop,other);
 memory.putFloat(other+yawOffset,-2.5f);
 memory.putFloat(other+pitchOffset,0.9f);
 MockMemory noClient;
 noClient.putPointer(base+chainGlobalA,globalA);
 noClient.putPointer(globalA+0x18,midA);
 noClient.putPointer(midA+clientHop,holder);
 noClient.putFloat(holder+yawOffset,0.75f);
 noClient.putFloat(holder+pitchOffset,-0.30f);
 noClient.putPointer(base+chainGlobalB,globalB);
 noClient.putPointer(globalB+0x10,midB);
 noClient.putPointer(midB+clientHop,other);
 noClient.putFloat(other+yawOffset,-2.5f);
 noClient.putFloat(other+pitchOffset,0.9f);
 assert(readRotation(base,0,[&](std::uintptr_t a,void* o,std::size_t n){ return noClient.read(a,o,n); },r));
 assert(r.sources==6 && std::fabs(r.yaw-0.75f)<1e-6f);

 // Garbage pointers and out-of-range angles are rejected per source.
 MockMemory bad;
 bad.putPointer(client+clientHop,0x7);           // unaligned
 bad.putPointer(base+chainGlobalA,globalA);
 bad.putPointer(globalA+0x18,midA);
 bad.putPointer(midA+clientHop,holder);
 bad.putFloat(holder+yawOffset,99.f);            // out of range
 assert(!readRotation(base,client,[&](std::uintptr_t a,void* o,std::size_t n){ return bad.read(a,o,n); },r));

 // A torn update (second read differs) invalidates the source.
 MockMemory torn;
 torn.putPointer(client+clientHop,holder);
 torn.putFloat(holder+yawOffset,0.4f);
 torn.putFloat(holder+pitchOffset,0.1f);
 torn.tearAddress=holder+yawOffset; torn.tearYaw=0.5f; torn.tearPitch=0.1f;
 assert(!readRotation(base,client,[&](std::uintptr_t a,void* o,std::size_t n){ return torn.read(a,o,n); },r));

 // No sources at all -> invalid reading, caller keeps the original item.
 MockMemory empty;
 assert(!readRotation(base,client,[&](std::uintptr_t a,void* o,std::size_t n){ return empty.read(a,o,n); },r));

 // Angle validation bounds.
 assert(validAngle(3.2f,maxYaw) && !validAngle(3.4f,maxYaw));
 assert(validAngle(-1.5f,maxPitch) && !validAngle(1.8f,maxPitch));
 assert(!validAngle(std::numeric_limits<float>::quiet_NaN(),maxYaw));
 assert(validPointer(0x10000) && !validPointer(0x10000-1) && !validPointer(8) && !validPointer(0xffffffffffffffffULL));

 // Yaw sign voting: forward walking confirms, sustained backwards flips.
 YawSignVote vote;
 for (int i=0;i<6;++i) vote.sample(0.9f);
 assert(vote.result()==1);
 YawSignVote flip;
 for (int i=0;i<6;++i) flip.sample(-0.95f);
 assert(flip.result()==-1);
 YawSignVote mixed;
 for (int i=0;i<10;++i) mixed.sample(i%2 ? 0.8f : -0.8f);
 assert(mixed.result()==0);
 YawSignVote undecided;
 for (int i=0;i<5;++i) undecided.sample(0.99f);
 assert(undecided.result()==0);
 // Strafing-like samples (|alignment| below the gate) are ignored entirely.
 YawSignVote strafe;
 for (int i=0;i<20;++i) strafe.sample(0.1f);
 assert(strafe.result()==0 && strafe.votes==0);
 // Non-finite samples never count.
 strafe.sample(std::numeric_limits<float>::quiet_NaN());
 assert(strafe.votes==0);
}
