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
assert 'drawModuleHud(runtime,false)' in u and 'hudEditorActive.load' in u
assert 'drawModuleHud(runtime,true)' not in u  # old editor preview removed in v5
assert 'if (!menuOpen) { hudEditorOpen=false; visualEditor=VisualEditor::None; }' in u
assert 'if (!menuOpen || (!hudEditorOpen && !editingVisual)) {' in u
assert 'if (menuOpen && !replaced) { VisualStyleScope style; realDrawGui(runtime); }' in u
assert 'else if (!menuOpen) realDrawGui(runtime);' in u
# The embedded new menu replaces the legacy GUI only while it is alive; the
# old panels stay hidden then and reappear automatically if the embed fails.
assert 'menu_rml::SetGameMenuOpen(menuOpen)' in u and 'if (replaced) {' in u
assert 'menu_rml::RenderOverlay(device, overlayTarget, desc.Width, desc.Height)' in u
assert u.index('if (hudHookCreated) {') > u.index('mhApply())')
for name in ('HatOpacity','TrailOpacity'):
    assert 'VisualParam::'+name in n and 'VisualParam::'+name in u and name in settings
assert 'vertexGradient{false}' in n and 'Vertex gradient (test)' in u
assert 'visual_gradient::phase(' in n  # wireframe helix phase, unchanged
# v11: the trail vertices now mix the two gradient colors instead of the old
# experimental vertex-gradient helper.
assert 'visual_gradient::trail(' not in n
assert 'param(VisualParam::Trail2Red)' in n and 'param(VisualParam::Trail2Green)' in n
assert 'sampleOutputMerger(depthWorld)' in n and 'sampleOutputMerger(depthBefore)' in n and 'sampleOutputMerger(depthAfter)' in n
assert 'OMGetDepthStencilState' in n and 'OMGetBlendState' in n
assert 'OMSetDepthStencilState' not in n and 'OMSetBlendState' not in n
assert (r/'HUD_PREVIEW.png').read_bytes().startswith(b'\x89PNG\r\n\x1a\n')
assert (r/'HUD_ALL_IN_ONE.txt').exists()
print('HUD, gradient and read-only depth diagnostics: static checks OK; Windows test required')

for symbol in ('_ZN5ImGui6ButtonEPKcRK6ImVec2','_ZN5ImGui17GetWindowDrawListEv',
               '_ZN5ImGui18GetCursorScreenPosEv','_ZN5ImGui5DummyERK6ImVec2',
               '_ZN10ImDrawList7AddLineERK6ImVec2S2_jf','_ZN10ImDrawList17AddTriangleFilledERK6ImVec2S2_S2_j',
               '_ZN10ImDrawList13AddQuadFilledERK6ImVec2S2_S2_S2_j','_ZN10ImDrawList13AddRectFilledERK6ImVec2S2_jfi',
               '_ZN10ImDrawList23AddRectFilledMultiColorERK6ImVec2S2_jjjj'):
    assert symbol in u and symbol.encode() in exports, symbol
assert 'Waiting for player; sample avatar' in u and 'Sample model; game shader may differ' in u
assert 'visual_avatar_preview::hat(mesh,s,now)' in u and 'drawTrailPreview(d,p,now,skin)' in u
assert 'captureGamePreview' not in u and 'CopyResource(previewTexture' not in u
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
assert 'if (!inVisualDraw)' in gpu and 'if (inPlayerCapture)' in gpu and 'call(); // The normal world draw' in gpu
assert 'Old overlay (see-through)' in u and 'SetVisualLegacyMode(legacy)' in u

# The main HUD theme is installed after essential hooks, is limited to known
# core windows during actual HUD/notification drawing, and falls back safely.
theme=(r/'src/hud_theme.h').read_text()
assert 'insideLegacyHud || !hud_theme::card(name)' in u
assert 'hud_theme::background(widgetOpacity*legacyOpacity,hudPalette)' in u
assert 'insideLegacyHud=true; realDrawHud(runtime)' in u
assert 'realNotificationDraw(queue,s)' in u and 'insideLegacyHud=previous; legacyOpacity=oldOpacity' in u
assert 'mhRemove(reinterpret_cast<void*>(guiBegin))' in u
assert u.index('mhCreate(reinterpret_cast<void*>(guiBegin)') > u.index('mhApply())')
# v5: widget toggles moved to the RmlUi menu and reach SetModule through the
# queued drain; the module list itself is drawn by drawModuleHud.
assert 'for (const auto& entry:module_hud::entries)' not in u  # old editor column gone
assert 'toggleGameModule(int moduleId, bool enable)' in u
assert 'setModule(menuEmbedRuntime, static_cast<ag::ModuleId>(static_cast<std::size_t>(moduleId)), enable)' in u
assert 'module_hud::labels(s->enabled' in u
for name in ('fps','cps','clock','cpu','ram','resolution','watermark','info','active','hotkeys','coordinates','keystrokes'):
    assert '##hud_'+name in theme
