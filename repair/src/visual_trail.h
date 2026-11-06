#pragma once
#include "../research/china_hat/hat_mesh.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <cmath>

// Bounded history in world space. No platform, input, game or rendering calls.
namespace visual_trail {
struct Sample { china_hat::Point point; std::uint64_t time=0; };
class History {
public:
 static constexpr std::size_t capacity=128;
 static constexpr std::uint64_t lifetime=3000;
 void clear() { count_=0; actor_=0; lastSeen_=0; }
 void update(std::uintptr_t actor, china_hat::Point p, std::uint64_t now, std::uint64_t ttl=lifetime) {
  if (!actor || !std::isfinite(p.x) || !std::isfinite(p.y) || !std::isfinite(p.z)) { clear(); return; }
  if (actor_!=actor || (lastSeen_ && (now<lastSeen_ || now-lastSeen_>1000))) { clear(); actor_=actor; }
  lastSeen_=now; expire(now,ttl);
  if (count_) {
   const auto& last=points_[count_-1];
   float dx=p.x-last.point.x, dy=p.y-last.point.y, dz=p.z-last.point.z;
   float distance2=dx*dx+dy*dy+dz*dz;
   if (!std::isfinite(distance2) || distance2>64.f) { clear(); actor_=actor; lastSeen_=now; }
   else if (distance2<.0025f || now-last.time<50) return;
  }
  if (count_==capacity) { for (std::size_t i=1;i<count_;++i) points_[i-1]=points_[i]; --count_; }
  points_[count_++]={p,now};
 }
 void expire(std::uint64_t now,std::uint64_t ttl=lifetime) {
  std::size_t n=0;
  while (n<count_ && (now<points_[n].time || now-points_[n].time>ttl)) ++n;
  if (n) { for (std::size_t i=n;i<count_;++i) points_[i-n]=points_[i]; count_-=n; }
 }
 std::size_t size() const { return count_; }
 const Sample& operator[](std::size_t i) const { return points_[i]; }
private:
 std::array<Sample,capacity> points_{};
 std::size_t count_=0;
 std::uintptr_t actor_=0;
 std::uint64_t lastSeen_=0;
};
}
