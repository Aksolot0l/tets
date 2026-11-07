"""Static wiring checks for the v6 RmlUi game menu; no Windows here."""
from pathlib import Path
r = Path(__file__).resolve().parents[1]
root = r.parent
u = (r/'src/compat.cpp').read_text(encoding='utf-8')
m = (r/'src/menu_rml.cpp').read_text(encoding='utf-8')
h = (r/'src/menu_rml.h').read_text(encoding='utf-8')
d = (r/'src/menu_rml_data.h').read_text(encoding='utf-8')
b = (r/'tools/build_adapter.py').read_text(encoding='utf-8')
import re

# 1. The old ImGui embed is gone everywhere: no menu_embed, no GAYClientMenu,
#    no vendored ImGui sources in the internal DLL build.
assert 'menu_embed' not in u and 'menu_embed' not in m
assert '#include "menu_rml.h"' in u and 'menu_rml::SetGameMenuOpen(menuOpen)' in u
assert 'menu_rml::RenderOverlay(device, overlayTarget, desc.Width, desc.Height)' in u
assert 'menu_rml::AddWheel(wheelX / 120.0f, wheelY / 120.0f)' in u
assert 'menu_rml::SetPointer(x, y, w, h)' in u
assert 'menu_rml::SetKeyQuery(keyDown)' in u
assert 'menu_rml::SetModuleStateReader(&readGameModule)' in u
assert 'menu_rml::DrainModuleToggles(&toggleGameModule)' in u
assert 'GAYClientMenu' not in u and 'GAYClientMenu' not in b
assert 'imgui_impl_dx11.cpp' not in b and 'vendor/imgui/imgui.cpp' not in b
assert not (r/'src/menu_embed.cpp').exists() and not (r/'src/menu_embed.h').exists()
assert not (r/'tests/check_menu_embed.py').exists()

# 2. Public contract: same surface as v5 plus IsHiddenByClose, which the
#    compat layer uses to clear the client's menu flag when the user closes
#    the interface with the X button.
for name in ('RenderOverlay', 'SetPointer', 'AddWheel', 'SetKeyQuery',
             'SetGameMenuOpen', 'SetModuleStateReader', 'DrainModuleToggles',
             'ReplacesLegacyMenu', 'ConsumeHudEditorRequest',
             'VisualPreviewActive', 'ReadPointer', 'IsHiddenByClose'):
    assert name in h and name in m
assert 'ID3D11RenderTargetView* rtv' in h and 'ID3D11DeviceContext' not in h
assert 'void RenderOverlay(ID3D11Device* device, ID3D11RenderTargetView* rtv,' in m

# 3. RmlUi bootstrap order: renderer installed before Initialise, fonts loaded
#    from memory buffers that outlive everything, contexts created from the
#    markup with the RCSS injected into the empty <style> element.
assert 'Rml::SetRenderInterface(g_renderer);' in m
assert m.index('Rml::SetRenderInterface(g_renderer);') < m.index('Rml::Initialise()')
assert 'CreateContext("gayclient"' in m
assert 'segoeui.ttf' in m and 'segoeuib.ttf' in m
assert 'LoadFontFace' in m and 'Rml::Style::FontWeight::Bold' in m
assert '"GAY"' in m
assert 'LoadDocumentFromMemory' in m and 'text.find("<style>")' in m
assert 'kMenuRml' in m and 'kMenuRcss' in m

# 4. Render loop discipline: dimensions follow the viewport, the RTV is passed
#    to EndFrame, and the legacy interface is replaced only after a complete
#    first frame (automatic fallback if RmlUi ever fails).
assert 'SetDimensions' in m and 'SetViewport' in m
assert 'g_renderer->BeginFrame();' in m and 'g_rmlContext->Render();' in m
assert 'g_renderer->EndFrame(rtv);' in m
assert 'bool ReplacesLegacyMenu() { return g_renderOk.load' in m
assert 'g_initFailed = true' in m
assert 'if (!menuVisible && g_fadeOutFrames == 0) return;' in m

