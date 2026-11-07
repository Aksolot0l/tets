"""Static wiring checks for the RmlUi game menu; no Windows here."""
from pathlib import Path
r = Path(__file__).resolve().parents[1]
root = r.parent
u = (r/'src/compat.cpp').read_text(encoding='utf-8')
m = (r/'src/menu_rml.cpp').read_text(encoding='utf-8')
h = (r/'src/menu_rml.h').read_text(encoding='utf-8')
d = (r/'src/menu_rml_data.h').read_text(encoding='utf-8')
b = (r/'tools/build_adapter.py').read_text(encoding='utf-8')

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

# 2. Public contract: same surface as the old embed, except the render call
#    takes the back-buffer render target view instead of the device context.
for name in ('RenderOverlay', 'SetPointer', 'AddWheel', 'SetKeyQuery',
             'SetGameMenuOpen', 'SetModuleStateReader', 'DrainModuleToggles',
             'ReplacesLegacyMenu', 'ConsumeHudEditorRequest'):
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
assert 'kTestToolsRml' not in m and 'kTestToolsRcss' not in m

# 4. Render loop discipline: dimensions follow the viewport, the RTV is passed
#    to EndFrame, and the legacy interface is replaced only after a complete
#    first frame (automatic fallback if RmlUi ever fails).
assert 'SetDimensions' in m and 'SetViewport' in m
assert 'g_renderer->BeginFrame();' in m and 'g_rmlContext->Render();' in m
assert 'g_renderer->EndFrame(rtv);' in m
assert 'bool ReplacesLegacyMenu() { return g_renderOk.load' in m
assert 'g_initFailed = true' in m
assert 'if (!menuVisible) return;' in m

# 5. Input is fed from the existing hooks only: cursor DIPs are scaled to
#    pixels like the old embed, buttons/keys are forwarded on transitions only
#    (RmlUi processes every call as a fresh event), the wheel keeps the
#    one-unit-per-notch convention of the adapter.
assert 'static_cast<float>(width) / w' in m
assert 'if (down == g_mouseStates[button]) continue;' in m
assert 'ProcessMouseButtonDown(button, 0)' in m and 'ProcessMouseButtonUp(button, 0)' in m
assert 'ProcessMouseWheel(Rml::Vector2f(wheelX, wheelY), 0)' in m
assert 'KI_ESCAPE' in m

# 6. Verified setter surface only: the module toggles go through the queued
#    ag::Runtime::SetModule bridge, the effect toggles through the direct
#    setters, and every slider writes one VisualParam.
assert 'queueAgModule' in m and 'g_pendingToggles.size() < 32' in m
assert 'SetChinaHatEnabled(on)' in m and 'SetTrailEnabled(on)' in m
assert 'SetJumpEnabled(on)' in m and 'SetKatanaEnabled(on)' in m and 'SetAccessoryEnabled(on)' in m
assert 'SetHatFillEnabled(on)' in m and 'SetHatRainbowEnabled(on)' in m
assert 'SetTrailFillEnabled(on)' in m and 'SetTrailRainbowEnabled(on)' in m
assert 'SetJumpRainbowEnabled(on)' in m
assert 'SetVisualParam(m_def->param, paramFromDom(*m_def, dom));' in m
assert 'percentToRange(dom, 0.03f, 0.80f)' in m

# 7. The DOM registry covers every interactive element of both documents.
for switch_id in ('sw-fps', 'sw-cps', 'sw-clock', 'sw-keys', 'sw-watermark',
                  'sw-coords', 'sw-res', 'sw-perf', 'sw-modlist',
                  'sw-chinahat', 'sw-trail', 'sw-jump', 'sw-katana', 'sw-crown',
                  'sw-gfx-chinahat', 'sw-gfx-trail', 'sw-gfx-jump',
                  'sw-ch-fill', 'sw-ch-rainbow', 'sw-tr-fill', 'sw-tr-rainbow',
                  'sw-jr-rainbow', 'sw-kt-master', 'sw-cr-master'):
    assert f'"{switch_id}"' in m, switch_id
    assert f'id="{switch_id}"' in d, switch_id
for slider_id in ('sl-ch-rainbow', 'sl-ch-opacity', 'sl-ch-radius', 'sl-ch-height',
                  'sl-ch-offset', 'sl-ch-amp', 'sl-ch-floatspeed',
                  'sl-tr-rainbow', 'sl-tr-opacity', 'sl-tr-height', 'sl-tr-life',
                  'sl-jr-rainbow', 'sl-jr-opacity', 'sl-jr-radius', 'sl-jr-duration',
                  'sl-kt-scale', 'sl-kt-tilt', 'sl-kt-side', 'sl-kt-height',
                  'sl-kt-forward', 'sl-kt-opacity', 'sl-kt-fpside', 'sl-kt-fpforward',
                  'sl-cr-scale', 'sl-cr-offset', 'sl-cr-opacity'):
    assert f'"{slider_id}"' in m, slider_id
    assert f'id="{slider_id}"' in d, slider_id
# Every slider id in the markup is registered and has a readout counterpart.
slider_ids = set(re.findall(r'id="(sl-[a-z0-9-]+)"', d)) if (re := __import__('re')) else set()
for slider_id in slider_ids:
    assert f'"{slider_id}"' in m, slider_id
    readout = 'ro' + slider_id[2:]
    assert f'id="{readout}"' in d, readout
