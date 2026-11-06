"""Static integration checks; actual process/window behavior needs Windows testing."""
from pathlib import Path
root = Path(__file__).resolve().parents[2]
loader = (root / 'repair/src/client_autoload.cpp').read_text(encoding='utf-8')
ui = (root / 'repair/src/launcher_ui.cpp').read_text(encoding='utf-8')
header = (root / 'repair/src/game_lifecycle.h').read_text(encoding='utf-8')
assert 'SYNCHRONIZE|PROCESS_QUERY_LIMITED_INFORMATION' in loader
assert 'QueryFullProcessImageNameW' in loader
assert 'WaitForSingleObject(watchedGame,0)' in loader
assert 'gameExitAction(snapshotOk,multiple,pid,watchedPid,wait==WAIT_OBJECT_0)' in loader
assert 'fallback.update(snapshotOk,multiple,pid)' in loader
assert 'launcherRequestShutdown();' in loader
assert 'if (!snapshotOk) continue;' in loader
assert 'showErrorAsync(L"Найдено несколько' in loader
assert 'if (!shuttingDown) CloseHandle(guard)' in loader
assert 'ERROR_ALREADY_EXISTS' in loader and 'Do not leave a second, unmonitored client process' in loader
assert 'emptySnapshots >= 3' in header
assert 'scheduleShutdownWatchdog();' in ui
assert 'Sleep(3000)' in ui
assert 'TerminateProcess(GetCurrentProcess(), 0)' in ui
assert 'TerminateProcess(GetCurrentProcess(),0)' in ui
assert ui.count('TerminateProcess(') == 2, 'termination must be restricted to the launcher itself'
assert 'PostMessageW(classicWindow,WM_CLOSE' in ui
print('Game-exit monitor and launcher-only shutdown wiring: static checks OK (Windows test required)')
