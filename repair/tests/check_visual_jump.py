from pathlib import Path
r=Path(__file__).resolve().parents[1]
n=(r/'src/china_hat_native.cpp').read_text(); u=(r/'src/compat.cpp').read_text()
assert 'jumpEnabled{false}' in n
assert '|| jumpEnabled.load(std::memory_order_acquire)' in n
assert n.count('renderJump(local);')==2
assert 'visual_jump::ring(' in n and 'submitVisual(t,opacity)' in n
assert 'jumpGeneration.fetch_add' in n and 'jumps.clear()' in n
assert 'localPlayer(actor)' in n and 'actor!=player.entity' in n
assert 'Jump rings (test)' in u and 'drawJumpSettings()' in u and 'SetJumpEnabled(false)' in u
print('Jump ring wiring OK; requires Windows/Minecraft verification')
