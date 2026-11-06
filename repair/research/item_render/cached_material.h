#pragma once
#include <cstddef>
#include <cstdint>

// Research model of the branches of 0x59f200 in the identified 1.1.5 EXE.
// This does not call game code, validate memory, or imply a material is suitable
// for a custom mesh.  In particular, required shader attributes and visual
// output are still unknown. Keep this offline until runtime validation exists.
namespace cached_material_research {
enum class Call { mesh, textured, dualTextured };
struct Plan {
 std::size_t material;
 Call call;
 std::size_t texture; // Offset in item context, zero when not used.
 std::size_t auxiliary; // Offset in renderer, zero when not used.
};
constexpr Plan plan(bool e8, bool e9, bool ea, bool mode) {
 if (mode) return {e8 ? 0x240u : e9 ? 0x210u : 0x1e0u,Call::dualTextured,0x90u,0x2a0u};
 if (e8) return {0x180u,Call::mesh,0,0};
 return {ea ? 0x1b0u : e9 ? 0x150u : 0x120u,Call::textured,0x90u,0};
}
}
