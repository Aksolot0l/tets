#pragma once
// hit_fx.h - hit impact particles and rings, pure math only. No Windows, no
// D3D and no game memory here: the native side (china_hat_native.cpp) feeds
// positions/look vectors in and consumes the geometry through templates, so
// every curve, the pool and the burst spawn run in the Linux test harness
// with a seeded RNG. Rendering goes through the game's tessellator like the
// hat/trail/jump effects: world-relative vertices, per-vertex RGB, batch
// opacity through the shader tint (the pipeline has no per-vertex alpha).
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>

namespace hit_fx {

struct Vec3 {
    float x = 0.f, y = 0.f, z = 0.f;
};

inline Vec3 add(Vec3 a, Vec3 b) { return {a.x + b.x, a.y + b.y, a.z + b.z}; }
inline Vec3 scale(Vec3 a, float s) { return {a.x * s, a.y * s, a.z * s}; }
inline Vec3 cross(Vec3 a, Vec3 b) {
    return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}
inline float dot(Vec3 a, Vec3 b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
inline float len(Vec3 a) { return std::sqrt(dot(a, a)); }
inline Vec3 norm(Vec3 a) {
    const float l = len(a);
    return l > 1e-6f ? scale(a, 1.f / l) : Vec3{};
}

// Deterministic xorshift32 so a burst is reproducible in tests.
struct Rng {
    std::uint32_t s;
    explicit Rng(std::uint32_t seed) : s(seed ? seed : 0x9E3779B9u) {}
    std::uint32_t next() { s ^= s << 13; s ^= s >> 17; s ^= s << 5; return s; }
    float unit() { return static_cast<float>(next() & 0x00FFFFFFu) / 16777216.0f; }
    float range(float lo, float hi) { return lo + (hi - lo) * unit(); }
};

// Two-color shimmer: 0 -> color A, 1 -> color B, ping-ponging smoothly.
// w = 0.5 - 0.5*cos(2*pi*(t*speed + phase))
inline float colorMix(float t, float speed, float phase) {
    const float a = 6.28318530718f * (t * speed + phase);
    return 0.5f - 0.5f * std::cos(a);
}

// Particle size over normalized life: quick pop above 1, then decay to 0.
inline float sizeCurve(float t) {
    t = std::clamp(t, 0.f, 1.f);
    if (t < 0.15f) return 0.55f + 0.60f * std::sin((t / 0.15f) * 1.57079632f);
    const float nt = (t - 0.15f) / 0.85f;
    return (1.15f - 0.15f * nt) * (1.f - nt * nt);
}

// Particle/ring opacity over normalized life: 1 -> 0, monotonically.
// 1 - 3t^2 + 2t^3 (smoothstep down, no popping at either end).
inline float alphaCurve(float t) {
    t = std::clamp(t, 0.f, 1.f);
    return 1.f - 3.f * t * t + 2.f * t * t * t;
}

// Ring radius over normalized life: fast start, eased expansion.
inline float ringRadius(float p, float maxRadius) {
    p = std::clamp(p, 0.f, 1.f);
    return 0.12f + (maxRadius - 0.12f) * (1.f - (1.f - p) * (1.f - p));
}

// Billboard basis from a look direction. right/up span the camera plane so
// quads face the viewer. Degenerate near straight up/down falls back to a
// yaw-stable basis (right stays horizontal-ish via the dominant axis).
inline void basisFromLook(Vec3 look, Vec3& right, Vec3& up) {
    const Vec3 worldUp{0.f, 1.f, 0.f};
    Vec3 r = cross(look, worldUp);
    if (len(r) < 1e-3f) r = cross(Vec3{1.f, 0.f, 0.f}, look);
    right = norm(r);
    up = norm(cross(right, look));
}

struct Config {
    int count = 24;            // particles per burst
    float size = 0.32f;        // sparkle long radius, blocks
    float speed = 4.2f;        // burst launch speed, blocks/s
    float life = 0.7f;         // average particle life, s
    float gravity = 5.5f;      // blocks/s^2
    float drag = 0.88f;        // horizontal damping per 1/60 s
    float reach = 2.8f;        // eye -> hit point distance, blocks
    int ringCount = 3;         // camera-facing impact rings
    float ringSize = 1.4f;     // max ring radius, blocks
    float ringLife = 0.55f;    // ring life, s
    float cycleSpeed = 0.9f;   // two-color shimmer cycles per second
    float ar = 1.f, ag = 1.f, ab = 1.f;    // color A (default white)
    float br = 1.f, bg = 0.15f, bb = 0.10f; // color B (default red)
};

struct Particle {
    Vec3 pos, vel;
    float age = 0.f, life = 1.f;
    float size = 0.2f, rot = 0.f, rotSpeed = 0.f;
    float phase = 0.f;
    bool live = false;
};

struct RingFx {
    Vec3 center, right, up;    // frozen spawn basis; ground ring uses XZ
    float age = 0.f, life = 0.5f, maxRadius = 1.f;
    float phase = 0.f;
    bool live = false;
    bool visible() const { return live && age >= 0.f; }
    float progress() const { return life > 0.f ? std::clamp(age / life, 0.f, 1.f) : 1.f; }
};

constexpr std::size_t kParticleCapacity = 128;
constexpr std::size_t kRingCapacity = 12;
constexpr int kStarVertices = 24;   // 4-pointed sparkle: 8 fan triangles
constexpr int kGlowVertices = 12;   // double-sided quad behind the sparkle
constexpr int kRingSteps = 40;
constexpr int kRingVertices = kRingSteps * 12;  // double-sided annulus

struct State {
    std::array<Particle, kParticleCapacity> parts;
    std::size_t cursor = 0;
    std::array<RingFx, kRingCapacity> rings;

    std::size_t liveParticles() const {
        std::size_t n = 0;
        for (const Particle& p : parts) n += p.live ? 1u : 0u;
        return n;
    }
    bool alive() const {
        if (liveParticles()) return true;
        for (const RingFx& r : rings)
            if (r.live) return true;
        return false;
    }
    void clear() { *this = State{}; }

    // Round-robin allocation: first free slot, else steal the cursor slot so
    // a burst can never fail, it only replaces the oldest-ish particle.
    Particle& acquire() {
        for (std::size_t k = 0; k < kParticleCapacity; ++k) {
            const std::size_t i = (cursor + k) % kParticleCapacity;
            if (!parts[i].live) { cursor = (i + 1) % kParticleCapacity; return parts[i]; }
        }
        const std::size_t i = cursor;
        cursor = (cursor + 1) % kParticleCapacity;
        return parts[i];
    }
    RingFx* acquireRing() {
        for (RingFx& r : rings)
            if (!r.live) return &r;
        return nullptr;  // too many rings in flight: drop extras, bounded draw
    }

    // One hit: sparkles burst from the impact point plus staggered
    // camera-facing rings there and one ground ring around the victim.
    void spawn(const Config& c, Vec3 center, Vec3 look, Rng& rng) {
        for (int i = 0; i < c.count; ++i) {
            Particle& p = acquire();
            const float theta = rng.range(0.f, 6.2831853f);
            const float phi = rng.range(0.12f, 3.0215927f);  // avoid poles
            const float sp = c.speed * rng.range(0.45f, 1.0f);
            p.live = true;
            p.age = 0.f;
            p.life = c.life * rng.range(0.7f, 1.3f);
            p.pos = center;
            p.vel = {std::sin(phi) * std::cos(theta) * sp,
                     std::fabs(std::cos(phi)) * sp + c.speed * 0.22f,
                     std::sin(phi) * std::sin(theta) * sp};
            p.size = c.size * rng.range(0.6f, 1.25f);
            p.rot = rng.range(0.f, 6.2831853f);
            p.rotSpeed = (rng.unit() < 0.5f ? -1.f : 1.f) * rng.range(0.8f, 6.0f);
            p.phase = rng.unit();
        }
        Vec3 camRight, camUp;
        basisFromLook(look, camRight, camUp);
        for (int i = 0; i < c.ringCount; ++i) {
            RingFx* r = acquireRing();
            if (!r) break;
            r->live = true;
            r->age = -0.06f * static_cast<float>(i);  // staggered start
            r->life = c.ringLife;
            r->maxRadius = c.ringSize * (0.8f + 0.2f * static_cast<float>(i));
            r->center = center;
            r->right = camRight;
            r->up = camUp;
            r->phase = rng.unit();
        }
        if (c.ringCount > 0) {
            RingFx* r = acquireRing();
            if (r) {
                r->live = true;
                r->age = -0.02f;
                r->life = c.ringLife * 1.25f;
                r->maxRadius = std::max(0.55f, c.ringSize * 0.62f);
                r->center = {center.x, center.y - 1.30f, center.z};  // victim feet
                r->right = {1.f, 0.f, 0.f};                            // ground plane
                r->up = {0.f, 0.f, 1.f};
                r->phase = rng.unit();
            }
        }
    }

    void update(float dt, const Config& c) {
        dt = std::clamp(dt, 0.f, 0.1f);
        if (dt <= 0.f) return;
        const float damp = std::pow(c.drag, dt * 60.f);
        for (Particle& p : parts) {
            if (!p.live) continue;
            p.age += dt;
            if (p.age >= p.life) { p.live = false; continue; }
            p.vel.y -= c.gravity * dt;
            p.vel.x *= damp;
            p.vel.z *= damp;
            p.pos.x += p.vel.x * dt;
            p.pos.y += p.vel.y * dt;
            p.pos.z += p.vel.z * dt;
            p.rot += p.rotSpeed * dt;
        }
        for (RingFx& r : rings) {
            if (!r.live) continue;
            r.age += dt;
            if (r.age >= r.life) r.live = false;
        }
    }
};

// ---- geometry emitters: pure, Emit is called as emit(x,y,z,r,g,b) with
// colors in 0..1. Positions are already relative to the render origin.

// 4-pointed sparkle: center vertex fanned to 8 rim vertices alternating the
// long and short radius, rotated by rot in the billboard plane.
template <class Emit>
void star(Vec3 p, Vec3 right, Vec3 up, float size, float rot,
          float r, float g, float b, Emit emit) {
    Vec3 rim[8];
    for (int i = 0; i < 8; ++i) {
        const float a = rot + 6.28318530718f * static_cast<float>(i) / 8.f;
        const float rad = (i & 1) ? size * 0.34f : size;
        const float ca = std::cos(a) * rad, sa = std::sin(a) * rad;
        rim[i] = {p.x + right.x * ca + up.x * sa,
                  p.y + right.y * ca + up.y * sa,
                  p.z + right.z * ca + up.z * sa};
    }
    for (int i = 0; i < 8; ++i) {
        const Vec3& a = rim[i];
        const Vec3& d = rim[(i + 1) & 7];
        emit(p.x, p.y, p.z, r, g, b);
        emit(a.x, a.y, a.z, r, g, b);
        emit(d.x, d.y, d.z, r, g, b);
    }
}

// Soft glow quad behind a sparkle: double-sided, dimmer color.
template <class Emit>
void glowQuad(Vec3 p, Vec3 right, Vec3 up, float size,
              float r, float g, float b, Emit emit) {
    const float s = size * 1.25f;
    const Vec3 v[4] = {
        {p.x - right.x * s - up.x * s, p.y - right.y * s - up.y * s, p.z - right.z * s - up.z * s},
        {p.x + right.x * s - up.x * s, p.y + right.y * s - up.y * s, p.z + right.z * s - up.z * s},
        {p.x + right.x * s + up.x * s, p.y + right.y * s + up.y * s, p.z + right.z * s + up.z * s},
        {p.x - right.x * s + up.x * s, p.y - right.y * s + up.y * s, p.z - right.z * s + up.z * s},
    };
    const int idx[12] = {0, 1, 2, 0, 2, 3, 0, 2, 1, 0, 3, 2};  // both windings
    for (int i = 0; i < 12; ++i) {
        const Vec3& q = v[idx[i]];
        emit(q.x, q.y, q.z, r, g, b);
    }
}

// Annulus (flat ring band) in the plane spanned by right/up around c.
// Double-sided triangles: the game's culling state is whatever it is.
template <class Emit>
void annulus(Vec3 c, Vec3 right, Vec3 up, float radius, float width,
             float r, float g, float b, Emit emit) {
    const float inner = std::max(0.f, radius - width);
    for (int i = 0; i < kRingSteps; ++i) {
        const float a0 = 6.28318530718f * static_cast<float>(i) / kRingSteps;
        const float a1 = 6.28318530718f * static_cast<float>(i + 1) / kRingSteps;
        const float c0 = std::cos(a0), s0 = std::sin(a0);
        const float c1 = std::cos(a1), s1 = std::sin(a1);
        const Vec3 o1{c.x + right.x * c0 * radius + up.x * s0 * radius,
                      c.y + right.y * c0 * radius + up.y * s0 * radius,
                      c.z + right.z * c0 * radius + up.z * s0 * radius};
        const Vec3 o2{c.x + right.x * c1 * radius + up.x * s1 * radius,
                      c.y + right.y * c1 * radius + up.y * s1 * radius,
                      c.z + right.z * c1 * radius + up.z * s1 * radius};
        const Vec3 i1{c.x + right.x * c0 * inner + up.x * s0 * inner,
                      c.y + right.y * c0 * inner + up.y * s0 * inner,
                      c.z + right.z * c0 * inner + up.z * s0 * inner};
        const Vec3 i2{c.x + right.x * c1 * inner + up.x * s1 * inner,
                      c.y + right.y * c1 * inner + up.y * s1 * inner,
                      c.z + right.z * c1 * inner + up.z * s1 * inner};
        emit(o1.x, o1.y, o1.z, r, g, b); emit(i1.x, i1.y, i1.z, r, g, b); emit(o2.x, o2.y, o2.z, r, g, b);
        emit(o2.x, o2.y, o2.z, r, g, b); emit(i1.x, i1.y, i1.z, r, g, b); emit(i2.x, i2.y, i2.z, r, g, b);
        emit(o2.x, o2.y, o2.z, r, g, b); emit(i1.x, i1.y, i1.z, r, g, b); emit(o1.x, o1.y, o1.z, r, g, b);
        emit(i2.x, i2.y, i2.z, r, g, b); emit(i1.x, i1.y, i1.z, r, g, b); emit(o2.x, o2.y, o2.z, r, g, b);
    }
}

}  // namespace hit_fx
