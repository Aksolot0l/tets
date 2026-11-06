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
assert 'GetTickCount64()-player.at>250' in n
assert 'available{false}, enabled{false}' in n
assert 'if (!ChinaHatAvailable()) return;' in n and 'if (installHooks())' in n
assert n.index('installHooks()') < n.index('bool InitializeChinaHat(')
assert 'China Hat (test)' in u and 'SetChinaHatEnabled(false)' in u
assert u.index('InitializeChinaHat(mhCreate') > u.index('mhApply())')
assert 'GetModuleFileNameW' in n and 'Minecraft.Windows.exe' in n
assert '0x28,0x83,0xb7,0x2a' in v
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
