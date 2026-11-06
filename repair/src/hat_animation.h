#pragma once
#include <cmath>
#include <cstdint>
namespace hat_animation {
// A gentle lift above the calibrated head anchor; period is in milliseconds.
// No accumulated timestep drift and no dependence on frame rate.
inline float bob(std::uint64_t now) {
 constexpr std::uint64_t period=3200;
 constexpr float tau=6.2831853071795864769f;
 constexpr float amplitude=.07f;
 return amplitude*(1.0f+std::sin(tau*static_cast<float>(now%period)/static_cast<float>(period)));
}
}