# 5. Input: cursor DIPs scaled to pixels, buttons/keys on transitions only,
#    and the wheel sign is INVERTED for RmlUi (UWP positive = wheel up, RmlUi
#    positive = scroll toward the bottom).
assert 'static_cast<float>(width) / w' in m
assert 'if (down == g_mouseStates[button]) continue;' in m
assert 'ProcessMouseButtonDown(button, 0)' in m and 'ProcessMouseButtonUp(button, 0)' in m
assert 'ProcessMouseWheel(Rml::Vector2f(-wheelX, -wheelY), 0)' in m
assert 'KI_ESCAPE' in m

# 6. Verified setter surface only: the module toggles go through the queued
#    ag::Runtime::SetModule bridge, the effect toggles through the direct
#    setters, and every slider writes one VisualParam. The crown is gone:
#    no Accessory switch from the menu.
assert 'queueAgModule' in m and 'g_pendingToggles.size() < 32' in m
assert 'SetChinaHatEnabled(on)' in m and 'SetTrailEnabled(on)' in m
assert 'SetJumpEnabled(on)' in m and 'SetKatanaEnabled(on)' in m
assert 'SetAccessoryEnabled' not in m
assert 'SetHatFillEnabled(on)' in m and 'SetHatRainbowEnabled(on)' in m
assert 'SetTrailFillEnabled(on)' in m and 'SetTrailRainbowEnabled(on)' in m
assert 'SetJumpRainbowEnabled(on)' in m
assert 'SetVisualParam(m_def->param, paramFromDom(*m_def, dom));' in m
assert 'percentToRange(dom, 0.03f, 0.80f)' in m

# 7. The DOM registry covers every interactive element.
for switch_id in ('sw-fps', 'sw-cps', 'sw-clock', 'sw-keys', 'sw-watermark',
                  'sw-coords', 'sw-res', 'sw-perf', 'sw-modlist', 'sw-preview',
                  'sw-gfx-chinahat', 'sw-gfx-trail', 'sw-gfx-jump',
                  'sw-ch-fill', 'sw-ch-rainbow', 'sw-tr-fill', 'sw-tr-rainbow',
                  'sw-jr-rainbow', 'sw-kt-master'):
    assert f'"{switch_id}"' in m, switch_id
    assert f'id="{switch_id}"' in d, switch_id
for slider_id in ('sl-ch-rainbow', 'sl-ch-opacity', 'sl-ch-radius', 'sl-ch-height',
                  'sl-ch-offset', 'sl-ch-amp', 'sl-ch-floatspeed',
                  'sl-tr-rainbow', 'sl-tr-opacity', 'sl-tr-height', 'sl-tr-life',
                  'sl-jr-rainbow', 'sl-jr-opacity', 'sl-jr-radius', 'sl-jr-duration',
                  'sl-kt-scale', 'sl-kt-tilt', 'sl-kt-side', 'sl-kt-height',
                  'sl-kt-forward', 'sl-kt-opacity', 'sl-kt-fpside', 'sl-kt-fpforward'):
    assert f'"{slider_id}"' in m, slider_id
    assert f'id="{slider_id}"' in d, slider_id
# Crown ids are gone from both sides.
for gone in ('sw-crown', 'sw-cr-master', 'sl-cr-scale', 'sl-cr-offset',
             'sl-cr-opacity', 'sl-cr-r', 'sl-cr-g', 'sl-cr-b', 'win-cr',
             'win-ch', 'win-tr', 'win-jr', 'win-kt',
             # v6: the duplicate master rows at the top of Visuals are gone;
             # only the section toggles remain.
             'sw-chinahat', 'sw-trail', 'sw-jump', 'sw-katana'):
    assert f'id="{gone}"' not in d, gone
    assert f'"{gone}"' not in m, gone
# Every slider id in the markup is registered and has a readout counterpart.
slider_ids = set(re.findall(r'id="(sl-[a-z0-9-]+)"', d))
for slider_id in slider_ids:
    assert f'"{slider_id}"' in m, slider_id
    readout = 'ro' + slider_id[2:]
    assert f'id="{readout}"' in d, readout
