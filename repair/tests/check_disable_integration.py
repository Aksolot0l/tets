"""Static checks complement state and PE tests; runtime behavior still needs Windows."""
from pathlib import Path
s = (Path(__file__).resolve().parents[1] / 'src/compat.cpp').read_text()
extra = (Path(__file__).resolve().parents[1] / 'src/uwp_extra.cpp').read_text()
bootstrap = s[s.index('static DWORD WINAPI bootstrap'):]
assert bootstrap.index('suppressMouseDiagnostic()') < bootstrap.index('mhQueueEnable(')
assert bootstrap.index('suppressMouseDiagnostic()') < bootstrap.index('bridgeInit()')
assert 'keys[VK_DELETE] = true;' in s
assert 'disableHotkey.update(down, down && gameIsForeground(), editing)' in s
assert 'getIO().WantTextInput' in s
assert 'overlayDisabled.load(std::memory_order_acquire)) { clearGamePreview(); return; }' in s
assert 'if (!chain || (flags & DXGI_PRESENT_TEST)) return;' in s
assert 'bridgeActive(false)' in s and 'DisableUwpExtraInput();' in s
assert 'if (!overlayDisabled.load(std::memory_order_acquire)) SampleUwpInput();' in s
assert 'if (!arg || !inputEnabled.load(std::memory_order_acquire)) return S_OK;' in extra
assert 'GET_MODULE_HANDLE_EX_FLAG_PIN' in bootstrap
print('Delete stop and patch wiring: OK; not a physical unload')
