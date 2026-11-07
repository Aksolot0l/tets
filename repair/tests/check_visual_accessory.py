from pathlib import Path
r=Path(__file__).resolve().parents[1]
n=(r/'src/china_hat_native.cpp').read_text(); u=(r/'src/compat.cpp').read_text(); mesh=(r/'src/visual_accessory.h').read_text()
assert 'accessoryEnabled{false}' in n and '|| accessoryEnabled.load(std::memory_order_acquire)' in n
assert n.count('renderAccessory(local);')==2
assert 'actor!=player.entity' in n[n.index('void renderAccessory(const PlayerFrame& player) {'):]
assert 'tessIdle(tess)' in n and 'visual_accessory::vertexCount' in n
assert 'submitVisual(t,opacity)' in n and 'SetAccessoryEnabled(false)' in u
# v5: the crown is gone from every interface; while the RmlUi menu owns the
# screen the adapter keeps it disabled. The render hook itself stays dormant.
assert '3D crown (test)' not in u and 'drawAccessorySettings' not in u
assert 'if (AccessoryEnabled()) SetAccessoryEnabled(false);' in u
assert 'CreateFile' not in mesh and 'ReadProcessMemory' not in mesh
print('3D crown render hook intact, UI removed (v5); Windows test required')
