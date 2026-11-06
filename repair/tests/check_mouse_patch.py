"""Validate exact patch target against the checked-in legacy PE, not just a fixture."""
import hashlib
import struct
from pathlib import Path
p = Path(__file__).resolve().parents[2] / 'GAYClientInternal.dll'
b = p.read_bytes()
assert hashlib.sha256(b).hexdigest() == 'f74a42c4aedd1c9d1f0c92200e9c7e977d800ab686d108114b2711c0c81835c5'
pe = struct.unpack_from('<I', b, 0x3c)[0]
nsects = struct.unpack_from('<H', b, pe+6)[0]
optsize = struct.unpack_from('<H', b, pe+20)[0]
secs = pe+24+optsize
def data(rva, length):
    for i in range(nsects):
        s = secs + i*40
        virtual, va, rawlen, raw = struct.unpack_from('<IIII', b, s+8)
        if va <= rva and rva+length <= va+rawlen:
            return b[raw+rva-va:raw+rva-va+length]
    raise AssertionError('unmapped RVA')
code = data(0x1836, 18)
assert code == bytes.fromhex('e8 f5 17 00 00 84 c0 0f 85 18 02 00 00 e8 b8 67 05 00')
# Old branch enters the mouse-test draw code; fall-through calls ImGui::Render.
assert 0x1843 + struct.unpack_from('<i', code, 9)[0] == 0x1a5b
assert 0x1848 + struct.unpack_from('<i', code, 14)[0] == 0x58000
assert data(0x1bb008, 50) == b'UWP mouse test: reg %u/3  events %u  poll %u/3 %s\x00'
assert data(0x1a5b, 4) != b'\0'*4
assert data(0x1bf0, 12) == bytes.fromhex('48 83 ec 28 44 89 c2 e8 84 f7 ff ff')
print('legacy PE mouse diagnostic target verified')
