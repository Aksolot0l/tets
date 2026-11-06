#pragma once
#include <array>

// Candidate material tint. Disabled is bit-for-bit the original white shader
// value; the old game's active material may ignore this tint entirely.
namespace visual_color {
enum class Effect { Hat, Trail };
inline std::array<float,4> material(bool enabled,Effect effect) {
 if (!enabled) return {{1.f,1.f,1.f,1.f}};
 return effect==Effect::Hat ? std::array<float,4>{{0.20f,1.f,0.42f,1.f}}
                            : std::array<float,4>{{0.08f,0.86f,1.f,1.f}};
}
}
