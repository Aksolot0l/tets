"""Static artifact/ABI checks; does not replace Windows installer testing."""
import hashlib
import json
from pathlib import Path
import re
import subprocess

ROOT = Path(__file__).resolve().parents[2]
manifest = json.loads((ROOT / 'repair/install-manifest.json').read_text())
assert manifest['schema'] == 1
expected = {
    'repair/client.exe': 'client.exe',
    'repair/GAYClientLauncher.dll': 'GAYClientLauncher.dll',
    'GAYClientInternal.dll': 'GAYClientCore.dll',
    'repair/GAYClientInternal.dll': 'GAYClientInternal.dll',
    'repair/ENABLE_ACCESS.cmd': 'ENABLE_ACCESS.cmd',
    'repair/RESTORE_ACCESS.cmd': 'RESTORE_ACCESS.cmd',
}
files = manifest['files']
assert len(files) == len(expected)
assert {item['source']: item['destination'] for item in files} == expected
for item in files:
    data = (ROOT / item['source']).read_bytes()
    assert len(data) == item['size'], f"{item['source']}: size mismatch"
    assert hashlib.sha256(data).hexdigest() == item['sha256'], f"{item['source']}: hash mismatch"
    assert all(re.fullmatch(r'[0-9a-f]{64}', h) for h in item['previousSha256'])
    print(f"{item['source']} -> {item['destination']}: OK")

exports = subprocess.check_output(['objdump', '-p', str(ROOT / 'GAYClientInternal.dll')], text=True)
required = ['MH_Initialize', 'MH_CreateHook', 'MH_QueueEnableHook', 'MH_ApplyQueued',
            'MH_DisableHook', '_Z11PresentHookP14IDXGISwapChainjj',
            '_Z23ImGui_ImplDX11_NewFramev',
            '_Z19ImGui_ImplDX11_InitP12ID3D11DeviceP19ID3D11DeviceContext',
            '_ZN5ImGui5GetIOEv', '_ZN2ag10InputState6UpdateERKNS_8SettingsEbb',
            '_ZN2ag19InitializeUwpBridgeEv', '_ZN2ag17SetUwpInputActiveEb',
            '_ZN2ag10UwpPointerERfS0_S0_S0_', '_ZN2ag8SettingsC1Ev',
            '_ZN2ag19FindMinecraftWindowEb', '_ZN2ag15AddEmbeddedFontEfb',
            '_ZN7ImGuiIO18AddMouseWheelEventEff',
            '_ZN2ag17UwpSetWatchedKeysERKSt5arrayIbLy256EE', '_ZN2ag10UwpKeyDownEi']
for name in required:
    assert re.search(r'\]\s+' + re.escape(name) + r'\s*$', exports, re.M), f'missing export {name}'
print(f'{len(required)} expected legacy exports: OK')
launcher = subprocess.check_output(['objdump', '-p', str(ROOT / 'repair/client.exe')], text=True)
assert re.search(r'Subsystem\s+0*2\s+\(Windows GUI\)', launcher), 'client is not Windows GUI subsystem'
exports = subprocess.check_output(['objdump', '-p', str(ROOT / 'repair/GAYClientLauncher.dll')], text=True)
assert re.search(r'\]\s+GAYClientAutoloadImport\s*$', exports, re.M)
print('client GUI subsystem and autoload DLL export: OK')
header = (ROOT / 'repair/vendor/imgui/imgui.h').read_text()
assert '#define IMGUI_VERSION       "1.1' not in header  # avoid accidental old major versions
assert '#define IMGUI_VERSION       "1.91.9b"' in header
print('ImGui header version: OK')

# The UI-dispatcher callback is not exported; the adapter uses a verified RVA
# in this specific legacy build. Check both the whole file hash and prologue.
import struct
core = (ROOT / 'GAYClientInternal.dll').read_bytes()
pe = struct.unpack_from('<I', core, 0x3c)[0]
number = struct.unpack_from('<H', core, pe + 6)[0]
optional_size = struct.unpack_from('<H', core, pe + 20)[0]
table = pe + 24 + optional_size
tick_rva = 0x2060
for index in range(number):
    _, virtual_size, rva, raw_size, raw_offset, *_ = struct.unpack_from('<8sIIIIIIHHI', core, table + index * 40)
    if rva <= tick_rva < rva + min(virtual_size, raw_size):
        offset = raw_offset + tick_rva - rva
        break
else:
    raise AssertionError('input tick RVA is not in a file-backed PE section')
assert core[offset:offset + 12] == bytes.fromhex('41554154555756534883ec68')
print('legacy dispatcher callback RVA and signature: OK')
