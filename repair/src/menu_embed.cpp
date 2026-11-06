#include "menu_embed.h"

#include <atomic>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <initializer_list>
#include <mutex>
#include <utility>
#include <vector>

#include "imgui.h"
#include "backends/imgui_impl_dx11.h"

#include "GAYClientMenu.h"
#include "china_hat_native.h"
#include "settings.h"
#include "visual_settings.h"

// This translation unit links the user-provided GAYClientMenu (pure Dear
// ImGui code) into the adapter:
//
//  - a private ImGui context + DX11 backend is created on the same device and
//    immediate context the legacy overlay already renders through, so no new
//    hooks or swap chains are needed;
//  - input is forwarded from the existing adapter hooks (patchedPointer for
//    the cursor, the UWP wheel, the legacy key state export), never from new
//    game hooks;
//  - module toggles and graphics settings are mapped one-to-one onto the
//    adapter's existing, tested setters (SetChinaHatEnabled, SetVisualParam,
//    ag::Runtime::SetModule through the compat bridge);
//  - the katana/crown test controls that have no place in the new menu live
//    in a small separate "test tools" window drawn by this module.
//
// Failure of anything above only means the new menu stays hidden and the
// previous interface keeps working.
namespace menu_embed {
namespace {

using gayclient::MenuActions;
using gayclient::MenuState;
using gayclient::ModuleId;

MenuState g_state;
MenuActions g_actions;
ImGuiContext* g_context = nullptr;
bool g_backendReady = false;
bool g_initFailed = false;

std::atomic<bool> g_gameMenuOpen{false};
std::atomic<bool> g_hiddenByClose{false};
std::atomic<bool> g_hudEditorRequest{false};
std::atomic<bool> g_pointerValid{false};
std::atomic<float> g_pointerX{0.0f}, g_pointerY{0.0f};
std::atomic<float> g_pointerW{0.0f}, g_pointerH{0.0f};
std::atomic<float> g_wheelX{0.0f}, g_wheelY{0.0f};
bool (*g_keyDown)(int) = nullptr;
bool (*g_moduleStateReader)(int) = nullptr;
// Toggles for the game's own HUD modules must run on the legacy GUI thread
// (ag::Runtime::SetModule is not documented as thread-safe), so the menu
// callback only queues them; the compat layer drains the queue each frame.
std::mutex g_pendingMutex;
std::vector<std::pair<int, bool>> g_pendingToggles;
ULONGLONG g_lastFrameTick = 0;
bool g_keyStates[256] = {};

float clamp01(float v) { return v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v); }

// Menu percentages (0..100) to the adapter parameter ranges.
float percentToRange(float percent, float low, float high) {
    return low + clamp01(percent / 100.0f) * (high - low);
}
float rangeToPercent(float value, float low, float high) {
    if (high <= low) return 0.0f;
    return clamp01((value - low) / (high - low)) * 100.0f;
}

// Initialize the menu's graphics values from the adapter's live parameters so
// the tab shows what is actually rendered instead of the design defaults.
void initLiveDefaults() {
    auto& hat = g_state.graphics.chinaHat;
    hat.fill = HatFillEnabled();
    hat.rainbow = HatRainbowEnabled();
    hat.rainbowSpeed = rangeToPercent(GetVisualParam(VisualParam::HatRainbowSpeed), 0.03f, 0.80f);
    hat.opacity = GetVisualParam(VisualParam::HatOpacity) * 100.0f;
    hat.radius = GetVisualParam(VisualParam::HatRadius);
    hat.height = GetVisualParam(VisualParam::HatConeHeight);
    hat.offset = GetVisualParam(VisualParam::HatOffset);
    hat.floatAmplitude = GetVisualParam(VisualParam::HatBobAmplitude);
    hat.floatSpeed = GetVisualParam(VisualParam::HatBobSpeed);
    hat.color = ImVec4(GetVisualParam(VisualParam::HatRed), GetVisualParam(VisualParam::HatGreen),
                       GetVisualParam(VisualParam::HatBlue), 1.0f);
    auto& trail = g_state.graphics.trail;
    trail.fill = TrailFillEnabled();
    trail.rainbow = TrailRainbowEnabled();
    trail.rainbowSpeed = rangeToPercent(GetVisualParam(VisualParam::TrailRainbowSpeed), 0.03f, 0.80f);
    trail.opacity = GetVisualParam(VisualParam::TrailOpacity) * 100.0f;
    trail.height = GetVisualParam(VisualParam::TrailHeight);
    trail.life = GetVisualParam(VisualParam::TrailSeconds);
    trail.color = ImVec4(GetVisualParam(VisualParam::TrailRed), GetVisualParam(VisualParam::TrailGreen),
                         GetVisualParam(VisualParam::TrailBlue), 1.0f);
    auto& jump = g_state.graphics.jumpRings;
    jump.rainbow = JumpRainbowEnabled();
    jump.rainbowSpeed = rangeToPercent(GetVisualParam(VisualParam::JumpRainbowSpeed), 0.03f, 0.80f);
    jump.opacity = GetVisualParam(VisualParam::JumpOpacity) * 100.0f;
    jump.radius = GetVisualParam(VisualParam::JumpRadius);
    jump.duration = GetVisualParam(VisualParam::JumpSeconds);
    jump.color = ImVec4(GetVisualParam(VisualParam::JumpRed), GetVisualParam(VisualParam::JumpGreen),
                        GetVisualParam(VisualParam::JumpBlue), 1.0f);
}

void pushChinaHat() {
    const auto& hat = g_state.graphics.chinaHat;
    SetHatFillEnabled(hat.fill);
    SetHatRainbowEnabled(hat.rainbow);
    SetVisualParam(VisualParam::HatRainbowSpeed, percentToRange(hat.rainbowSpeed, 0.03f, 0.80f));
    SetVisualParam(VisualParam::HatOpacity, clamp01(hat.opacity / 100.0f));
    SetVisualParam(VisualParam::HatRadius, hat.radius);
    SetVisualParam(VisualParam::HatConeHeight, hat.height);
    SetVisualParam(VisualParam::HatOffset, hat.offset);
    SetVisualParam(VisualParam::HatBobAmplitude, hat.floatAmplitude);
    SetVisualParam(VisualParam::HatBobSpeed, hat.floatSpeed);
    SetVisualParam(VisualParam::HatRed, clamp01(hat.color.x));
    SetVisualParam(VisualParam::HatGreen, clamp01(hat.color.y));
    SetVisualParam(VisualParam::HatBlue, clamp01(hat.color.z));
}

void pushTrail() {
    const auto& trail = g_state.graphics.trail;
    SetTrailFillEnabled(trail.fill);
    SetTrailRainbowEnabled(trail.rainbow);
    SetVisualParam(VisualParam::TrailRainbowSpeed, percentToRange(trail.rainbowSpeed, 0.03f, 0.80f));
    SetVisualParam(VisualParam::TrailOpacity, clamp01(trail.opacity / 100.0f));
    SetVisualParam(VisualParam::TrailHeight, trail.height);
    SetVisualParam(VisualParam::TrailSeconds, trail.life);
    SetVisualParam(VisualParam::TrailRed, clamp01(trail.color.x));
    SetVisualParam(VisualParam::TrailGreen, clamp01(trail.color.y));
    SetVisualParam(VisualParam::TrailBlue, clamp01(trail.color.z));
}

void pushJumpRings() {
    const auto& jump = g_state.graphics.jumpRings;
    SetJumpRainbowEnabled(jump.rainbow);
    SetVisualParam(VisualParam::JumpRainbowSpeed, percentToRange(jump.rainbowSpeed, 0.03f, 0.80f));
    SetVisualParam(VisualParam::JumpOpacity, clamp01(jump.opacity / 100.0f));
    SetVisualParam(VisualParam::JumpRadius, jump.radius);
    SetVisualParam(VisualParam::JumpSeconds, jump.duration);
    SetVisualParam(VisualParam::JumpRed, clamp01(jump.color.x));
    SetVisualParam(VisualParam::JumpGreen, clamp01(jump.color.y));
    SetVisualParam(VisualParam::JumpBlue, clamp01(jump.color.z));
}

void onSettingChanged(const char* path) {
    if (!path) return;
    if (std::strcmp(path, "graphics.chinaHat.enabled") == 0) { SetChinaHatEnabled(g_state.graphics.chinaHat.enabled); return; }
    if (std::strncmp(path, "graphics.chinaHat.", 18) == 0) { pushChinaHat(); return; }
    if (std::strcmp(path, "graphics.trail.enabled") == 0) { SetTrailEnabled(g_state.graphics.trail.enabled); return; }
    if (std::strncmp(path, "graphics.trail.", 15) == 0) { pushTrail(); return; }
    if (std::strcmp(path, "graphics.jumpRings.enabled") == 0) { SetJumpEnabled(g_state.graphics.jumpRings.enabled); return; }
    if (std::strncmp(path, "graphics.jumpRings.", 19) == 0) { pushJumpRings(); return; }
    // hud.*, ui.*, sound.* and controls.* stay in the menu state for now:
    // the game HUD layout, sound and key bindings are not exposed by the
    // adapter's verified surface, and nothing is guessed here.
}

void queueAgModule(int moduleId, bool enable) {
    std::lock_guard<std::mutex> lock(g_pendingMutex);
    if (g_pendingToggles.size() < 32) g_pendingToggles.emplace_back(moduleId, enable);
}

void onModuleToggled(ModuleId id, bool enable) {
    switch (id) {
        case ModuleId::ChinaHat: SetChinaHatEnabled(enable); return;
        case ModuleId::Trail: SetTrailEnabled(enable); return;
        case ModuleId::JumpRings: SetJumpEnabled(enable); return;
        case ModuleId::Katana: SetKatanaEnabled(enable); return;
        case ModuleId::Crown: SetAccessoryEnabled(enable); return;
        case ModuleId::Fps: queueAgModule(static_cast<int>(ag::ModuleId::Fps), enable); return;
        case ModuleId::Cps: queueAgModule(static_cast<int>(ag::ModuleId::Cps), enable); return;
        case ModuleId::Clock: queueAgModule(static_cast<int>(ag::ModuleId::Clock), enable); return;
        case ModuleId::Keystrokes: queueAgModule(static_cast<int>(ag::ModuleId::Keystrokes), enable); return;
        case ModuleId::Watermark: queueAgModule(static_cast<int>(ag::ModuleId::Watermark), enable); return;
        case ModuleId::Coordinates: queueAgModule(static_cast<int>(ag::ModuleId::Coordinates), enable); return;
        case ModuleId::Resolution: queueAgModule(static_cast<int>(ag::ModuleId::Resolution), enable); return;
        case ModuleId::ModuleList: queueAgModule(static_cast<int>(ag::ModuleId::ActiveModules), enable); return;
        case ModuleId::Performance:
            queueAgModule(static_cast<int>(ag::ModuleId::Cpu), enable);
            queueAgModule(static_cast<int>(ag::ModuleId::Ram), enable);
            return;
    }
}

// Mirror the live game state into the menu so the toggles never drift.
void syncLiveState() {
    g_state.graphics.chinaHat.enabled = ChinaHatEnabled();
    g_state.graphics.trail.enabled = TrailEnabled();
    g_state.graphics.jumpRings.enabled = JumpEnabled();
    if (!g_state.modules.empty() && g_state.modules.back().id != ModuleId::Crown) {
        g_state.modules.push_back({ModuleId::Katana, "Katana (test)", "Графика",
            "Замена алмазного меча катаной (испытание)", false});
        g_state.modules.push_back({ModuleId::Crown, "3D crown (test)", "Графика",
            "Пробная золотистая корона", false});
    }
    for (gayclient::ModuleEntry& module : g_state.modules) {
        switch (module.id) {
            case ModuleId::Katana: module.enabled = KatanaEnabled(); break;
            case ModuleId::Crown: module.enabled = AccessoryEnabled(); break;
            default: break;
        }
    }
    if (g_moduleStateReader) {
        g_state.hud.showFps = g_moduleStateReader(static_cast<int>(ag::ModuleId::Fps));
        g_state.hud.showCps = g_moduleStateReader(static_cast<int>(ag::ModuleId::Cps));
        g_state.hud.showClock = g_moduleStateReader(static_cast<int>(ag::ModuleId::Clock));
        g_state.hud.showKeystrokes = g_moduleStateReader(static_cast<int>(ag::ModuleId::Keystrokes));
        g_state.hud.showWatermark = g_moduleStateReader(static_cast<int>(ag::ModuleId::Watermark));
        g_state.hud.showCoordinates = g_moduleStateReader(static_cast<int>(ag::ModuleId::Coordinates));
        g_state.hud.showResolution = g_moduleStateReader(static_cast<int>(ag::ModuleId::Resolution));
        g_state.hud.showPerformance = g_moduleStateReader(static_cast<int>(ag::ModuleId::Cpu));
        g_state.hud.showModuleList = g_moduleStateReader(static_cast<int>(ag::ModuleId::ActiveModules));
    }
}

void wireActions() {
    g_actions.settingChanged = [](const char* path) { onSettingChanged(path); };
    g_actions.moduleToggled = [](ModuleId id, bool enabled) { onModuleToggled(id, enabled); };
    g_actions.closeRequested = [] { g_hiddenByClose.store(true, std::memory_order_relaxed); };
    g_actions.hudEditorRequested = [] { g_hudEditorRequest.store(true, std::memory_order_relaxed); };
    // Profiles, resets, statistics and preview textures have no verified
    // counterpart in the adapter yet; the callbacks stay unset and the menu
    // keeps its honest "no data" presentation for those.
}

bool keyIsDown(int vk) {
    if (g_keyDown && g_keyDown(vk)) return true;
    return (GetAsyncKeyState(vk) & 0x8000) != 0;
}

struct KeyMap { int vk; ImGuiKey key; bool character; };
const KeyMap kKeys[] = {
    {VK_BACK, ImGuiKey_Backspace, false},   {VK_RETURN, ImGuiKey_Enter, false},
    {VK_ESCAPE, ImGuiKey_Escape, false},    {VK_SPACE, ImGuiKey_Space, true},
    {VK_DELETE, ImGuiKey_Delete, false},    {VK_LEFT, ImGuiKey_LeftArrow, false},
    {VK_RIGHT, ImGuiKey_RightArrow, false}, {VK_UP, ImGuiKey_UpArrow, false},
    {VK_DOWN, ImGuiKey_DownArrow, false},   {VK_PRIOR, ImGuiKey_PageUp, false},
    {VK_NEXT, ImGuiKey_PageDown, false},    {VK_HOME, ImGuiKey_Home, false},
    {VK_END, ImGuiKey_End, false},          {VK_SHIFT, ImGuiKey_LeftShift, false},
    {VK_CONTROL, ImGuiKey_LeftCtrl, false},
};
constexpr int kKeyCount = sizeof(kKeys) / sizeof(kKeys[0]);

void feedInput(unsigned width, unsigned height) {
    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2(static_cast<float>(width), static_cast<float>(height));
    const ULONGLONG now = GetTickCount64();
    io.DeltaTime = g_lastFrameTick && now > g_lastFrameTick
        ? static_cast<float>(now - g_lastFrameTick) / 1000.0f
        : 1.0f / 60.0f;
    if (io.DeltaTime <= 0.0f || io.DeltaTime > 0.25f) io.DeltaTime = 1.0f / 60.0f;
    g_lastFrameTick = now;

    if (g_pointerValid.load(std::memory_order_relaxed)) {
        const float w = g_pointerW.load(std::memory_order_relaxed);
        const float h = g_pointerH.load(std::memory_order_relaxed);
        if (w > 1.0f && h > 1.0f) {
            const float mx = g_pointerX.load(std::memory_order_relaxed) * (static_cast<float>(width) / w);
            const float my = g_pointerY.load(std::memory_order_relaxed) * (static_cast<float>(height) / h);
            io.AddMousePosEvent(mx, my);
        }
    }
    io.AddMouseButtonEvent(0, keyIsDown(VK_LBUTTON));
    io.AddMouseButtonEvent(1, keyIsDown(VK_RBUTTON));
    const float wheelX = g_wheelX.exchange(0.0f, std::memory_order_relaxed);
    const float wheelY = g_wheelY.exchange(0.0f, std::memory_order_relaxed);
    if (wheelX != 0.0f || wheelY != 0.0f) io.AddMouseWheelEvent(wheelX, wheelY);

    const bool shift = keyIsDown(VK_SHIFT);
    for (int i = 0; i < kKeyCount; ++i) {
        const bool down = keyIsDown(kKeys[i].vk);
        if (down == g_keyStates[kKeys[i].vk]) continue;
        g_keyStates[kKeys[i].vk] = down;
        io.AddKeyEvent(kKeys[i].key, down);
        if (down && kKeys[i].character)
            io.AddInputCharacter(static_cast<unsigned int>(shift ? kKeys[i].vk : (kKeys[i].vk | 0x20)));
    }
    for (int vk = '0'; vk <= '9'; ++vk) {
        const bool down = keyIsDown(vk);
        if (down == g_keyStates[vk]) continue;
        g_keyStates[vk] = down;
        io.AddKeyEvent(static_cast<ImGuiKey>(ImGuiKey_0 + (vk - '0')), down);
        if (down) io.AddInputCharacter(static_cast<unsigned int>(vk));
    }
    for (int vk = 'A'; vk <= 'Z'; ++vk) {
        const bool down = keyIsDown(vk);
        if (down == g_keyStates[vk]) continue;
        g_keyStates[vk] = down;
        io.AddKeyEvent(static_cast<ImGuiKey>(ImGuiKey_A + (vk - 'A')), down);
        if (down) io.AddInputCharacter(static_cast<unsigned int>(shift ? vk : (vk | 0x20)));
    }
}

void drawTestToolsWindow() {
    const bool katana = KatanaEnabled();
    const bool crown = AccessoryEnabled();
    if (!katana && !crown) return;
    ImGui::SetNextWindowPos(ImVec2(18.0f, ImGui::GetIO().DisplaySize.y - 320.0f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(330.0f, 300.0f), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("Test tools (adapter)", nullptr,
                      ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings)) {
        ImGui::End();
        return;
    }
    if (katana && ImGui::CollapsingHeader("Katana (test)", ImGuiTreeNodeFlags_DefaultOpen)) {
        const int status = KatanaStatus();
        ImGui::TextUnformatted(status == 1 ? "Katana: drawing (world pass)"
            : status == 2 ? "Katana: draw unavailable; original kept"
            : status == 9 ? "Katana: local player not matched"
            : status == 10 ? "Katana: sword not matched"
            : status == 11 ? "Katana: sword candidate"
            : status == 12 ? "Katana: third person not confirmed yet"
            : status == 13 ? "Katana: rotation unreadable; original kept"
            : status == 14 ? "Katana: visible; replacing sword"
            : status == 15 ? "Katana: model invalid"
            : "Katana: waiting for held diamond sword");
        const unsigned e = KatanaEvidence();
        ImGui::Text("F:%u V:%u S:%u C:%u", !!e, !!(e & 2), !!(e & 16), !!(e & 32) | !!(e & 64));
        ImGui::Text("R:%u X:%u D:%u O:%u P:%u", !!(e & 256), !!(e & 512), !!(e & 128), !!(e & 2048), !!(e & 4096) | !!(e & 8192));
        const auto d = KatanaDebugSnapshot();
        ImGui::Text("Rot P:%u A:%u B:%u H:%u%s yaw:%+.2f pitch:%+.2f",
            d.sources & 1 ? 1u : 0u, d.sources & 2 ? 1u : 0u, d.sources & 4 ? 1u : 0u, d.sources & 8 ? 1u : 0u,
            d.wrapped ? " (wrapped)" : "", d.yaw, d.pitchUp);
        ImGui::Text("CI:%llX PL:%llX H58:%llX", d.clientInstance, d.player, d.rotationHolder);
        ImGui::Text("TP in:%d out:%d tex:%llX learned:%llX",
            d.tpCallsInPass, d.tpCallsOutOfPass, d.tpLastTexture, d.learnedTexture);
        struct Slider { const char* label; VisualParam param; float low, high; };
        const Slider sliders[] = {
            {"Scale", VisualParam::KatanaScale, 0.4f, 1.0f},
            {"Blade tilt", VisualParam::KatanaTilt, 0.2f, 1.55f},
            {"Side", VisualParam::KatanaSide, 0.0f, 0.8f},
            {"Hand height", VisualParam::KatanaHeight, 0.6f, 1.8f},
            {"Forward", VisualParam::KatanaForward, -0.5f, 1.0f},
            {"Opacity", VisualParam::KatanaOpacity, 0.3f, 1.0f},
            {"Side (FP)", VisualParam::KatanaFirstSide, 0.0f, 0.8f},
            {"Forward (FP)", VisualParam::KatanaFirstForward, -0.3f, 1.0f},
        };
        for (const Slider& slider : sliders) {
            float value = GetVisualParam(slider.param);
            if (ImGui::SliderFloat(slider.label, &value, slider.low, slider.high, "%.2f"))
                SetVisualParam(slider.param, value);
        }
    }
    if (crown && ImGui::CollapsingHeader("3D crown (test)")) {
        struct Slider { const char* label; VisualParam param; };
        const Slider sliders[] = {
            {"Scale", VisualParam::AccessoryScale}, {"Offset", VisualParam::AccessoryOffset},
            {"Opacity", VisualParam::AccessoryOpacity}, {"Red", VisualParam::AccessoryRed},
            {"Green", VisualParam::AccessoryGreen}, {"Blue", VisualParam::AccessoryBlue},
        };
        for (const Slider& slider : sliders) {
            float value = GetVisualParam(slider.param);
            if (ImGui::SliderFloat(slider.label, &value, 0.0f, 2.0f, "%.2f"))
                SetVisualParam(slider.param, value);
        }
    }
    ImGui::End();
}

} // namespace

void SetPointer(float clientX, float clientY, float clientWidth, float clientHeight) {
    if (clientWidth <= 0.0f || clientHeight <= 0.0f) return;
    g_pointerX.store(clientX, std::memory_order_relaxed);
    g_pointerY.store(clientY, std::memory_order_relaxed);
    g_pointerW.store(clientWidth, std::memory_order_relaxed);
    g_pointerH.store(clientHeight, std::memory_order_relaxed);
    g_pointerValid.store(true, std::memory_order_relaxed);
}

void AddWheel(float dx, float dy) {
    if (dx == 0.0f && dy == 0.0f) return;
    g_wheelX.store(dx, std::memory_order_relaxed);
    g_wheelY.store(dy, std::memory_order_relaxed);
}

void SetKeyQuery(bool (*query)(int)) { g_keyDown = query; }
void SetModuleStateReader(bool (*reader)(int)) { g_moduleStateReader = reader; }

void DrainModuleToggles(void (*toggler)(int, bool)) {
    if (!toggler) return;
    std::vector<std::pair<int, bool>> pending;
    {
        std::lock_guard<std::mutex> lock(g_pendingMutex);
        if (g_pendingToggles.empty()) return;
        pending.swap(g_pendingToggles);
    }
    for (const auto& toggle : pending) toggler(toggle.first, toggle.second);
}

void SetGameMenuOpen(bool open) {
    if (!open) g_hiddenByClose.store(false, std::memory_order_relaxed);
    g_gameMenuOpen.store(open, std::memory_order_relaxed);
}

bool ReplacesLegacyMenu() { return g_backendReady; }
bool ConsumeHudEditorRequest() { return g_hudEditorRequest.exchange(false, std::memory_order_relaxed); }

void RenderOverlay(ID3D11Device* device, ID3D11DeviceContext* context,
                   unsigned width, unsigned height) {
    if (g_initFailed || !device || !context || width < 200 || height < 200) return;
    if (!g_backendReady) {
        ImGuiContext* created = ImGui::CreateContext();
        ImGui::SetCurrentContext(created);
        g_context = created;
        ImGuiIO& io = ImGui::GetIO();
        io.IniFilename = nullptr;  // no ini file writes from inside the game
        g_state.cyrillicFontLoaded = gayclient::LoadCyrillicFont(io, 17.0f);
        if (!ImGui_ImplDX11_Init(device, context)) {
            ImGui::DestroyContext(created);
            g_context = nullptr;
            g_initFailed = true;
            return;
        }
        g_state.dataConnected = true;
        wireActions();
        initLiveDefaults();
        g_backendReady = true;
    }
    ImGui::SetCurrentContext(g_context);
    feedInput(width, height);
    ImGui::NewFrame();
    const bool open = g_gameMenuOpen.load(std::memory_order_relaxed) &&
                      !g_hiddenByClose.load(std::memory_order_relaxed);
    g_state.visible = open;
    if (open) {
        syncLiveState();
        gayclient::RenderMenu(g_state, g_actions);
        drawTestToolsWindow();
    }
    ImGui::Render();
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}

} // namespace menu_embed
