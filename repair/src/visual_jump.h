#pragma once
#include "../research/china_hat/hat_mesh.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>

// Movement-only approximation: no unverified game-memory on-ground flag.
// Bounded, world-space rings; all state belongs to one render thread.
namespace visual_jump {
struct Ring { china_hat::Point center{}; std::uint64_t start=0; };
class History {
public:
 static constexpr std::size_t capacity=8;
 void clear() { *this=History{}; }
 void update(std::uintptr_t actor,china_hat::Point p,std::uint64_t now,std::uint64_t ttl) {
  if (!actor || !std::isfinite(p.x) || !std::isfinite(p.y) || !std::isfinite(p.z)) { clear(); return; }
  if (actor_!=actor || (seen_ && (now<last_ || now-last_>500))) {
   clear(); actor_=actor; prev_=p; anchor_=p; stableSince_=last_=now; seen_=true; return;
  }
  float dx=p.x-prev_.x, dy=p.y-prev_.y, dz=p.z-prev_.z;
  if (dx*dx+dy*dy+dz*dz>16.f) { clear(); actor_=actor; prev_=anchor_=p; stableSince_=last_=now; seen_=true; return; }
  expire(now,ttl);
  // Arm only after roughly level movement. Detect cumulative upward movement,
  // because interpolation can make the per-frame delta arbitrarily small.
  if (std::fabs(p.y-anchor_.y)<.025f) {
   if (!armed_ && now-stableSince_>=100) armed_=true;
   if (armed_) { anchor_.x=p.x; anchor_.z=p.z; }
  } else if (armed_ && p.y-anchor_.y>.08f) {
   if (count_==capacity) { for (std::size_t i=1;i<count_;++i) rings_[i-1]=rings_[i]; --count_; }
   rings_[count_++]={anchor_,now}; armed_=false; anchor_=p; stableSince_=now;
  } else if (!armed_ || p.y<anchor_.y-.025f) {
   armed_=false; anchor_=p; stableSince_=now;
  }
  prev_=p; last_=now;
 }
 void expire(std::uint64_t now,std::uint64_t ttl) {
  std::size_t n=0; while(n<count_ && (now<rings_[n].start || now-rings_[n].start>=ttl)) ++n;
  if(n) { for(std::size_t i=n;i<count_;++i) rings_[i-n]=rings_[i]; count_-=n; }
 }
 std::size_t size() const { return count_; }
 const Ring& operator[](std::size_t i) const { return rings_[i]; }
private:
 std::array<Ring,capacity> rings_{}; std::size_t count_=0; std::uintptr_t actor_=0;
 china_hat::Point prev_{},anchor_{}; std::uint64_t stableSince_=0,last_=0;
 bool seen_=false,armed_=false;
};
inline float progress(std::uint64_t now,std::uint64_t start,std::uint64_t ttl) {
 if (!ttl || now<start) return 1.f;
 return std::clamp(static_cast<float>(now-start)/static_cast<float>(ttl),0.f,1.f);
}
inline float radius(float p,float maxRadius) { p=std::clamp(p,0.f,1.f); return .20f+(maxRadius-.20f)*(1.f-(1.f-p)*(1.f-p)); }
inline float alpha(float p) { p=std::clamp(p,0.f,1.f); return (1.f-p)*(1.f-p); }
// Annulus triangle list, double-sided. Vertices are in world-relative space.
template<class Emit> void ring(china_hat::Point c,float r,float width,Emit emit) {
 constexpr int steps=48; constexpr float tau=6.283185307179586f;
 const float inner=std::max(0.f,r-width);
 for(int i=0;i<steps;++i) {
  const float a=tau*i/steps,b=tau*(i+1)/steps;
  china_hat::Point o1{c.x+std::cos(a)*r,c.y,c.z+std::sin(a)*r};
  china_hat::Point o2{c.x+std::cos(b)*r,c.y,c.z+std::sin(b)*r};
  china_hat::Point i1{c.x+std::cos(a)*inner,c.y,c.z+std::sin(a)*inner};
  china_hat::Point i2{c.x+std::cos(b)*inner,c.y,c.z+std::sin(b)*inner};
  emit(o1);emit(i1);emit(o2); emit(o2);emit(i1);emit(i2);
  emit(o2);emit(i1);emit(o1); emit(i2);emit(i1);emit(o2);
 }
}
}
