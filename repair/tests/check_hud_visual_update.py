"""Static wiring checks; cannot replace Windows/game/graphics testing."""
from pathlib import Path
r=Path(__file__).resolve().parents[1]
u=(r/'src/compat.cpp').read_text(); n=(r/'src/china_hat_native.cpp').read_text()
settings=(r/'src/visual_settings.h').read_text(); exports=(r.parent/'GAYClientInternal.dll').read_bytes()
for symbol in ('_ZN2ag7DrawHudERNS_7RuntimeE','_ZN2ag17NotificationQueue4DrawERKNS_8SettingsE',
               '_ZN5ImGui14PushStyleColorEiRK6ImVec4','_ZN5ImGui13PopStyleColorEi',
               '_ZN5ImGui18SetWindowFontScaleEf'):
    assert symbol in u and symbol.encode() in exports, symbol
assert 'module_hud::editing(menu,hudEditorOpen)' in u
assert 'realDrawHud(runtime)' in u and 'drawModuleHud(runtime,false)' in u
assert 'drawModuleHud(runtime,true)' in u and 'hudEditorActive.load' in u
assert 'if (!menuOpen) hudEditorOpen=false;' in u
assert 'if (!menuOpen || !hudEditorOpen) realDrawGui(runtime);' in u
assert u.index('if (hudHookCreated) {') > u.index('mhApply())')
for name in ('HatOpacity','TrailOpacity'):
    assert 'VisualParam::'+name in n and 'VisualParam::'+name in u and name in settings
assert 'vertexGradient{false}' in n and 'Vertex gradient (test)' in u
assert 'visual_gradient::trail(' in n and 'visual_gradient::phase(' in n
assert 'sampleOutputMerger(depthBefore)' in n and 'sampleOutputMerger(depthAfter)' in n
assert 'OMGetDepthStencilState' in n and 'OMGetBlendState' in n
assert 'OMSetDepthStencilState' not in n and 'OMSetBlendState' not in n
assert (r/'HUD_PREVIEW.png').read_bytes().startswith(b'\x89PNG\r\n\x1a\n')
assert (r/'HUD_ALL_IN_ONE.txt').exists()
print('HUD, gradient and read-only depth diagnostics: static checks OK; Windows test required')
