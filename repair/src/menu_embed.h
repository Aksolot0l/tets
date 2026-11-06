#pragma once
#include <cstdint>
struct ID3D11Device;
struct ID3D11DeviceContext;

// Embedding of the new GAYClient menu (GAYClientMenu.h/.cpp, Dear ImGui
// 1.91.9b) into the adapter. The menu renders with its own private ImGui
// context and its own DX11 backend objects, created from the same device and
// immediate context the legacy overlay already uses; the legacy client's
// ImGui context is never touched. Everything below is called from existing
// adapter hooks, so no new game hooks are introduced.
namespace menu_embed {

// Called at the end of the Present-path draw, after the legacy overlay has
// rendered. Renders the new menu plus the test-tools window with a private
// ImGui context. Safe no-op until initialization succeeds once.
void RenderOverlay(ID3D11Device* device, ID3D11DeviceContext* context,
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

// True once the new menu is alive; the legacy click GUI and the old test
// panels stay hidden while this holds, falling back automatically otherwise.
bool ReplacesLegacyMenu();

// One-shot request from the menu's HUD-editor button.
bool ConsumeHudEditorRequest();

} // namespace menu_embed