for symbol in ('_ZN5ImGui5BeginEPKcPbi','_ZN5ImGui8SameLineEff','_ZN5ImGui12PushStyleVarEif','_ZN5ImGui11PopStyleVarEi'):
    assert symbol in u and symbol.encode() in exports
print('Scoped HUD style and module wiring: static OK; Windows test required')

# The game skin candidate is only observed during the local player's original
# renderer, uses no readback, and is held through the ImGui render submission.
skin=(r/'src/visual_player_skin.h').read_text()
for symbol in ('_ZN10ImDrawList8AddImageEyRK6ImVec2S2_S2_S2_j','_ZN10ImDrawList12AddImageQuadEyRK6ImVec2S2_S2_S2_S2_S2_S2_S2_j'):
    assert symbol in u and symbol.encode() in exports
assert 'visual_player_skin::acquire(frameDevice)' in u and 'previewFrameView=skin.view' in u
assert u.index('release(previewFrameView);',u.index('legacyPresent(chain'))>u.index('legacyPresent(chain')
assert 'SetPlayerSkinPreviewActive((editingVisual && visualEditor!=VisualEditor::Options && visualEditor!=VisualEditor::Jump && visualEditor!=VisualEditor::Accessory && visualEditor!=VisualEditor::Katana) || rmlPreview)' in u
assert 'menu_rml::VisualPreviewActive()' in u and 'drawRmlVisualPreview' in u
assert 'visual_player_skin::begin(local,captureContext)' in n and 'visual_gpu_depth::inPlayerCapture=capture' in n
assert n.index('visual_player_skin::begin(local,captureContext)')<n.index('originalPlayer(self,entity,pos,renderValue,partial)')
assert 'scopeGeneration==generation' in skin and 'ctx!=scopeContext' in skin
assert 'PSGetShaderResources(0,4,views)' in skin and 'count<8' in skin
assert 'CreateFile' not in skin and 'Map(' not in skin and 'CopyResource' not in skin
print('Read-only skin candidate preview: static OK; Windows test required')

replay=(r/'src/visual_player_replay.h').read_text()
assert 'static bool previewGameModelTest=false' in u
assert 'Game model replay (test)' in u and 'Trail preview not available yet.' in u
assert 'SetPlayerModelPreviewActive(editingVisual && visualEditor!=VisualEditor::Options && visualEditor!=VisualEditor::Jump && visualEditor!=VisualEditor::Accessory && visualEditor!=VisualEditor::Katana && previewGameModelTest)' in u
assert 'visual_player_replay::acquire(frameDevice)' in u and 'previewFrameView=model' in u
assert 'scope.draws>=128' in replay and 'srvBound(ctx,scope.srv)' in replay and 'outputBound(ctx,scope.rtv,scope.dsv)' in replay
for guard in ('SOGetTargets','GetPredication','OMGetRenderTargetsAndUnorderedAccessViews','d.StencilEnable','RSGetViewports'):
    assert guard in replay
assert 'ctx->OMSetRenderTargets(D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT,saved.rtv,saved.dsv)' in replay
assert 'ctx->RSSetViewports(1,&saved.viewport)' in replay and 'ctx->RSSetScissorRects(saved.scissorCount' in replay
assert 'CreateFile' not in replay and 'CopyResource' not in replay and 'Map(' not in replay
assert 'originalDraw(c,n,start); },true' in gpu and 'originalIndexed(c,n,start,base); },true' in gpu
assert 'originalInstanced(c,n,instances,start,first); },true' in gpu
assert 'originalIndirect(c,b,off); },true' not in gpu and 'originalAuto(c); },true' not in gpu
assert 'NestedScope' in n and 'visual_player_replay::end()' in n
print('Opt-in replay wiring: static OK; D3D11/Minecraft test required')

# No perspective interception: it hid the first-person hand and interfered
# with F5 in the reported game build. Keep camera input under game control.
assert 'perspectiveHook' not in n and 'originalPerspective' not in n
assert 'PlayerPreviewCameraStatus' not in u  # crown settings UI removed in v5
assert 'SetPlayerPreviewHatActive(editingVisual && visualEditor==VisualEditor::Hat && previewGameModelTest)' in u
assert 'inHatCapture && replayable' in gpu and 'submitted.fetch_add(1,std::memory_order_relaxed); call();' in gpu
assert 'struct EntityScope { ~EntityScope() { visual_player_replay::end();' in n
for palette in ('Aogiri Green','Emerald','Graphite'): assert palette in theme
print('Manual camera, hat replay and HUD palette wiring: static OK; Windows test required')
