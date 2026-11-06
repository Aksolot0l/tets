"""Static checks; a native Windows visual test is still required."""
from pathlib import Path
r = Path(__file__).resolve().parents[1]
u = (r/'src/launcher_ui.cpp').read_text()
w = (r/'src/launcher_webview.cpp').read_text()
create = u[u.index('case WM_CREATE:'):u.index('case WM_TIMER:')]
assert 'WS_VISIBLE' not in create
assert 'startLauncherWebView(window,webDispatch,webReady,webFailure)' in create
assert 'startupView.value()!=LauncherView::Fallback' in u
assert 'startupView.value()==LauncherView::Loading' in u
assert 'Загружаем лаунчер...' in u
assert 'ShowWindow(homeNav,SW_SHOW); ShowWindow(profileNav,SW_SHOW);' in u
assert 'switchPage(false)' in u[u.index('void webFailure()'):u.index('LRESULT CALLBACK windowProc')]
assert 'updateClassic(); // Hide an already-created original window' in u
assert 'now-lastClassic>=125' in u
assert 'ready=true; if(onReady) onReady()' in w
assert 'if (onFailure) onFailure()' in w and '30000' in w
print('Loading screen, WebView readiness and fallback wiring: OK (Windows test required)')
