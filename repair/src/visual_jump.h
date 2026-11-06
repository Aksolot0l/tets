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
inline float alpha(float p) {
 p=std::clamp((p-.12f)/.88f,0.f,1.f);
 return 1.f-p*p*(3.f-2.f*p); // visible core followed by a smooth fade
}
constexpr int ringSteps=48, arcCount=8, arcSteps=4, sparkCount=8;
constexpr int haloVertices=ringSteps*12*2;
constexpr int detailVertices=ringSteps*12*2+arcCount*arcSteps*12+sparkCount*24;
// Annulus triangle list, double-sided. Vertices are in world-relative space.
template<class Emit> void ring(china_hat::Point c,float r,float width,Emit emit) {
 constexpr int steps=ringSteps; constexpr float tau=6.283185307179586f;
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
// Build a broken ring in the same plane. Double-sided triangles keep the
// effect visible with the game's existing rasterizer/culling state.
template<class Emit> void arc(china_hat::Point c,float r,float width,float start,float span,Emit emit) {
 constexpr int steps=arcSteps;
 const float inner=std::max(0.f,r-width);
 for(int i=0;i<steps;++i) {
  const float a=start+span*i/steps,b=start+span*(i+1)/steps;
  china_hat::Point o1{c.x+std::cos(a)*r,c.y,c.z+std::sin(a)*r};
  china_hat::Point o2{c.x+std::cos(b)*r,c.y,c.z+std::sin(b)*r};
  china_hat::Point i1{c.x+std::cos(a)*inner,c.y,c.z+std::sin(a)*inner};
  china_hat::Point i2{c.x+std::cos(b)*inner,c.y,c.z+std::sin(b)*inner};
  emit(o1);emit(i1);emit(o2); emit(o2);emit(i1);emit(i2);
  emit(o2);emit(i1);emit(o1); emit(i2);emit(i1);emit(o2);
 }
}
// Two perpendicular diamonds are visible from either side without billboards,
// textures, custom shaders, unverified vertex gradients, or camera access.
template<class Emit> void spark(china_hat::Point c,float size,Emit emit) {
 for(int axis=0;axis<2;++axis) {
  china_hat::Point top{c.x,c.y+size*1.65f,c.z}, bottom{c.x,c.y-size*1.65f,c.z};
  china_hat::Point left{c.x-(axis==0?size:0.f),c.y,c.z-(axis==1?size:0.f)};
  china_hat::Point right{c.x+(axis==0?size:0.f),c.y,c.z+(axis==1?size:0.f)};
  emit(top);emit(left);emit(bottom); emit(top);emit(bottom);emit(right);
  emit(bottom);emit(left);emit(top); emit(right);emit(bottom);emit(top);
 }
}
template<class Emit> void halo(china_hat::Point c,float r,Emit emit) {
 ring(c,r+.065f,.17f,emit);
 ring(c,std::max(.12f,r*.76f),.085f,emit);
}
template<class Emit> void detail(china_hat::Point c,float r,float p,Emit emit) {
 constexpr float tau=6.283185307179586f;
 c.y+=.003f;
 ring(c,r,.038f,emit);
 ring(c,std::max(.12f,r*.76f),.018f,emit);
 // The arcs rotate slowly as the ripples expand.
 for(int i=0;i<arcCount;++i)
  arc(c,std::max(.13f,r*.87f),.028f,tau*i/arcCount+p*.65f,.35f,emit);
 // Outward and upward drifting geometric sparkles; they share the effect's
 // material color and fade, so selected colors and rainbow still apply.
 for(int i=0;i<sparkCount;++i) {
  const float angle=tau*i/sparkCount+p*.24f;
  const float distance=r*(.83f+.16f*p);
  const float height=.07f+.24f*p+((i%2)? .035f:0.f);
  china_hat::Point pos{c.x+std::cos(angle)*distance,c.y+height,c.z+std::sin(angle)*distance};
  spark(pos,.025f+.012f*(1.f-p),emit);
 }
}
}