# Every switch id in the markup is referenced from the C++ side.
for switch_id in set(re.findall(r'id="(sw-[a-z0-9-]+)"', d)):
    assert f'"{switch_id}"' in m, switch_id

# 8. Window manager: exactly two windows, fold buttons, sections, preview
#    toggle, layout reset, close and HUD editor buttons are wired.
assert 'btn-close' in m and 'btn-hudeditor' in m and 'btn-hudeditor2' not in m
assert 'class WindowListener' in m and 'EventId::Dragstart' in m and 'EventId::Drag' in m
assert 'focusWin' in m and 'applyWin' in m and 'layoutWindows' in m
kwindows = re.search(r'const WinDef kWindows\[\] = \{(.*?)\};', m, re.S).group(1)
assert re.findall(r'\{"(win-[a-z]+)"', kwindows) == ['win-home', 'win-visuals']
assert 'menu_layout::computeMenuWindows' in m and 'menu_layout::clampDraggedWindow' in m
assert 'id.compare(0, 5, "fold-")' in m and 'SetClass("folded"' in m
assert 'id == "btn-layout"' in m
assert 'id == "sw-preview"' in m and 'g_previewOn' in m
assert 'id.compare(0, 4, "sec-")' in m
assert 'SetClass("on"' in m and 'IsClassSet("on")' in m
assert 'SetClass("focus"' in m
# v6: window fold animates through a C++ height interpolation.
assert 'startWinAnim' in m and 'stepWindowAnims' in m and 'currentWinH' in m
assert 'kFoldAnimMs' in m and 'SetClass("folding"' in m
assert 'startWinAnim(win, fromH);' in m  # start height captured before the flip
assert 'g_hudEditorRequest.store(true, std::memory_order_relaxed);' in m
# v6: document fade through the body.faded class; the doc keeps rendering a
# few frames after closing so the fade-out is visible.
assert 'SetClass("faded"' in m and 'kFadeOutFrames' in m
assert 'body.faded { opacity: 0; }' in d
# The icon animation driver is gone (icons were removed entirely).
assert 'driveIconAnimation' not in m and 'kAnimPeriod' not in m
assert 'SetClass("anim"' not in m
assert 'layoutWindows(static_cast<float>(width), static_cast<float>(height), true);' in m
assert 'layoutWindows(static_cast<float>(width), static_cast<float>(height), false);' in m
ml = (r/'src/menu_layout.h').read_text(encoding='utf-8')
assert 'rectsOverlap' in ml and 'kMinRowHeight' in ml
assert 'float right_inset = 0.0f' in ml

# 8b. UI zoom: every px in the RCSS and the window constants carry the scale.
assert 'computeUiScale' in m and 'scaleRcssPx' in m
assert 'g_uiScale = computeUiScale(width, height);' in m
# v6: compact menu, half of the v5 zoom (0.575 floor, 1.0 cap).
assert 'ref * 1.25f * 0.5f' in m
assert 'if (scale < 0.575f) return 0.575f;' in m
assert 'if (scale > 1.0f) return 1.0f;' in m
assert '460.0f, 1650.0f' in m  # visuals natural height without master rows
assert 'const std::string scaledRcss = scaleRcssPx(kMenuRcss, g_uiScale);' in m
assert 'kFoldedHeightRef * g_uiScale' in m
assert 'kWindows[i].w * g_uiScale' in m
# The 3D preview strip is reserved in screen pixels (ImGui side is not zoomed).
assert 'kPreviewInsetPx' in m and 'viewW > kPreviewInsetPx * 2.0f' in m

# 9. Katana diagnostics live at the bottom of the katana section; the text is
#    updated only when it actually changed.
assert 'katanaStatusText' in m and 'KatanaDebugSnapshot()' in m
assert 'if (status != g_ttStatusCache)' in m
assert 'tt-status' in d and 'tt-diag' in d and 'sec-kt' in d

