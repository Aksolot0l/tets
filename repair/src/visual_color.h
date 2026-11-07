#pragma once
#include <array>
#include <cmath>
#include <cstdint>

namespace visual_color {
enum class Effect { Hat, Trail };
inline std::array<float,4> material(bool enabled,Effect effect) {
 if (!enabled) return {{1.f,1.f,1.f,1.f}};
 return effect==Effect::Hat ? std::array<float,4>{{0.20f,1.f,0.42f,1.f}}
                            : std::array<float,4>{{0.08f,0.86f,1.f,1.f}};
}
// Animate the whole material color; 1.1.5's active material did not honor our
// per-vertex gradient in the user's test. Never imply per-vertex rainbow.
// Two-color gradient: the effect breathes between baseA and baseB; phase is
// in cycles (0 -> A, 0.5 -> B, 1 -> A again).
inline std::array<float,4> gradient(std::array<float,3> baseA,std::array<float,3> baseB,float phase) {
 float w=0.5f-0.5f*std::cos(6.28318530718f*phase);
 return {{baseA[0]+(baseB[0]-baseA[0])*w,baseA[1]+(baseB[1]-baseA[1])*w,
          baseA[2]+(baseB[2]-baseA[2])*w,1.f}};
}
inline std::array<float,4> animated(bool enabled,bool rainbow,std::array<float,3> base,float cyclesPerSecond,std::uint64_t ms) {
 if (!enabled) return {{1.f,1.f,1.f,1.f}};
 if (!rainbow) return {{base[0],base[1],base[2],1.f}};
 double hue=std::fmod(static_cast<double>(ms)*0.001*static_cast<double>(cyclesPerSecond),1.0);
 float x=static_cast<float>(hue*6.0); int sector=static_cast<int>(x); float f=x-sector;
 constexpr float floor=.22f; float rise=floor+(1.f-floor)*f, fall=1.f-(1.f-floor)*f;
 switch (sector) {
  case 0: return {{1.f,rise,floor,1.f}};
  case 1: return {{fall,1.f,floor,1.f}};
  case 2: return {{floor,1.f,rise,1.f}};
  case 3: return {{floor,fall,1.f,1.f}};
  case 4: return {{rise,floor,1.f,1.f}};
  default:return {{1.f,floor,fall,1.f}};
 }
}
}
