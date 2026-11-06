#pragma once
#include <cmath>
#include <cstdint>
namespace hat_animation {
// A gentle lift above the calibrated head anchor; period is in milliseconds.
// No accumulated timestep drift and no dependence on frame rate.
inline float bob(std::uint64_t now,float amplitude=.07f,float speed=.3125f) {
 constexpr double tau=6.2831853071795864769;
 const double phase=std::fmod(static_cast<double>(now)*.001*static_cast<double>(speed),1.0);
 return amplitude*(1.0f+static_cast<float>(std::sin(tau*phase)));
}
}