# 10. Build script discipline: static RmlUi without export leaks, the DX11
#     renderer with non-multisampled targets, the FreeType font engine
#     actually compiled in, the minimal FreeType module set.
assert '"-DRMLUI_STATIC_LIB"' in b
assert 'RMLUI_CORE_EXPORTS' not in b.replace('without -DRMLUI_CORE_EXPORTS', '')
assert '"-DRMLUI_FONT_ENGINE_FREETYPE"' in b
assert '"-DNUM_MSAA_SAMPLES=0"' in b
assert 'ftmodule.h' in b and 'ftgzip.c' in b and 'ftmm.c' in b
assert 'menu_rml.cpp' in b and 'menu_embed.cpp' not in b
assert 'GAYClientInternal.dll' in b

# 10b. Init bugs found on the first user run must stay fixed.
assert 'g_renderer->SetViewport(static_cast<int>(width), static_cast<int>(height));' in m
assert m.index('g_renderer->SetViewport') < m.index('Rml::Initialise()')
assert 'Rml::SetSystemInterface(&g_systemInterface);' in m
assert m.index('Rml::SetSystemInterface(&g_systemInterface);') < m.index('Rml::Initialise()')
assert 'bool LogMessage(Rml::Log::Type type, const Rml::String& message) override' in m
assert 'gayclient-menu.log' in m
for step in ('segoeui.ttf unreadable', 'Rml::Initialise returned false',
             'CreateContext returned null', 'LoadFontFace (regular)',
             'menu document did not parse',
             'element registry incomplete', 'init: complete'):
    assert step in m, step
assert 'first frame complete' in m

# 11. The markup keeps the launcher palette in windowed form. v6: switches
#     are square checkboxes (empty when off, check mark when on), the icon
#     chips and section count captions are gone, and typography is plain
#     ASCII (no middle dots, no typographic dashes; U+25BE renders as tofu
#     in Segoe UI).
assert '.win-head' in d and 'drag: drag;' in d
assert 'overflow-y: auto' in d and 'scrollbarvertical' in d
assert 'border-radius: 13px 13px 0 0' in d
assert 'id="doc-body"' in d and '#020805' in d
assert '.win.folded .win-body { display: none; }' in d
assert '.win.folding .win-body {' in d  # body stays visible while folding
for win in ('win-home', 'win-visuals'):
    assert f'id="{win}"' in d, win
assert '#6ef58c' in d and 'transition:' in d
assert 'decorator: linear-gradient(' in d
assert 'font-size: 20px' in d
# checkboxes: square box, check mark drawn from two borders, class-driven
assert '.check {' in d and 'width: 26px' in d and 'height: 26px' in d
assert '.check .ck {' in d and 'border-left: 3px #05230f' in d
assert 'border-bottom: 3px #05230f' in d and 'rotate(-45deg)' in d
assert '.check.on {' in d and '.check.on .ck { opacity: 1; }' in d
assert d.count('class="check"') == 19
assert d.count('<span class="ck"/>') == 19
# no icon machinery, no section count captions
for gone in ('.m-ico', '.ip ', 'class="ip', '.sec-count', 'body.anim'):
    assert gone not in d, gone
# fold glyphs: plain ASCII '+' and '-'
assert '\u25be' not in d and '\u25b2' not in d and '\u25b6' not in d
assert 'class="f-plus">+' in d
assert 'class="f-minus">-<' in d
assert '.f-plus, .f-minus { display: none; }' in d
# collapsible sections animate through max-height + padding transitions
assert '.sec.folded .sec-body {' in d and '.sec-head' in d
assert 'max-height: 560px' in d and 'max-height: 0px' in d
assert 'transition: max-height 0.16s cubic-out, opacity 0.12s cubic-out, padding 0.16s cubic-out;' in d
assert '.sec.folded .sec-body' in d and 'padding-bottom: 0px' in d
for sec in ('sec-ch', 'sec-tr', 'sec-jr', 'sec-kt'):
    assert f'id="{sec}"' in d, sec
# typography: no middle dots, no typographic dashes anywhere in the UI text
for bad in ('\u00b7', '\u2014', '\u2013'):
    assert bad not in d, bad
    assert bad not in u, bad
assert 'выход - Esc' in d and 'HUD EDIT' not in d

