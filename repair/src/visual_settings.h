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
 JumpRadius, JumpSeconds, JumpOpacity, JumpRed, JumpGreen, JumpBlue, JumpRainbowSpeed,
 AccessoryScale, AccessoryOffset, AccessoryOpacity, AccessoryRed, AccessoryGreen, AccessoryBlue,
 KatanaScale, KatanaTilt, KatanaSide, KatanaHeight, KatanaForward, KatanaOpacity,
 KatanaFirstSide, KatanaFirstForward,
 HitCount, HitSize, HitSpeed, HitLife, HitGravity, HitReach,
 HitRingCount, HitRingSize, HitRingLife, HitCycleSpeed,
 HitARed, HitAGreen, HitABlue, HitBRed, HitBGreen, HitBBlue,
 Hat2Red, Hat2Green, Hat2Blue, Trail2Red, Trail2Green, Trail2Blue,
 Jump2Red, Jump2Green, Jump2Blue, Count
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
 {.20f,0.f,1.f},{1.f,0.f,1.f},{.42f,0.f,1.f},{.15f,.03f,.8f},
 {1.f,.5f,2.f},{0.f,-.20f,.50f},{1.f,0.f,1.f},
 {1.f,0.f,1.f},{.76f,0.f,1.f},{.14f,0.f,1.f},
 {.052f,.020f,.100f},{1.0f,.20f,1.55f},{.32f,0.f,.80f},
 {1.30f,.60f,1.80f},{.10f,-.50f,1.00f},{1.f,0.f,1.f},
 {.34f,0.f,.80f},{.30f,-.30f,1.00f},
 {24.f,4.f,48.f},{.32f,.08f,.80f},{4.2f,1.f,10.f},{.70f,.25f,1.60f},{5.5f,0.f,14.f},{2.8f,1.f,4.5f},
 {3.f,0.f,5.f},{1.4f,.40f,3.0f},{.55f,.20f,1.2f},{.90f,.05f,3.f},
 {1.f,0.f,1.f},{1.f,0.f,1.f},{1.f,0.f,1.f},{1.f,0.f,1.f},{.15f,0.f,1.f},{.10f,0.f,1.f},
 {1.f,0.f,1.f},{1.f,0.f,1.f},{1.f,0.f,1.f},{1.f,0.f,1.f},{1.f,0.f,1.f},{1.f,0.f,1.f},
 {1.f,0.f,1.f},{1.f,0.f,1.f},{1.f,0.f,1.f}
};
static_assert(sizeof(visualParamSpecs)/sizeof(visualParamSpecs[0])==static_cast<std::size_t>(VisualParam::Count),"visual parameters mismatch");
inline float sanitizeVisualParam(VisualParam param,float value) {
 auto i=static_cast<std::size_t>(param);
 if (i>=static_cast<std::size_t>(VisualParam::Count)) return 0.f;
 const auto spec=visualParamSpecs[i];
 return std::isfinite(value) ? std::clamp(value,spec.low,spec.high) : spec.value;
}
