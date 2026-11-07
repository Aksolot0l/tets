from pathlib import Path
r = Path(__file__).resolve().parents[1]
n = (r/'src/china_hat_native.cpp').read_text(encoding='utf-8')
u = (r/'src/compat.cpp').read_text(encoding='utf-8')
m = (r/'src/menu_rml.cpp').read_text(encoding='utf-8')
h = (r/'src/hit_fx.h').read_text(encoding='utf-8')
d = (r/'src/menu_rml_data.h').read_text(encoding='utf-8')
vs = (r/'src/visual_settings.h').read_text(encoding='utf-8')

# 1. The pure module stays pure: no Windows/D3D/game types, testable on Linux.
assert '#include <windows' not in h.lower() and 'd3d11' not in h.lower()
assert 'GetAsyncKeyState' not in h and 'tessVertex' not in h
for sym in ('struct Config', 'struct State', 'sizeCurve', 'alphaCurve', 'colorMix',
            'ringRadius', 'basisFromLook', 'glowQuad', 'annulus', 'struct Rng'):
    assert sym in h, sym
assert 'kParticleCapacity = 128' in h and 'kRingCapacity = 12' in h

# 2. Native wiring: the effect renders in both passes, right after the rings.
assert '#include "hit_fx.h"' in n
assert n.count('renderHitFx(local);') == 2
assert 'hitFxEnabled.load(std::memory_order_acquire) || hitFxEnabled' in n or \
       'katanaEnabled.load(std::memory_order_acquire) || hitFxEnabled.load(std::memory_order_acquire)) && !local.valid' in n
assert 'bool anyVisual()' in n and 'hitFxEnabled.load(std::memory_order_acquire); }' in n

# 3. The trigger: rising edge of the left button, suppressed while UI is open.
assert 'GetAsyncKeyState(VK_LBUTTON)&0x8000' in n
assert 'clicked=down&&!hitPrevDown' in n
assert 'hitFxUiActive.load(std::memory_order_acquire)' in n
assert 'now-hitLastBurst>=120' in n
# aimed along the verified look rotation at the configured reach
assert 'katana_rotation::readRotation' in n
assert 'eye.x+dir.x*config.reach' in n
# state stays on the render thread: dt from the wall clock, edge-based poll
assert 'float dt=hitLastTick?(now-hitLastTick)/1000.f:0.f' in n

# 4. Rendering follows the tessellator contract: origin-relative vertices,
# four opacity buckets (no per-vertex alpha in this pipeline), rings drawn
# as bright band + halo like the jump rings.
assert 'bucketOf[hit_fx::kParticleCapacity]' in n
assert 'tessBegin(t,2,verts)' in n and 'tessBegin(t,2,hit_fx::kRingVertices)' in n
assert 'hit_fx::glowQuad(rel,right,up,size' in n and 'hit_fx::star(rel,right,up,size' in n
assert 'hit_fx::annulus(rel,r.right,r.up,radius,width' in n
assert 'hit_fx::basisFromLook' in n

# 5. Public API + UI suppression flag.
assert 'bool HitFxEnabled();' in (r/'src/china_hat_native.h').read_text(encoding='utf-8')
assert 'void SetHitFxUiActive(bool active);' in (r/'src/china_hat_native.h').read_text(encoding='utf-8')
assert 'SetHitFxUiActive(menuOpen)' in u

# 6. Menu wiring: master switch + 16 sliders + two color chips.
assert '{"sw-gfx-hit", SwKind::HitFx, 0}' in m
assert 'case SwKind::HitFx: return HitFxEnabled();' in m
assert 'case SwKind::HitFx: SetHitFxEnabled(on); break;' in m
assert m.count('{"sl-hit-') == 16
assert '{"chip-hita", VisualParam::HitARed' in m and '{"chip-hitb", VisualParam::HitBRed' in m
assert 'id="sec-hit"' in d and '<span class="sec-title">УДАРЫ</span>' in d
assert d.count('id="sl-hit-') == 16

# 7. Parameters: 16 new ones with specs, appended after the katana block.
for name in ('HitCount, HitSize, HitSpeed, HitLife, HitGravity, HitReach,',
             'HitRingCount, HitRingSize, HitRingLife, HitCycleSpeed,',
             'HitARed, HitAGreen, HitABlue, HitBRed, HitBGreen, HitBBlue, Count'):
    assert name in vs, name

# 8. The pure module has a compiled unit test.
assert (r/'tests/hit_fx.cpp').exists()
for probe in ('sizeCurve starts', 'alphaCurve is monotonically',
              'same seed reproduces the same burst', 'staggered',
              'overflowing the pool', 'annulus emits exactly'):
    assert probe in (r/'tests/hit_fx.cpp').read_text(encoding='utf-8'), probe

print('Hit fx wiring OK; physics/geometry covered by tests/hit_fx.cpp (Linux run)')
