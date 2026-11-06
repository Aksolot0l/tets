#pragma once
#include "../research/china_hat/hat_mesh.h"
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>

namespace visual_gradient {
inline float phase(std::uint64_t ms, float cyclesPerSecond) {
 if (!std::isfinite(cyclesPerSecond) || cyclesPerSecond < 0) return 0.f;
 return static_cast<float>(std::fmod(static_cast<double>(ms)*.001*cyclesPerSecond,1.0));
}
// Samples run from oldest to newest. Geometry interpolation between samples is
// controlled by the game's vertex material; this code does not enable blending.
inline china_hat::Color trail(std::size_t index, std::size_t count, float timePhase, std::uint8_t alpha) {
 if (count < 2 || !std::isfinite(timePhase)) return china_hat::rainbow(0.f,alpha);
 const float position=static_cast<float>(std::min(index,count-1))/static_cast<float>(count-1);
 // Less than a full hue turn so the newest and oldest ends differ.
 return china_hat::rainbow(timePhase + position*.8f,alpha);
}
}
