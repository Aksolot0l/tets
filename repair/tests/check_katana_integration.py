from pathlib import Path
import importlib.util, re
r=Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location('convert_katana',r/'tools/convert_katana.py')
c=importlib.util.module_from_spec(spec);spec.loader.exec_module(c)
asset=(r/'src/katana_asset.h').read_text()
embedded=bytes(int(x,16) for x in re.findall(r'0x([0-9a-f]{2})',asset))
assert embedded==c.convert((r/'assets/models/cyberpunk-katana.glb').read_bytes())
n=(r/'src/china_hat_native.cpp').read_text(); g=(r/'src/visual_gpu_depth.h').read_text(); ui=(r/'src/compat.cpp').read_text()
assert 'katanaEnabled{false}' in n and 'returnAddress!=base+0x5a1d2d' in n and 'returnAddress!=base+0x59fd1e' in n
assert 'local!=reinterpret_cast<std::uintptr_t>(actor)' in n and 'diamond_sword_candidate::matches' in n
assert 'validateGame(true)' in n and 'katanaCodeMatches()' in n and 'visual_hooks::install(hooks,3' in n
assert 'hookState.load(std::memory_order_acquire)!=2' in n
assert n.count('hat_validation::mayBePatched(')==2
for message in ('game hooks unavailable','GPU hooks unavailable','game check failed','item code changed','model invalid','hook install failed'): assert message in ui
assert 'if (!submitted || !katanaEnabled.load' in n and 'return originalHeldDraw(renderer,context,mode)' in n
assert 'katanaObserve=ctx' in n and 'katanaSuppress=ctx' in n and 'katanaSuppress=nullptr' in n
assert 'if (ctx && ctx==katanaSuppress) return;' in g
assert 'Diamond sword: katana (test)' in ui
assert 'SetKatanaEnabled(false);' in ui
for message in ('local player not matched','sword not matched','sword candidate','geometry prepared','stream unavailable'): assert message in ui
print('Embedded model and first-person opt-in wiring OK; no Windows/game validation')

assert 'katana_stream_policy::eligible' in n and 'prepareKatanaStream()' in n
assert 'return originalHeldGeometry(self,a2,a3,stack,a5)' in n
assert 'streaming!=1 || !tessIdle(t)' in n

assert 'katanaEvents.fetch_or' in n and 'KatanaEvidence()' in ui
assert 'Seen F:%u V:%u S1:%u S3:%u' in ui
