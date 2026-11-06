"""Check exact-version PE patch and all legacy import descriptors, without Windows."""
import importlib.util
from pathlib import Path
import struct
import subprocess

root = Path(__file__).resolve().parents[2]
spec = importlib.util.spec_from_file_location('patch_client', root / 'repair/src/patch_client.py')
mod = importlib.util.module_from_spec(spec)
spec.loader.exec_module(mod)
original = (root / 'client.exe').read_bytes()
patched = (root / 'repair/client.exe').read_bytes()
assert mod.patch(original) == patched
try:
    mod.patch(original[:-1] + bytes([original[-1] ^ 1]))
except ValueError:
    pass
else:
    raise AssertionError('unknown original was accepted')
pe, = struct.unpack_from('<I', original, 0x3c)
opt = pe + 24
n, = struct.unpack_from('<H', original, pe + 6)
opt_size, = struct.unpack_from('<H', original, pe + 20)
table = opt + opt_size
assert struct.unpack_from('<H', patched, pe + 6)[0] == n + 1
# All original section definitions and bytes remain intact. Only selected PE
# header fields and one appended section header should differ.
assert original[table:table+n*40] == patched[table:table+n*40]
headers_size, = struct.unpack_from('<I', original, opt + 60)
assert original[headers_size:] == patched[headers_size:len(original)]
name, vsize, rva, raw_size, raw, *_ = struct.unpack_from('<8sIIIIIIHHI', patched, table+n*40)
assert name.rstrip(b'\0') == b'.agimp'
assert raw == len(original) and raw_size >= vsize
import_rva, import_size = struct.unpack_from('<II', patched, opt + 120)
assert import_rva == rva
old_imp, _ = struct.unpack_from('<II', original, opt + 120)
sections = [struct.unpack_from('<IIII', original, table+i*40+8) for i in range(n)]
def to_offset(target):
    for size, va, rsize, roff in sections:
        if va <= target < va + min(size, rsize):
            return roff + target - va
    raise AssertionError('unmapped RVA')
oldoff = to_offset(old_imp)
old = []
while original[oldoff:oldoff+20] != bytes(20):
    old.append(original[oldoff:oldoff+20]); oldoff += 20
assert patched[raw:raw+20*len(old)] == b''.join(old)
iltrva, ts, chain, dllrva, iatrva = struct.unpack_from('<IIIII', patched, raw+20*len(old))
assert ts == chain == 0
assert patched[raw+20*(len(old)+1):raw+20*(len(old)+2)] == bytes(20)
assert patched[raw+dllrva-rva:raw+dllrva-rva+len(mod.DLL_NAME)] == mod.DLL_NAME
ilt, ilt_end = struct.unpack_from('<QQ', patched, raw+iltrva-rva)
iat, iat_end = struct.unpack_from('<QQ', patched, raw+iatrva-rva)
assert ilt == iat and ilt_end == iat_end == 0
assert patched[raw+ilt-rva:raw+ilt-rva+2] == bytes(2)
assert patched[raw+ilt-rva+2:raw+ilt-rva+2+len(mod.IMPORT_NAME)] == mod.IMPORT_NAME
assert import_size == (len(old)+2)*20
print(f'PE patch: original sections, {len(old)} imports, new DLL and thunk verified')
# pefile performs an independent cross-section RVA parse when available.
try:
    import pefile
except ImportError:
    print('Optional pefile parser unavailable; skipped independent PE parse')
else:
    before, after = pefile.PE(data=original), pefile.PE(data=patched)
    def imports(pe):
        return [(e.dll, [(v.name, v.ordinal, v.address) for v in e.imports]) for e in pe.DIRECTORY_ENTRY_IMPORT]
    assert imports(before) == imports(after)[:-1]
    assert imports(after)[-1][0] == mod.DLL_NAME[:-1]
    assert imports(after)[-1][1][0][0] == mod.IMPORT_NAME[:-1]
    assert not after.get_warnings(), after.get_warnings()
    print('Independent pefile parse: all original imports and IAT addresses preserved')
