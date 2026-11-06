#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include "hat_animation.h"
#include "visual_gradient.h"

// Screen-space illustration only. It shares the same settings/time base but
// intentionally does not claim to replicate the game's material or depth.
namespace visual_preview {
struct Point { float x,y; };
inline Point hatRim(float turn,float radius,float y) {
 constexpr float tau=6.28318530718f;
 const float a=turn*tau;
 return {140.f+std::cos(a)*radius*105.f,y+std::sin(a)*radius*27.f};
}
inline float hatY(std::uint64_t now,float offset,float amplitude,float speed) {
 return 145.f-offset*55.f-hat_animation::bob(now,amplitude,speed)*55.f;
}
inline Point trailPoint(float t,std::uint64_t now,float lifetime) {
 // A repeating stylised ribbon. Duration changes its displayed length.
 const float phase=visual_gradient::phase(now,.25f)*6.28318530718f;
 const float len=std::clamp(lifetime/6.f,1.f/6.f,1.f);
 return {240.f-t*(80.f+140.f*len),165.f+std::sin(t*4.0f+phase)*12.f};
}
}
