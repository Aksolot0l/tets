"""Integration checks only; actual native renderer requires Windows/game testing."""
from pathlib import Path
r=Path(__file__).resolve().parents[1]
n=(r/'src/china_hat_native.cpp').read_text()
u=(r/'src/compat.cpp').read_text()
v=(r/'src/china_hat_validation.h').read_text()
assert 'hat_validation::identity(digest' in n and 'hashFile(bytes,size,digest)' in n
assert 'std::memcmp(inMemory,fromFile,16)!=0' in n
assert 'std::memcmp(fromFile,sig.bytes,16)!=0' in n
assert 'state[0x125] || state[0x170] || pending' in n
assert 'oldFlag' in n and 'saved,sizeof(saved)' in n
assert 'actual!=actor' in n and 'type!=30' in n
assert 'hat_player::fromClient' in n and 'clientTickRva=0x11b2d0' in n
assert 'for (auto rva : {screenRva,endRva,clientTickRva})' in n
assert 'visual_hooks::install(hooks,3' in n and '&clientTickHook' in n
local=n[n.index('bool localPlayer('):n.index('bool writable(')]
assert '0x1921df8' not in local and 'fromClient' in local
player=(r/'src/china_hat_player.h').read_text()
assert 'playerSlot=0x60' in player and 'again!=first' in player and 'maxTickAge=2000' in player
assert 'GetTickCount64()-player.at>250' in n and 'now-player.at>250' in n
assert 'available{false}, enabled{false}' in n
assert 'if (!ChinaHatAvailable()) return false;' in n and 'return installHooks();' in n
assert n.index('installHooks()') < n.index('bool InitializeChinaHat(')
assert 'China Hat (test)' in u and 'SetChinaHatEnabled(false)' in u
assert 'Trail (test)' in u and 'SetTrailEnabled(false)' in u
assert 'Hat height' in u and 'Hat: submitted' in u and 'Trail: submitted' in u
assert 'ObserveVisualContext(context)' in u
assert 'OMSetDepthStencilState' not in n  # scoped guard owns and restores GPU state
assert 'trailGeneration' in n and 'trail.clear()' in n
assert 'tessBegin(t,4,segments*2)' in n
hat=n[n.index('void renderHat(const PlayerFrame& player) {'):n.index('void renderTrail(const PlayerFrame& player) {')]
assert 'visual_color::animated(materialTint.load' in hat and 'hatRainbow.load' in hat
assert 'Enable colors' in u and 'materialTint{true}' in n
assert 'Filled hat (test)' in u and 'Filled trail (test)' in u
assert 'hatFill{true}, trailFill{true}' in n
assert 'guiBegin("Visuals"' in u and 'Settings##open_hat' in u and 'Settings##open_trail' in u
assert 'Rainbow hat' in u and 'Rainbow trail' in u
assert 'hatRainbow{true}, trailRainbow{true}' in n
assert 'std::atomic<bool> available{false}, enabled{false}' in n and 'trailEnabled{false}' in n
assert '_ZN5ImGui11BeginTabBarEPKci' in u and '_ZN5ImGui12BeginTabItemEPKcPbi' in u
for symbol in ('_ZN5ImGui11BeginTabBarEPKci','_ZN5ImGui9EndTabBarEv',
               '_ZN5ImGui12BeginTabItemEPKcPbi','_ZN5ImGui10EndTabItemEv',
               '_ZN5ImGui10ColorEdit3EPKcPfi','_ZN5ImGui17SetNextWindowSizeERK6ImVec2i'):
    assert symbol in u and symbol.encode() in (r.parent/'GAYClientInternal.dll').read_bytes(), symbol
