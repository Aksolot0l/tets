"""Static wiring checks for the embedded new GAYClient menu; no Windows here."""
from pathlib import Path
import re
r=Path(__file__).resolve().parents[1]
root=r.parent
u=(r/'src/compat.cpp').read_text()
m=(r/'src/menu_embed.cpp').read_text()
h=(r/'src/menu_embed.h').read_text()
menu=(root/'GAYClientMenu.h').read_text()
menucpp=(root/'GAYClientMenu.cpp').read_text()

# The user-provided menu sources stay the integration contract: only the
# ModuleId enum gained the adapter test modules, everything else is wired
# through MenuState/MenuActions callbacks.
assert '#include "GAYClientMenu.h"' in m and '#include "menu_embed.h"' in u
assert 'Katana,' in menu and 'Crown' in menu
assert menu.count('ModuleId::') >= 1 and 'RenderMenu(MenuState& state, const MenuActions& actions)' in menu
# The menu itself never touches D3D or creates contexts.
assert 'CreateContext' not in menucpp and 'ID3D11' not in menucpp

# Private ImGui context: created once, no ini writes, cyrillic font through
# the menu's own loader, and the DX11 backend initialized from the same
# device/context the legacy overlay already uses.
assert 'ImGui::CreateContext()' in m and 'io.IniFilename = nullptr' in m
assert 'LoadCyrillicFont(io, 17.0f)' in m
assert 'ImGui_ImplDX11_Init(device, context)' in m
assert 'g_context = nullptr' in m and 'g_initFailed = true' in m
assert 'ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData())' in m
# The backend's NewFrame hook must run before ImGui::NewFrame: it creates the
# shaders, states and font texture, and RenderDrawData maps them without null
# checks (a missing call crashed the game at startup).
assert m.index('ImGui_ImplDX11_NewFrame();') < m.index('ImGui::NewFrame();')
# Draw only once the backend really owns its device objects, and replace the
# legacy interface only after the first successful frame.
assert 'ImGui::GetIO().Fonts->TexID != 0' in m
assert 'bool ReplacesLegacyMenu() { return g_renderOk.load' in m

# The legacy client's ImGui context is never touched: the embed only uses its
# own context and its own compiled backend.
assert 'SetCurrentContext' in m and 'getIO' not in m

# Input comes from the existing hooks only; no new game hooks.
assert 'menu_embed::SetPointer(x, y, w, h)' in u
assert 'menu_embed::AddWheel(wheelX / 120.0f, wheelY / 120.0f)' in u
assert 'menu_embed::SetKeyQuery(keyDown)' in u
assert 'GetAsyncKeyState(vk) & 0x8000' in m  # mouse buttons fallback

# Game-module toggles are queued and drained on the legacy GUI thread where
# setModule is already used; the runtime pointer comes from patchedDrawGui.
assert 'queueAgModule' in m and 'DrainModuleToggles' in h and 'void DrainModuleToggles' in m
assert 'menu_embed::DrainModuleToggles(&toggleGameModule)' in u
assert 'setModule(menuEmbedRuntime' in u and 'static_cast<ag::ModuleId>(static_cast<std::size_t>(moduleId))' in u

# Present path: the overlay renders after the legacy, with the back buffer
# bound explicitly because ImGui never sets targets itself.
assert u.index('legacyPresent(chain, 0, flags);') < u.index('menu_embed::RenderOverlay(device, context, desc.Width, desc.Height)')
assert 'CreateRenderTargetView(back, nullptr, &overlayTarget)' in u
assert 'context->OMSetRenderTargets(1, &overlayTarget, nullptr)' in u

# Replacement gating: the old interface survives any embed failure.
assert 'const bool replaced=menuOpen && menu_embed::ReplacesLegacyMenu();' in u
assert 'if (menuOpen && menu_embed::ConsumeHudEditorRequest()) hudEditorOpen=true;' in u
assert 'bool ReplacesLegacyMenu();' in h and 'bool ConsumeHudEditorRequest();' in h

# Module wiring: every menu module maps to an existing tested setter; the
# test modules (katana/crown) keep their controls in the test tools window.
for wiring in ('case ModuleId::ChinaHat: SetChinaHatEnabled(enable); return;',
               'case ModuleId::Trail: SetTrailEnabled(enable); return;',
               'case ModuleId::JumpRings: SetJumpEnabled(enable); return;',
               'case ModuleId::Katana: SetKatanaEnabled(enable); return;',
               'case ModuleId::Crown: SetAccessoryEnabled(enable); return;'):
    assert wiring in m, wiring
assert 'g_state.modules.push_back({ModuleId::Katana' in m
assert 'g_state.modules.push_back({ModuleId::Crown' in m
assert 'SetVisualParam(VisualParam::HatRed' in m and 'SetVisualParam(VisualParam::JumpSeconds' in m
assert 'SetHatRainbowEnabled' in m and 'SetTrailRainbowEnabled' in m and 'SetJumpRainbowEnabled' in m
assert 'SetHatFillEnabled' in m and 'SetTrailFillEnabled' in m

# The HUD tab toggles route through the same queued SetModule path as the
# modules tab; without this the flags snapped back to the live state.
for wiring in ('"hud.showFps") == 0) { queueAgModule(static_cast<int>(ag::ModuleId::Fps)',
               '"hud.showWatermark") == 0) { queueAgModule(static_cast<int>(ag::ModuleId::Watermark)',
               '"hud.showPerformance") == 0) {',
               '"hud.showModuleList") == 0) { queueAgModule(static_cast<int>(ag::ModuleId::ActiveModules)'):
    assert wiring in m, wiring
# Module flags come from the menu-owned shadow (seeded once from the live
# state) so the toggles never race the queued SetModule calls.
assert 'g_moduleShadow[ag::kModuleCount]' in m and 'g_moduleShadowReady' in m
assert 'syncModuleShadow();' in m and 'shadowModule(static_cast<int>(ag::ModuleId::Fps))' in m

# The design canvas was tuned so the menu lands near 1.5x on 1080p-class
# game screens (17 px text was unreadable at the old 1:1 fit).
assert 'constexpr float kShellWidth = 1040.0f;' in menucpp
assert 'constexpr float kShellHeight = 580.0f;' in menucpp
assert 'std::clamp(fit, 0.55f, 1.80f)' in menucpp
assert 'std::clamp(state.ui.menuScale / 100.0f, 0.70f, 1.50f)' in menucpp

# The vendored Dear ImGui implementation matches the header version the menu
# was developed against (the shipped DLL links these exact sources).
version=re.search(r'#define IMGUI_VERSION\s+"([^"]+)"',(r/'vendor/imgui/imgui.h').read_text()).group(1)
assert version=='1.91.9b', version
for source in ('imgui.cpp','imgui_draw.cpp','imgui_tables.cpp','imgui_widgets.cpp',
               'backends/imgui_impl_dx11.cpp','backends/imgui_impl_dx11.h'):
    assert (r/'vendor/imgui'/source).exists(), source
print('Embedded GAYClient menu wiring OK; Windows in-game test still required')