# 12. RmlUi 6.x rendering pitfalls must stay fixed:
#     (a) default display is inline: every block container needs display block;
#     (b) rgba() parses alpha with atoi -> 0: all colors are #rrggbbaa hex;
#     (c) flex-basis: 0px on the scrolling body;
#     (d) unique ids.
block_divs = ('.win-body', '.brand', '.brand-name', '.brand-sub', '.m-text',
              '.m-name', '.m-desc', '.note', '.foot', '#tt-status', '#tt-diag',
              '.sec-body')
for sel in block_divs:
    pat = re.escape(sel) + r'\s*\{[^}]*display:\s*block'
    assert re.search(pat, d), sel
assert '.win-body {\n    flex-basis: 0px;' in d
assert '.win-head {\n    height: 44px;\n    flex-basis: 44px;' in d
assert 'rgba(' not in d
assert len(re.findall(r'id="doc-body"', d)) == 1
assert len(re.findall(r'class="win"', d)) == 2
assert re.findall(r'id="fold-\d"', d) == ['id="fold-0"', 'id="fold-1"']

# 13. The web bridge is gone completely: no WebSocket server, no ws2_32, no
#     web-menu file pair, no manifest entry.
assert 'namespace bridge' not in m and '39731' not in m and 'Sec-WebSocket' not in m
assert 'ws2_32' not in b and 'winsock' not in m.lower()
assert not (root/'index.html').exists() and not (r/'menu-web').exists()
manifest = (r/'install-manifest.json').read_text(encoding='utf-8')
assert 'web-menu' not in manifest and 'index.html' not in manifest

# 14. compat.cpp v5: the HUD editor shows only the HUD, and its widgets are
#     dragged through the already-installed ImGui::Begin hook.
assert 'hudEditBegin' in u and 'kHudDragNames' in u and 'updateHudEditInput' in u
assert 'menu_rml::ReadPointer(px,py,pw,ph)' in u
assert '"_ZN5ImGui12GetWindowPosEv"' in u and '"_ZN5ImGui13GetWindowSizeEv"' in u
assert 'guiSetNextPos(ImVec2(e.x,e.y),ImGuiCond_Always,ImVec2(0,0))' in u  # user position overrides the pin
assert 'drawHudDragOutline' in u and 'drawHudEditCaption' in u
# patchedDrawHud draws the HUD while the editor is open (no editing skip).
assert 'if (!editing) {' not in u.split('static void patchedDrawHud')[1].split('static void patchedNotificationDraw')[0]
# the old five-column editor and its entry button are gone
for gone in ('drawHudEditor', 'editorColumn', 'editorToolbar', 'editorHome',
             'editorHud', 'editorModules', 'editorAppearance', 'editorSettings',
             'editorPreviewCaption', 'drawHudEntry', 'editorVisible', 'editorSelected'):
    assert gone not in u, gone
# HUD editor mode leaves nothing but the HUD on screen
assert 'if (menuOpen && hudEditorOpen) return;' in u
# the 3D preview window for the RmlUi Visuals tab
assert 'drawRmlVisualPreview' in u and 'menu_rml::VisualPreviewActive()' in u
assert '##rml_visual_preview' in u
assert '|| rmlPreview' in u  # skin capture also runs for the RmlUi preview
# the crown stays off while the new menu owns the screen
assert 'if (AccessoryEnabled()) SetAccessoryEnabled(false);' in u
assert 'drawAccessorySettings' not in u and '3D crown' not in u
# v6: closing the interface releases the client's menu flag. The X button
# (menu hidden by close) and Esc both clear the byte at runtime+0x551, so
# the game never stays in a dead menu mode (hotbar/start buttons vanished).
assert 'menu_rml::IsHiddenByClose() && !hudEditorOpen' in u
assert 'keyDown(VK_ESCAPE)' in u and 'GetAsyncKeyState(VK_ESCAPE)' in u
assert 'escEdge' in u
assert '*(reinterpret_cast<unsigned char*>(runtime)+0x551)=0;' in u
# plain ASCII punctuation in the compat captions too
assert 'перетаскивай элементы мышкой - Esc - выход' in u

print('check_menu_rml: ok')