assert 'drawVisualEditor(); return;' in u and 'drawEffectPreview(current)' in u
trail=n[n.index('void renderTrail(const PlayerFrame& player) {'):n.index('void screenHook(')]
assert 'visual_color::animated(materialTint.load' in trail and 'trailRainbow.load' in trail
assert 'saved,sizeof(saved)' in hat and 'saved,sizeof(saved)' in trail
assert 'constexpr int segments=48, spokes=12' in hat
assert 'tessBegin(t,4,2*(segments+spokes))' in hat and 'china_hat::emitWireframe' in hat
assert hat.index('submitVisual(') < hat.index('tessEnd(')
assert 'hat_animation::bob(' in hat
assert 'if (hatFill.load(std::memory_order_acquire))' in hat and 'if (!tessIdle(tess))' in hat
assert 'if (trailFill.load(std::memory_order_acquire))' in trail and 'if (!tessIdle(tess))' in trail
assert hat.index('tessBegin(t,4,') < hat.index('tessBegin(t,2,')
assert trail.index('tessBegin(t,4,') < trail.index('tessBegin(t,2,')
assert 'param(VisualParam::TrailHeight)' in trail and 'param(VisualParam::TrailSeconds)' in trail
assert 'visual_ribbon::segment' in trail and 'segments*12' in trail
assert hat.index('surfaceScratch') < hat.rindex('saved,sizeof(saved)')
assert trail.index('ribbonScratch') < trail.rindex('saved,sizeof(saved)')
screen=n[n.index('void screenHook('):n.index('bool ChinaHatAvailable()')]
assert screen.index('renderHat(visible)') < screen.index('renderTrail(local)')
assert u.index('InitializeChinaHat(mhCreate') > u.index('mhApply())')
assert 'GetModuleFileNameW' in n and 'Minecraft.Windows.exe' in n
assert '0x28,0x83,0xb7,0x2a' in v
for field in ('HatRadius','HatConeHeight','HatOffset','HatBobAmplitude','HatBobSpeed','HatRainbowSpeed',
              'TrailHeight','TrailSeconds','TrailRainbowSpeed','HatRed','HatGreen','HatBlue','TrailRed','TrailGreen','TrailBlue'):
    assert 'VisualParam::'+field in n and 'VisualParam::'+field in u, field
print('Experimental China Hat gating, input and render wiring: static OK (Windows game test required)')

# Recorded diagnostic bytes are checked independently of the C++ initializer.
import re
observed=(r/'research/china_hat/observed-build.txt').read_text()
labels=['player-render','entity-render','render-entities','render-level','tess-begin','tess-color','tess-vertex','tess-draw']
entries=re.findall(r'\{0x[0-9a-f]+, \{([^}]+)\}\}',v)
assert len(entries)==len(labels)
for label,entry in zip(labels,entries):
    recorded=re.search(r'^'+re.escape(label)+r': ([0-9a-f]{32})$',observed,re.M).group(1)
    encoded=bytes(int(x,16) for x in re.findall(r'0x[0-9a-f]{2}',entry)).hex()
    assert encoded==recorded, label
print('Recorded prologues: OK')

# Optional earlier player-pass experiment: no depth bypass, no second hat in
# the normal world path; original trail and legacy overlay remain independent.
assert 'hatPlayerPass{false}' in n
assert 'if (inEntityPass) return originalEntities(self,partial);' in n
assert 'isLocal && enabled.load(std::memory_order_acquire)' in n
assert 'playerPassAttempted=true' in n and 'drawHatAtPlayer(frame)' in n
assert 'if (visual_depth_policy::entitiesHat(entityPlayerPass)) {' in n and 'renderHat(visible);' in n
alt=n[n.index('void drawHatAtPlayer(const PlayerFrame& player) {'):n.index('// Activate D3D guards')]
assert 'visual_depth_policy::worldDraw(' in alt and 'visual_gpu_depth::matches(ctx)' in alt
assert 'visual_gpu_depth::expectedDepth=dsv' in alt and 'visual_gpu_depth::expectedDepth=nullptr' in alt
assert 'renderHat(player)' in alt and 'renderTrail(' not in alt
assert 'Hat at player pass (test)' in u and 'Player compare:' in u and 'World compare:' in u
print('Alternative player timing, mutual exclusion and depth gate: static OK; Windows test required')

# On this build's observed dispatcher the fourth argument is in XMM3 (float).
# Do not let the observer clobber it before forwarding to the game renderer.
assert 'using PlayerRender = std::intptr_t (*)(void*,void*,void*,float,float);' in n
assert 'playerHook(void* self,void* entity,void* pos,float renderValue,float partial)' in n
assert 'originalPlayer(self,entity,pos,renderValue,partial)' in n
print('Player renderer float argument forwarding: static OK; Windows test required')
