#pragma once
#include <algorithm>
#include <cmath>
#include <cstddef>

// Per-session settings only. Every value exposed in the UI is consumed by the
// renderer and clamped before publication to the render thread.
enum class VisualParam : std::size_t {
 HatRadius, HatConeHeight, HatOffset, HatBobAmplitude, HatBobSpeed, HatRainbowSpeed,
 HatRed, HatGreen, HatBlue, TrailHeight, TrailSeconds, TrailRainbowSpeed,
 TrailRed, TrailGreen, TrailBlue, HatOpacity, TrailOpacity,
 JumpRadius, JumpSeconds, JumpOpacity, JumpRed, JumpGreen, JumpBlue, JumpRainbowSpeed, Count
};
struct VisualParamSpec { float value, low, high; };
constexpr VisualParamSpec visualParamSpecs[] = {
 {.58f,.30f,1.00f}, {.34f,.12f,.80f}, {0.f,-.20f,.40f},
 {.07f,0.f,.20f}, {.3125f,.10f,1.f}, {.15f,.03f,.8f},
 {.20f,0.f,1.f},{1.f,0.f,1.f},{.42f,0.f,1.f},
 {.80f,.30f,1.00f},{3.f,1.f,6.f},{.15f,.03f,.8f},
 {.08f,0.f,1.f},{.86f,0.f,1.f},{1.f,0.f,1.f},
 {1.f,0.f,1.f},{1.f,0.f,1.f},
 {1.4f,.4f,3.f},{1.f,.3f,3.f},{.85f,0.f,1.f},
 {.20f,0.f,1.f},{1.f,0.f,1.f},{.42f,0.f,1.f},{.15f,.03f,.8f}
};
static_assert(sizeof(visualParamSpecs)/sizeof(visualParamSpecs[0])==static_cast<std::size_t>(VisualParam::Count),"visual parameters mismatch");
inline float sanitizeVisualParam(VisualParam param,float value) {
 auto i=static_cast<std::size_t>(param);
 if (i>=static_cast<std::size_t>(VisualParam::Count)) return 0.f;
 const auto spec=visualParamSpecs[i];
 return std::isfinite(value) ? std::clamp(value,spec.low,spec.high) : spec.value;
}
