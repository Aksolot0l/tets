#pragma once
#include <cstdint>
struct ID3D11Device;
struct ID3D11RenderTargetView;

// Full replacement of the adapter's game menu, built on RmlUi (HTML/CSS with
// animations) instead of Dear ImGui. The markup and styling live in
// menu_rml_data.h; this module owns the RmlUi context, the DX11 render
// interface, the input plumbing and the wiring onto the adapter's existing,
// verified setters. Everything below is called from existing adapter hooks,
// so no new game hooks are introduced.
namespace menu_rml {

// Called at the end of the Present-path draw, after the legacy overlay has
// rendered. Creates and renders the RmlUi menu plus the katana/crown test
// tools window. The caller provides the back-buffer render target view; the
// RmlUi DX11 renderer binds it during EndFrame and restores all DirectX
// state afterwards. Safe no-op until initialization succeeds once.
void RenderOverlay(ID3D11Device* device, ID3D11RenderTargetView* rtv,
                   unsigned width, unsigned height);

// Input plumbing from the existing adapter hooks. All cross-thread safe.
void SetPointer(float clientX, float clientY, float clientWidth, float clientHeight);
void AddWheel(float dx, float dy);
void SetKeyQuery(bool (*query)(int vk));

// Live game state plumbing: menu visibility follows the game menu flag, the
// HUD-style module flags are read through the reader, and toggles requested
// from the menu are queued here and must be drained on the legacy GUI thread
// by DrainModuleToggles, which forwards them to ag::Runtime::SetModule.
void SetGameMenuOpen(bool open);
void SetModuleStateReader(bool (*reader)(int moduleId));
void DrainModuleToggles(void (*toggler)(int moduleId, bool));

// True once the RmlUi menu has rendered a full frame; the legacy click GUI
// and the old test panels stay hidden while this holds, falling back
// automatically otherwise.
bool ReplacesLegacyMenu();

// One-shot request from the menu's HUD-editor buttons.
bool ConsumeHudEditorRequest();

// True while the 3D preview toggle in the Visuals window is on. The compat
// layer draws the actual 3D portrait window (game ImGui pass) and the menu
// reserves layout space for it on the right when this holds.
bool VisualPreviewActive();

// Last pointer sample received from the adapter's pointer hook, in client
// DIPs. Used by the compat layer's HUD-drag editor to hit-test widgets.
bool ReadPointer(float& x, float& y, float& w, float& h);

} // namespace menu_rml
