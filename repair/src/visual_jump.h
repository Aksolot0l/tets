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
   clear(); actor_=actor; prev_=anchor_=p; peak_=p.y; last_=now; seen_=true; return;
  }
  float dx=p.x-prev_.x, dy=p.y-prev_.y, dz=p.z-prev_.z;
  if (dx*dx+dy*dy+dz*dz>16.f) { clear(); actor_=actor; prev_=anchor_=p; peak_=p.y; last_=now; seen_=true; return; }
  expire(now,ttl);
  // Start armed immediately; a 100ms stable-Y requirement misses held-space
  // auto-jumps and moving players. Accumulate ascent from the latest valley
  // and rearm after descent, with hysteresis against tiny interpolation jitter.
  if (ascending_) {
   peak_=std::max(peak_,p.y);
   if (peak_-p.y>.08f) { ascending_=false; anchor_=p; }
  } else if (p.y-anchor_.y>.08f) {
   if (count_==capacity) { for (std::size_t i=1;i<count_;++i) rings_[i-1]=rings_[i]; --count_; }
   rings_[count_++]={anchor_,now}; ascending_=true; peak_=p.y;
  } else if (p.y<=anchor_.y) {
   anchor_=p;
  } else if (p.y-anchor_.y<.015f) {
   // Follow level horizontal movement without lifting the ring off the floor.
   anchor_.x=p.x; anchor_.z=p.z;
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
 china_hat::Point prev_{},anchor_{}; std::uint64_t last_=0;
 float peak_=0; bool seen_=false,ascending_=false;
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
