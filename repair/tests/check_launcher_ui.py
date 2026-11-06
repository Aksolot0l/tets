"""Static integration checks for the new launcher; Windows runtime is still required."""
from pathlib import Path
import hashlib
import re
import subprocess

root = Path(__file__).resolve().parents[2]
ui = (root / 'repair/src/launcher_ui.cpp').read_text(encoding='utf-8')
loader = (root / 'repair/src/client_autoload.cpp').read_text(encoding='utf-8')
exe = (root / 'repair/client.exe').read_bytes()
assert hashlib.sha256(exe).hexdigest() == '1230ca6952be96a418f684b99aace91bcb298c39329964b095191e2b820b3d57'
assert 'launcher-classic.flag' in loader and 'launcherStartUi()' in loader
assert loader.index('launcher-classic.flag') < loader.index('launcherStartUi()')
for status in ('Waiting', 'GameDetected', 'Loading', 'Loaded', 'Failed', 'MultipleGames'):
    assert 'LauncherStatus::' + status in loader
    assert 'LauncherStatus::' + status in ui or status == 'Waiting'
assert 'shell:AppsFolder' in ui and 'validAppId(appId)' in ui
assert 'detectMinecraftAppId' in ui and 'loadLauncherProfiles' in ui
assert 'Старый интерфейс' not in ui
assert 'client-autoload.log' in ui
assert 'WS_TABSTOP' in ui and 'IsDialogMessageW' in ui
assert 'Aogiri Green Launcher' in ui and 'SW_HIDE' in ui
assert 'PostMessageW(classicWindow,WM_CLOSE' in ui
assert 'CreateProcess' not in ui and 'WinExec' not in ui
dll = (root / 'repair/GAYClientLauncher.dll').read_bytes()
art = (root / 'repair/assets/launcher-world.jpg').read_bytes()
assert art[:3] == b'\xff\xd8\xff' and art in dll, 'hero image must be embedded in the launcher DLL'
assert 'FindResourceW' in ui and 'GdiplusStartup' in ui
binary = subprocess.check_output(['objdump', '-p', str(root / 'repair/GAYClientLauncher.dll')], text=True)
for symbol in ('ShellExecuteW', 'CoInitializeEx', 'CreateWindowExW', 'SetWindowsHookExW', 'GAYClientAutoloadImport', 'GdiplusStartup', 'FindResourceW', 'GetApplicationUserModelId'):
    assert re.search(r'\b' + symbol + r'\b', binary), symbol
print('Launcher GUI, classic fallback, preserved EXE and wheel hook: static checks OK (not a Windows runtime test)')
