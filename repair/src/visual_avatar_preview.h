#pragma once
// Independent, deterministic preview geometry. The game's settings, mesh and
// animation functions are shared, but this is not the game's player model or
// shader. It uses an original, simplified Steve-style sample avatar.
#include "../research/china_hat/hat_mesh.h"
#include "hat_animation.h"
#include "visual_gradient.h"
#include <algorithm>
#include <cmath>
#include <cstdint>

namespace visual_avatar_preview {
struct HatSettings {
 float radius, height, offset, bobAmount, bobSpeed, rainbowSpeed;
 bool gradient;
};
inline bool hat(china_hat::Mesh& mesh,const HatSettings& s,std::uint64_t now) {
 const float phase=s.gradient ? visual_gradient::phase(now,s.rainbowSpeed) : static_cast<float>(now%8000)/8000.f;
 // Sample head has a top at y=.64; use the game's calibrated .03 offset and
 // the exact same bob function/radius/height/phase as the game renderer.
 const china_hat::Point anchor{0.f,.64f+.03f+s.offset+hat_animation::bob(now,s.bobAmount,s.bobSpeed),0.f};
 return china_hat::build(mesh,anchor,s.radius,s.height,48,phase,255);
}
struct Point2 { float x,y; };
inline Point2 project(china_hat::Point p,float scale=125.f) {
 return {140.f+(p.x-p.z*.22f)*scale, 205.f-(p.y+.35f)*scale+p.z*scale*.24f};
}
inline float hatScale(const HatSettings& s,std::uint64_t now) {
 const float top=.64f+.03f+s.offset+hat_animation::bob(now,s.bobAmount,s.bobSpeed)+s.height;
 return std::min({145.f,115.f/(s.radius*1.03f),190.f/std::max(.5f,top+.35f+s.radius*.24f)});
}
inline Point2 trailPoint(float t,std::uint64_t now,float lifetime) {
 t=std::clamp(t,0.f,1.f); const float phase=visual_gradient::phase(now,.18f)*6.28318530718f;
 float length=70.f+110.f*std::clamp((lifetime-1.f)/5.f,0.f,1.f);
 // t=0 is the moving player's position; t=1 is the oldest trail end.
 return {226.f-t*length,183.f+8.f*std::sin(phase+t*4.4f)*t};
}
inline float trailHeight(float normalizedHeight) { return std::clamp(normalizedHeight,.3f,1.f)*80.f; }
}
