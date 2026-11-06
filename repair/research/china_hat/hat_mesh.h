#pragma once
// Standalone experimental cone mesh. No Minecraft hooks, memory access, or
// rendering side effects. The caller must supply the interpolated world-space
// position of the head, relative to the render origin; the game-specific
// camera/model binding for 1.1.5 is not yet established.
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>

namespace china_hat {
struct Point { float x, y, z; };
struct Color { std::uint8_t r, g, b, a; };
struct Vertex { Point position; Color color; };
constexpr std::size_t kMaxSegments = 96;
struct Mesh {
    // A triangle list is usable by either a game's tessellator or a GPU buffer.
    std::array<Vertex, kMaxSegments * 3> triangles{};
    std::array<Vertex, kMaxSegments + 1> outline{};
    std::size_t triangleVertices = 0, outlineVertices = 0;
};
inline Color rainbow(float hue, std::uint8_t alpha) {
    hue = hue - std::floor(hue);
    float t = hue * 6.0f;
    int sector = static_cast<int>(t);
    float f = t - static_cast<float>(sector);
    auto scale = [](float v) { return static_cast<std::uint8_t>(std::lround(v * 255.0f)); };
    const float dim = 0.22f, mid = dim + (1.0f - dim) * f, inv = 1.0f - (1.0f - dim) * f;
    switch (sector) {
    case 0: return {255, scale(mid), scale(dim), alpha};
    case 1: return {scale(inv), 255, scale(dim), alpha};
    case 2: return {scale(dim), 255, scale(mid), alpha};
    case 3: return {scale(dim), scale(inv), 255, alpha};
    case 4: return {scale(mid), scale(dim), 255, alpha};
    default: return {255, scale(dim), scale(inv), alpha};
    }
}
// Start empty on invalid input. Explicit limits prevent huge / nonfinite world
// camera-relative coordinates from generating invalid vertices. Subtract the
// render origin before calling, so distant world positions keep float precision.
// phase is cycles, not seconds.
inline bool build(Mesh& out, Point head, float radius, float height,
                  std::size_t segments, float phase, std::uint8_t alpha = 160) {
    out.triangleVertices = out.outlineVertices = 0;
    if (!std::isfinite(head.x) || !std::isfinite(head.y) || !std::isfinite(head.z) ||
        std::fabs(head.x) > 32768.0f || std::fabs(head.y) > 32768.0f || std::fabs(head.z) > 32768.0f ||
        !std::isfinite(radius) || !std::isfinite(height) || !std::isfinite(phase) ||
        radius <= 0.0f || radius > 2.0f || height <= 0.0f || height > 2.0f ||
        segments < 8 || segments > kMaxSegments) return false;
    constexpr float tau = 6.2831853071795864769f;
    // Reduce before adding fractions so very large phases cannot overflow.
    phase -= std::floor(phase);
    const Point apex{head.x, head.y + height, head.z};
    auto rim = [&](std::size_t i) {
        float turn = static_cast<float>(i % segments) / static_cast<float>(segments);
        float angle = tau * turn;
        return Vertex{{head.x + radius * std::cos(angle), head.y,
                       head.z + radius * std::sin(angle)}, rainbow(turn + phase, alpha)};
    };
    for (std::size_t i = 0; i < segments; ++i) {
        Vertex a = rim(i), b = rim(i + 1);
        // Winding is consistent around the cone. Material/depth configuration
        // is deliberately left to the eventual game-specific renderer.
        out.triangles[i * 3] = {apex, rainbow(phase + (static_cast<float>(i) + 0.5f) / segments, alpha)};
        out.triangles[i * 3 + 1] = a;
        out.triangles[i * 3 + 2] = b;
        out.outline[i] = a;
    }
    out.outline[segments] = out.outline[0];
    out.triangleVertices = segments * 3;
    out.outlineVertices = segments + 1;
    return true;
}
}
