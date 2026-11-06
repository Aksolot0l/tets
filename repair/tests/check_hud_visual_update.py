"""Static wiring checks; cannot replace Windows/game/graphics testing."""
from pathlib import Path
r=Path(__file__).resolve().parents[1]
u=(r/'src/compat.cpp').read_text(); n=(r/'src/china_hat_native.cpp').read_text()
settings=(r/'src/visual_settings.h').read_text(); exports=(r.parent/'GAYClientInternal.dll').read_bytes()
for symbol in ('_ZN2ag7DrawHudERNS_7RuntimeE','_ZN2ag17NotificationQueue4DrawERKNS_8SettingsE',
               '_ZN5ImGui14PushStyleColorEiRK6ImVec4','_ZN5ImGui13PopStyleColorEi',
               '_ZN5ImGui18SetWindowFontScaleEf'):
    assert symbol in u and symbol.encode() in exports, symbol
assert 'module_hud::editing(menu,hudEditorOpen || visualEditor!=VisualEditor::None)' in u
assert 'realDrawHud(runtime)' in u and 'drawModuleHud(runtime,false)' in u
assert 'drawModuleHud(runtime,true)' in u and 'hudEditorActive.load' in u
assert 'if (!menuOpen) { hudEditorOpen=false; visualEditor=VisualEditor::None; }' in u
assert 'if (!menuOpen || (!hudEditorOpen && !editingVisual)) realDrawGui(runtime);' in u
assert u.index('if (hudHookCreated) {') > u.index('mhApply())')
for name in ('HatOpacity','TrailOpacity'):
    assert 'VisualParam::'+name in n and 'VisualParam::'+name in u and name in settings
assert 'vertexGradient{false}' in n and 'Vertex gradient (test)' in u
assert 'visual_gradient::trail(' in n and 'visual_gradient::phase(' in n
assert 'sampleOutputMerger(depthWorld)' in n and 'sampleOutputMerger(depthBefore)' in n and 'sampleOutputMerger(depthAfter)' in n
assert 'OMGetDepthStencilState' in n and 'OMGetBlendState' in n
assert 'OMSetDepthStencilState' not in n and 'OMSetBlendState' not in n
assert (r/'HUD_PREVIEW.png').read_bytes().startswith(b'\x89PNG\r\n\x1a\n')
assert (r/'HUD_ALL_IN_ONE.txt').exists()
print('HUD, gradient and read-only depth diagnostics: static checks OK; Windows test required')

for symbol in ('_ZN5ImGui6ButtonEPKcRK6ImVec2','_ZN5ImGui5ImageEyRK6ImVec2S2_S2_RK6ImVec4S5_'):
    assert symbol in u and symbol.encode() in exports, symbol
assert 'captureGamePreview(device,context,back,desc)' in u
assert u.index('captureGamePreview(device,context,back,desc)') < u.index('legacyPresent(chain, 0, flags)')
assert 'CopyResource(previewTexture,back)' in u and 'ResolveSubresource(previewTexture,0,back,0,desc.Format)' in u
assert 'previewReady=false; release(previewView); release(previewTexture);' in u
assert 'Live game view (center)' in u and 'Animated illustration' not in u
assert 'Settings##open_hat' in u and 'Settings##open_trail' in u
assert 'drawVisualEditor(); return;' in u and 'drawEffectPreview(current)' in u
assert u.index('drawHatSettings();',u.index('static void drawVisualEditor')) < u.index('drawEffectPreview(current)',u.index('static void drawVisualEditor'))
assert 'realText(label,end)' in u and 'guiPushWidth(-1.f)' in u
assert 'worldEntitiesRva=0x5ae250' in n and 'entitiesDiagnosticHook' in n
assert 'visual_hooks::install(world,1' in n

gpu=(r/'src/visual_gpu_depth.h').read_text()
assert 'strictDepth{true}' in n and 'visual_depth_policy::overlayDraw' in n
assert 'visual_depth_policy::worldDraw' in n and 'visual_gpu_depth::matches(ctx)' in n
assert 'visual_gpu_depth::expectedDepth=dsv' in n and 'submitVisual(t,param(VisualParam::HatOpacity))' in n
assert 'D3D11_DEPTH_WRITE_MASK_ZERO' in gpu and 'D3D11_BLEND_BLEND_FACTOR' in gpu
assert 'current==expectedDepth' in gpu and 'ctx!=expectedContext' in gpu
assert 'ctx->OMSetDepthStencilState(oldDepth,stencil)' in gpu and 'ctx->OMSetBlendState(oldBlend,oldFactor,sampleMask)' in gpu
assert 'if (!inVisualDraw) { call(); return; }' in gpu
assert 'Old overlay (see-through)' in u and 'SetVisualLegacyMode(legacy)' in u
