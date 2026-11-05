"""Static artifact and legacy-ABI checks; does not replace Windows testing."""
import hashlib
from pathlib import Path
import re
import subprocess

ROOT = Path(__file__).resolve().parents[2]
installer = (ROOT / 'INSTALL_FROM_GITHUB.ps1').read_text()
entries = re.findall(r"Source = '([^']+)'; Destination = '([^']+)'; Size = (\d+); SHA256 = '([0-9a-f]{64})'", installer)
assert len(entries) == 5, f'expected five installer entries, found {len(entries)}'
assert len({x[1] for x in entries}) == len(entries), 'duplicate destination'
for source, dest, size, expected in entries:
    data = (ROOT / source).read_bytes()
    assert len(data) == int(size), f'{source} size mismatch'
    assert hashlib.sha256(data).hexdigest() == expected, f'{source} hash mismatch'
    print(f'{source} -> {dest}: OK')

exports = subprocess.check_output(['objdump', '-p', str(ROOT / 'GAYClientInternal.dll')], text=True)
required = ['MH_Initialize', 'MH_CreateHook', 'MH_QueueEnableHook', 'MH_ApplyQueued',
            'MH_DisableHook', '_Z11PresentHookP14IDXGISwapChainjj',
            '_Z23ImGui_ImplDX11_NewFramev',
            '_Z19ImGui_ImplDX11_InitP12ID3D11DeviceP19ID3D11DeviceContext',
            '_ZN5ImGui5GetIOEv', '_ZN2ag10InputState6UpdateERKNS_8SettingsEbb',
            '_ZN2ag19InitializeUwpBridgeEv', '_ZN2ag17SetUwpInputActiveEb',
            '_ZN2ag10UwpPointerERfS0_S0_S0_', '_ZN2ag8SettingsC1Ev',
            '_ZN2ag19FindMinecraftWindowEb', '_ZN2ag15AddEmbeddedFontEfb']
for name in required:
    assert re.search(r'\]\s+' + re.escape(name) + r'\s*$', exports, re.M), f'missing export {name}'
print(f'{len(required)} expected legacy exports: OK')
header = (ROOT / 'repair/vendor/imgui/imgui.h').read_text()
assert '#define IMGUI_VERSION       "1.91.9b"' in header
print('ImGui header version: OK')
