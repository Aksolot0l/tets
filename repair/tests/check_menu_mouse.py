"""Static checks only: Windows/Minecraft behavior still needs user testing."""
from pathlib import Path
r=Path(__file__).resolve().parents[1]
s=(r/'src/menu_mouse_native.cpp').read_text()
c=(r/'src/compat.cpp').read_text()
assert 'ChinaHatAvailable()' in s and 'std::memcmp(bytes,sig.bytes,sizeof(bytes))' in s
assert 'vt==image+mouseVtableRva' in s and 'kind==0' in s
assert 'reinterpret_cast<DeleteFn>(destroy)(event,1)' in s and '*slot=nullptr' in s
assert 'realQueue(reinterpret_cast<void*>(queue),&event)' in s
assert 'void releaseHeld(void* handler)' in s and 'up=0' in s
assert 'visual_hooks::install(hooks,2' in s and 'ready.store(true' in s
assert 'SetMenuMouseOpen(menuOpen)' in c and 'SetMenuMouseOpen(false)' in c
assert 'if (verified && coordHooks && mhEnable && mhRemove)' in c
assert 'InitializeMenuMouse(mhCreate,mhEnable,mhDisable,mhRemove)' in c
print('Menu mouse wiring: static checks OK; Windows test required')

import re
observed=(r/'research/china_hat/observed-input.txt').read_text()
labels=['mouse-queue','input-tick','mouse-factory','cursor-command-read','grab-method','release-method']
block=s[s.index('constexpr Signature signatures[]'):s.index('using QueueFn')]
entries=re.findall(r'\{(?:queueRva|tickRva|factoryRva|0x[0-9a-f]+),\{([^}]+)\}\}',block)
assert len(entries)==len(labels)
for label,entry in zip(labels,entries):
 expected=re.search(r'^'+label+r': ([0-9a-f]{32})$',observed,re.M).group(1)
 assert bytes(int(x,16) for x in re.findall(r'0x[0-9a-f]{2}',entry)).hex()==expected
print('Recorded input prologues: OK')