# Every switch id in the markup is registered.
for switch_id in set(re.findall(r'id="(sw-[a-z0-9-]+)"', d)):
    assert f'"{switch_id}"' in m, switch_id

# 8. Window manager: fold buttons, layout reset, close and HUD editor
#    buttons are wired; drag + focus go through the WindowListener.
assert 'btn-close' in m and 'btn-hudeditor' in m and 'btn-hudeditor2' not in m
assert 'activateTab' not in m and '"tab-modules"' not in m and '"panel-modules"' not in m
assert 'class WindowListener' in m and 'EventId::Dragstart' in m and 'EventId::Drag' in m
assert 'focusWin' in m and 'applyWin' in m and 'layoutWindows' in m
assert 'kWindows[]' in m and '"win-home"' in m and '"win-visuals"' in m
assert 'menu_layout::computeMenuWindows' in m and 'menu_layout::clampDraggedWindow' in m
assert 'id.compare(0, 5, "fold-")' in m and 'SetClass("folded"' in m
assert 'id == "btn-layout"' in m
assert 'SetClass("on"' in m and 'IsClassSet("on")' in m
assert 'SetClass("focus"' in m
assert 'g_hudEditorRequest.store(true, std::memory_order_relaxed);' in m
assert 'layoutWindows(static_cast<float>(width), static_cast<float>(height), true);' in m
assert 'layoutWindows(static_cast<float>(width), static_cast<float>(height), false);' in m
assert (r/'src/menu_layout.h').exists()
ml = (r/'src/menu_layout.h').read_text(encoding='utf-8')
assert 'rectsOverlap' in ml and 'kMinRowHeight' in ml

# 9. Katana diagnostics live at the bottom of the katana window; the text is
#    updated only when it actually changed.
assert 'katanaStatusText' in m and 'KatanaDebugSnapshot()' in m
assert 'g_ttStatusCache != status' not in m and 'if (status != g_ttStatusCache)' in m
assert 'tt-status' in d and 'tt-diag' in d and 'win-kt' in d
assert 'id="tt"' not in d

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

# 10b. Init bugs found on the first user run must stay fixed: the viewport is
# set before the first BeginFrame (the renderer defaults to 0x0 targets), and
# every early return in initBackend logs its reason to gayclient-menu.log
# through the custom SystemInterface (which also captures RmlUi's own
# messages, e.g. RCSS parse errors).
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

# 11. The markup keeps the launcher palette in windowed form: draggable
#     title bars, folding, scrolling bodies, dim backdrop, accent gradients.
assert '.win-head' in d and 'drag: drag;' in d
assert 'overflow-y: auto' in d and 'scrollbarvertical' in d
assert 'border-radius: 13px 13px 0 0' in d
assert 'id="doc-body"' in d and '#020805' in d
assert '.win.folded .win-body { display: none; }' in d
for win in ('win-home', 'win-visuals', 'win-ch', 'win-tr', 'win-jr', 'win-kt', 'win-cr'):
    assert f'id="{win}"' in d, win
assert '#6ef58c' in d and 'transition:' in d
assert 'decorator: linear-gradient(' in d
assert '#3ae882' not in d
assert 'font-size: 20px' in d
assert '1140px' not in d and '716px' not in d

# 12. RmlUi 6.x rendering pitfalls found via the software-rasterizer repro
#     must stay fixed:
#     (a) default display is inline in RmlUi, so every block container needs
#         an explicit display: block (or flex);
#     (b) rgba(r,g,b,0.97) parses alpha with atoi -> 0 => invisible; all
#         colors use #rrggbbaa hex instead;
#     (c) a column flex child whose natural height exceeds the container
#         collapses fixed siblings unless flex-basis: 0px is set;
#     (d) element ids must be unique (GetElementById returns the first).
import re as _re2
block_divs = ('.win-body', '.brand', '.brand-name', '.brand-sub', '.m-text',
              '.m-name', '.m-desc', '.note', '.foot', '#tt-status', '#tt-diag')
for sel in block_divs:
    pat = _re2.escape(sel) + r'\s*\{[^}]*display:\s*block'
    assert _re2.search(pat, d), sel
assert '.win-body {\n    flex-basis: 0px;' in d
assert '.win-head {\n    height: 44px;\n    flex-basis: 44px;' in d
assert 'rgba(' not in d
import re as _re
assert len(_re.findall(r'id="doc-body"', d)) == 1
assert len(_re.findall(r'class="win"', d)) == 7
assert _re.findall(r'id="fold-\d"', d) == ['id="fold-0"', 'id="fold-1"', 'id="fold-2"',
                                            'id="fold-3"', 'id="fold-4"', 'id="fold-5"',
                                            'id="fold-6"']

# 13. Web bridge: the web replica (repair/menu-web, root index.html) drives
#     the same verified setters through a localhost-only WebSocket server.
#     Commands are applied on the Present thread; bridge failure is non-fatal;
#     the socket thread never touches the RmlUi DOM.
assert 'namespace bridge {' in m
assert '127.0.0.1' in m and '39731' in m
assert 'Sec-WebSocket-Accept' in m and '258EAFA5-E914-47DA-95CA-C5AB0DC85B11' in m
assert 'bridge::start();' in m and 'bridge::tick();' in m
assert m.index('bridge::start();') > m.index('g_ready = true;')
assert m.index('bridge::tick();') < m.index('if (!menuVisible) return;')
assert 'queueCommand(' in m and 'pushOutbox(' in m
assert '"-lws2_32"' in b

print('check_menu_rml: ok')
