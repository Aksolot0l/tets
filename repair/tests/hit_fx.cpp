// hit_fx.cpp - Linux unit tests for the pure hit fx math (hit_fx.h).
// No Windows, no D3D: curves, pool, burst spawn, physics and geometry
// emitters, all with seeded RNG so everything is reproducible.
#include "hit_fx.h"
#include <cmath>
#include <cstdio>
#include <cstdlib>

using namespace hit_fx;

static int failures = 0;
static void expect(bool ok, const char* what) {
    if (!ok) { std::printf("FAIL: %s\n", what); ++failures; }
    else std::printf("ok  : %s\n", what);
}
static bool finite(Vec3 v) { return std::isfinite(v.x) && std::isfinite(v.y) && std::isfinite(v.z); }

int main() {
    // ---- curves ----------------------------------------------------------
    expect(std::fabs(sizeCurve(0.f) - 0.55f) < 1e-4f, "sizeCurve starts at 0.55");
    expect(std::fabs(sizeCurve(1.f)) < 1e-5f, "sizeCurve ends at 0");
    {
        float peak = 0.f;
        for (int i = 0; i <= 200; ++i) peak = std::max(peak, sizeCurve(i / 200.f));
        expect(peak > 1.02f && peak < 1.16f, "sizeCurve overshoots above 1 then settles");
    }
    expect(std::fabs(alphaCurve(0.f) - 1.f) < 1e-5f, "alphaCurve starts at 1");
    expect(std::fabs(alphaCurve(1.f)) < 1e-5f, "alphaCurve ends at 0");
    {
        bool mono = true;
        for (int i = 1; i <= 200; ++i)
            if (alphaCurve(i / 200.f) > alphaCurve((i - 1) / 200.f) + 1e-6f) mono = false;
        expect(mono, "alphaCurve is monotonically decreasing");
    }
    expect(std::fabs(ringRadius(0.f, 1.4f) - 0.12f) < 1e-4f, "ringRadius starts at the inner hole");
    expect(std::fabs(ringRadius(1.f, 1.4f) - 1.4f) < 1e-5f, "ringRadius reaches the max");
    {
        bool mono = true;
        for (int i = 1; i <= 200; ++i)
            if (ringRadius(i / 200.f, 1.4f) < ringRadius((i - 1) / 200.f, 1.4f) - 1e-6f) mono = false;
        expect(mono, "ringRadius is monotonically increasing");
    }

    // ---- two-color shimmer -------------------------------------------------
    expect(std::fabs(colorMix(0.f, 1.f, 0.f)) < 1e-5f, "colorMix at phase 0 gives color A");
    expect(std::fabs(colorMix(0.5f, 1.f, 0.f) - 1.f) < 1e-4f, "colorMix at half a cycle gives color B");
    expect(std::fabs(colorMix(1.f, 1.f, 0.f)) < 1e-4f, "colorMix returns to A after a full cycle");
    expect(std::fabs(colorMix(0.25f, 1.f, 0.f) - 0.5f) < 1e-4f, "colorMix is halfway at a quarter cycle");

    // ---- billboard basis ----------------------------------------------------
    {
        Vec3 right, up;
        basisFromLook(norm(Vec3{0.3f, -0.2f, -0.9f}), right, up);
        expect(std::fabs(dot(right, up)) < 1e-4f, "billboard right and up are perpendicular");
        expect(std::fabs(len(right) - 1.f) < 1e-4f && std::fabs(len(up) - 1.f) < 1e-4f,
               "billboard basis vectors are unit length");
        // degenerate straight-up look must not explode
        basisFromLook(Vec3{0.f, 1.f, 0.f}, right, up);
        expect(finite(right) && finite(up) && std::fabs(len(right)) > 0.5f,
               "straight-up look still yields a finite basis");
    }

    // ---- burst spawn: determinism, counts, finiteness ------------------------
    Config c;
    c.count = 24; c.ringCount = 3;
    State a, b;
    Rng ra(1234), rb(1234);
    a.spawn(c, {1.f, 2.f, 3.f}, norm(Vec3{0.f, 0.f, -1.f}), ra);
    b.spawn(c, {1.f, 2.f, 3.f}, norm(Vec3{0.f, 0.f, -1.f}), rb);
    expect(a.liveParticles() == 24u, "burst spawns the configured particle count");
    {
        std::size_t rings = 0;
        for (const RingFx& r : a.rings) rings += r.live ? 1u : 0u;
        expect(rings == 4u, "burst spawns ringCount impact rings plus one ground ring");
    }
    {
        bool same = true, fin = true;
        for (std::size_t i = 0; i < kParticleCapacity; ++i) {
            if (a.parts[i].pos.x != b.parts[i].pos.x || a.parts[i].pos.y != b.parts[i].pos.y ||
                a.parts[i].pos.z != b.parts[i].pos.z) same = false;
            if (a.parts[i].live && !finite(a.parts[i].pos)) fin = false;
            if (a.parts[i].live && !finite(a.parts[i].vel)) fin = false;
        }
        expect(same, "same seed reproduces the same burst");
        expect(fin, "all spawned particles have finite positions and velocities");
    }
    {
        // the ground ring is horizontal, around the victim's feet
        const RingFx* ground = nullptr;
        for (const RingFx& r : a.rings)
            if (r.live && r.right.y == 0.f && r.up.y == 0.f && r.right.x == 1.f) ground = &r;
        expect(ground && std::fabs(ground->center.y - (2.f - 1.30f)) < 1e-4f,
               "the ground ring sits at the victim's feet height");
    }
    {
        // staggered ring starts: later rings wait with a negative age
        int waiting = 0;
        for (const RingFx& r : a.rings)
            if (r.live && r.age < 0.f) ++waiting;
        expect(waiting >= 2, "impact rings are staggered (negative start ages)");
    }

    // ---- physics -------------------------------------------------------------
    {
        State s;
        Rng rng(7);
        s.spawn(c, {0.f, 0.f, 0.f}, norm(Vec3{0.f, 0.f, -1.f}), rng);
        float vy0[kParticleCapacity], vx0[kParticleCapacity];
        for (std::size_t i = 0; i < kParticleCapacity; ++i) {
            vy0[i] = s.parts[i].live ? s.parts[i].vel.y : 0.f;
            vx0[i] = s.parts[i].live ? std::fabs(s.parts[i].vel.x) : 0.f;
        }
        const float dt = 1.f / 60.f;
        s.update(dt, c);
        bool gravity = true, drag = true, moved = false;
        for (std::size_t i = 0; i < kParticleCapacity; ++i) {
            const Particle& p = s.parts[i];
            if (!p.live) continue;
            // each particle's own vy drops by exactly gravity*dt
            if (p.vel.y > vy0[i] - c.gravity * dt + 1e-4f) gravity = false;
            if (std::fabs(p.vel.x) > vx0[i] + 1e-4f) drag = false;
            if (std::fabs(p.pos.x) + std::fabs(p.pos.y) + std::fabs(p.pos.z) > 1e-3f) moved = true;
        }
        expect(gravity, "gravity pulls each particle's own velocity down by g*dt");
        expect(drag, "horizontal drag never speeds a particle up");
        expect(moved, "particles actually leave the spawn point");
        // everything dies by the longest possible life
        const float maxLife = c.life * 1.3f + 0.05f;
        for (int step = 0; step < static_cast<int>(maxLife / dt) + 2; ++step) s.update(dt, c);
        expect(!s.alive(), "the burst fully expires within its maximum life");
    }
    {
        // a huge dt clamps instead of exploding into NaN
        State s;
        Rng rng(9);
        s.spawn(c, {0.f, 0.f, 0.f}, norm(Vec3{1.f, 0.f, 0.f}), rng);
        for (int step = 0; step < 40; ++step) s.update(1000.f, c);  // each clamps to 0.1s
        bool fin = true;
        for (const Particle& p : s.parts)
            if (p.live && !finite(p.pos)) fin = false;
        expect(fin && !s.alive(), "clamped huge dts kill the burst without NaNs");
    }

    // ---- pool bounds -----------------------------------------------------------
    {
        State s;
        Rng rng(11);
        for (int burst = 0; burst < 6; ++burst)
            s.spawn(c, {0.f, 0.f, 0.f}, norm(Vec3{0.f, 0.f, 1.f}), rng);
        expect(s.liveParticles() <= kParticleCapacity,
               "overflowing the pool never exceeds its capacity");
    }

    // ---- geometry emitters -------------------------------------------------------
    {
        int n = 0;
        Vec3 center{2.f, 3.f, 4.f};
        star(center, {1.f, 0.f, 0.f}, {0.f, 1.f, 0.f}, 0.5f, 0.7f, 1.f, 1.f, 1.f,
             [&](float, float, float, float, float, float) { ++n; });
        expect(n == kStarVertices, "star emits exactly kStarVertices vertices");
        n = 0;
        glowQuad(center, {1.f, 0.f, 0.f}, {0.f, 1.f, 0.f}, 0.5f, 1.f, 1.f, 1.f,
                 [&](float, float, float, float, float, float) { ++n; });
        expect(n == kGlowVertices, "glowQuad emits exactly kGlowVertices vertices");
        n = 0;
        annulus(center, {1.f, 0.f, 0.f}, {0.f, 0.f, 1.f}, 1.f, 0.1f, 1.f, 1.f, 1.f,
                [&](float, float, float, float, float, float) { ++n; });
        expect(n == kRingVertices, "annulus emits exactly kRingVertices vertices");
    }
    {
        // star rim vertices sit at the long/short radius from the center
        const Vec3 center{0.f, 0.f, 0.f};
        const Vec3 right{1.f, 0.f, 0.f}, up{0.f, 1.f, 0.f};
        bool radii = true;
        star(center, right, up, 0.5f, 0.f, 1.f, 1.f, 1.f,
             [&](float x, float y, float, float, float, float) {
                 const float d = std::sqrt(x * x + y * y);
                 if (std::fabs(x) > 1e-4f || std::fabs(y) > 1e-4f) {
                     const bool longR = std::fabs(d - 0.5f) < 1e-4f;
                     const bool shortR = std::fabs(d - 0.5f * 0.34f) < 1e-4f;
                     if (!longR && !shortR) radii = false;
                 }
             });
        expect(radii, "star rim vertices alternate the long and short radius");
        // glow quad corners at size*1.25*sqrt(2) from the center
        bool quad = true;
        glowQuad(center, right, up, 0.5f, 1.f, 1.f, 1.f,
                 [&](float x, float y, float, float, float, float) {
                     const float d = std::sqrt(x * x + y * y);
                     const float want = 0.5f * 1.25f * 1.41421356f;
                     if (std::fabs(d - want) > 1e-4f) quad = false;
                 });
        expect(quad, "glow quad corners are equidistant from the particle center");
        // annulus vertices at radius or radius-width in the ring plane
        bool band = true;
        annulus(center, right, {0.f, 0.f, 1.f}, 1.f, 0.1f, 1.f, 1.f, 1.f,
                 [&](float x, float, float z, float, float, float) {
                     const float d = std::sqrt(x * x + z * z);
                     if (std::fabs(d - 1.f) > 1e-4f && std::fabs(d - 0.9f) > 1e-4f) band = false;
                 });
        expect(band, "annulus vertices lie on the outer or inner circle");
    }

    if (failures) { std::printf("%d FAILURES\n", failures); return 1; }
    std::printf("ALL HIT FX CHECKS PASSED\n");
    return 0;
}
