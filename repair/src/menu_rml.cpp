#include "menu_rml.h"

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdio>
#include <cstdarg>
#include <cstring>
#include <memory>
#include <mutex>
#include <utility>
#include <vector>

#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/Core.h>
#include <RmlUi/Core/Element.h>
#include <RmlUi/Core/ElementDocument.h>
#include <RmlUi/Core/Event.h>
#include <RmlUi/Core/EventListener.h>
#include <RmlUi/Core/Input.h>
#include <RmlUi/Core/StringUtilities.h>
#include <RmlUi/Core/SystemInterface.h>
#include <RmlUi/Core/Types.h>

#include "RmlUi_Renderer_DX11.h"

#include "china_hat_native.h"
#include "menu_layout.h"
#include "menu_rml_data.h"
#include "settings.h"
#include "visual_settings.h"

// RmlUi-based replacement of the adapter's game menu. The document markup and
// styling come from menu_rml_data.h; this translation unit:
//
//  - creates one RmlUi context with the vendored DX11 render interface on the
//    same device the legacy overlay already renders through (the back-buffer
//    render target view is passed in by the caller; RmlUi binds it during
//    EndFrame and restores the DirectX state afterwards);
//  - feeds input from the existing adapter hooks (patchedPointer, the UWP
//    wheel, the legacy key state export), never from new game hooks;
//  - maps switches and sliders one-to-one onto the adapter's existing,
//    verified setters (SetChinaHatEnabled, SetVisualParam, and
//    ag::Runtime::SetModule through the queued compat bridge);
//  - lays the menu out as independent windows: each window is dragged by its
//    title bar (RmlUi drag events), folds to its title bar, scrolls inside
//    when the viewport is small, and the default tiling (menu_layout.h)
//    never overlaps windows, whatever the number of functions.
//
// Failure of anything above only means the new menu stays hidden and the
// previous interface keeps working.
namespace menu_rml {
namespace {

// ---------------------------------------------------------------------------
// Cross-thread input and state plumbing (mirrors the previous menu embed).
// ---------------------------------------------------------------------------

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
// (ag::Runtime::SetModule is not documented as thread-safe), so the menu only
// queues them; the compat layer drains the queue each frame.
std::mutex g_pendingMutex;
std::vector<std::pair<int, bool>> g_pendingToggles;

// The menu's own copy of the game's HUD module flags (indexed by
// ag::ModuleId): it initializes from the live state once and then follows the
// user's toggles, because re-reading the game state every frame raced the
// queued SetModule calls and the switch visibly jumped back.
bool g_moduleShadow[ag::kModuleCount] = {};
bool g_moduleShadowReady = false;

bool g_ready = false;
bool g_initFailed = false;
std::atomic<bool> g_renderOk{false};

bool g_mouseStates[2] = {};
bool g_escState = false;

RenderInterface_DX11* g_renderer = nullptr;
Rml::Context* g_rmlContext = nullptr;
Rml::ElementDocument* g_menuDoc = nullptr;
Rml::Element* g_ttStatus = nullptr;
Rml::Element* g_ttDiag = nullptr;
unsigned g_width = 0, g_height = 0;
bool g_menuShown = false;
// Document fade: frames left to keep rendering after the menu closed (the
// fade-out transition needs them).
constexpr unsigned kFadeOutFrames = 12;
unsigned g_fadeOutFrames = 0;

// ---------------------------------------------------------------------------
// Window manager state: the two menu windows with their natural sizes and
// live geometry. Positions are owned here (never read back from the DOM), so
// dragging and re-tiling cannot desync from what is rendered.
// ---------------------------------------------------------------------------

// UI zoom: the menu ships at a 1080p reference size; on small windows the
// fonts and shapes read as pixel mush. Every px in the RCSS and every layout
// constant below is multiplied by this factor at load time.
float g_uiScale = 1.0f;
// Screen-pixel strip reserved on the right for the compat layer's 3D preview
// window (drawn by the game's ImGui pass, so it is NOT multiplied by
// g_uiScale; it matches the ImGui window's pixel size).
constexpr float kPreviewInsetPx = 356.0f;

float computeUiScale(unsigned width, unsigned height) {
    const float ref = std::fmax(static_cast<float>(height) / 1080.0f,
                                 static_cast<float>(width) / 1920.0f);
    // v7: v6's compact zoom times 1.6 (1080p = 1.0, floor 0.92, 4K = 1.6).
    const float scale = ref * 1.25f * 0.5f * 1.6f;
    if (scale < 0.92f) return 0.92f;
    if (scale > 1.6f) return 1.6f;
    return scale;
}

// Multiply every `<number>px` in the RCSS by the UI scale. 0px stays 0px;
// deg/percent values have no px unit and are left alone.
std::string scaleRcssPx(const char* rcss, float scale) {
    if (scale == 1.0f) return std::string(rcss);
    std::string out;
    out.reserve(std::strlen(rcss) + 1024);
    const char* p = rcss;
    const char* end = p + std::strlen(rcss);
    while (p < end) {
        // Match -?digits(.digits)? followed by 'px'.
        const char* numberStart = p;
        if (*p == '-' && p + 1 < end && *(p + 1) >= '0' && *(p + 1) <= '9') ++p;
        while (p < end && *p >= '0' && *p <= '9') ++p;
        if (p < end && *p == '.' && p + 1 < end && *(p + 1) >= '0' && *(p + 1) <= '9') {
            ++p;
            while (p < end && *p >= '0' && *p <= '9') ++p;
        }
        if (p + 1 < end && p[0] == 'p' && p[1] == 'x' && p > numberStart) {
            const double value = std::atof(std::string(numberStart, p - numberStart).c_str());
            char scaled[48];
            std::snprintf(scaled, sizeof(scaled), "%.2fpx", value * static_cast<double>(scale));
            out += scaled;
            p += 2;
        } else {
            p = numberStart;
            out += *p;
            ++p;
        }
    }
    return out;
}

struct WinDef {
    const char* id;
    const char* headId;
    float w;
    float h;
};

// Natural (reference 1080p) sizes; multiplied by g_uiScale at init. v7: the
// Visuals window is the same height as Home; its body scrolls inside.
const WinDef kWindows[] = {
    {"win-home",    "head-home",    300.0f, 750.0f},
    {"win-visuals", "head-visuals", 460.0f, 750.0f},
};
constexpr int kWindowCount = static_cast<int>(sizeof(kWindows) / sizeof(kWindows[0]));
constexpr float kFoldedHeightRef = 46.0f;  // title bar + borders (reference)
float kFoldedHeight = kFoldedHeightRef;    // scaled at init
// Window fold/unfold runs as a short C++ height interpolation (RmlUi has no
// keyframes, and animating an explicit height property from CSS would fight
// the layout math that owns the same property).
constexpr unsigned kFoldAnimMs = 150;

struct WinState {
    Rml::Element* el = nullptr;
    Rml::Element* head = nullptr;
    float w = 0.0f, h = 0.0f;      // natural size
    float x = 0.0f, y = 0.0f;      // current position
    float shownH = 0.0f;           // unfolded height after tiling caps it
    bool dragged = false;          // user moved it: keep the position
    bool folded = false;
    float grabDx = 0.0f, grabDy = 0.0f;
    // Fold animation: animFrom -> animTo over kFoldAnimMs while animActive.
    bool animActive = false;
    DWORD animStart = 0;
    float animFrom = 0.0f, animTo = 0.0f, animH = 0.0f;
};
WinState g_win[kWindowCount];
int g_zTop = 20;
std::string g_ttStatusCache;
std::string g_ttDiagCache;

// 3D preview toggle (Visuals window).
std::atomic<bool> g_previewOn{true};
Rml::Element* g_previewSwitch = nullptr;

std::vector<char> g_fontNormal;
std::vector<char> g_fontBold;

float clamp01(float v) { return v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v); }

// ---------------------------------------------------------------------------
// Diagnostic log: appended next to this DLL as gayclient-menu.log, so init
// failures and RmlUi parse errors are diagnosable on the user's machine
// without a debugger.
// ---------------------------------------------------------------------------

std::mutex g_logMutex;

void menuLog(const char* format, ...) {
    char path[MAX_PATH] = {};
    HMODULE module = nullptr;
    if (!GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                                GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                            reinterpret_cast<LPCSTR>(&menuLog), &module) || !module)
        return;
    const DWORD length = GetModuleFileNameA(module, path, MAX_PATH);
    if (length == 0 || length >= MAX_PATH) return;
    char* slash = std::strrchr(path, '\\');
    if (!slash) return;
    std::snprintf(slash + 1, MAX_PATH - (slash + 1 - path), "gayclient-menu.log");
    char text[512];
    va_list args;
    va_start(args, format);
    const int written = std::vsnprintf(text, sizeof(text) - 1, format, args);
    va_end(args);
    if (written <= 0) return;
    std::lock_guard<std::mutex> lock(g_logMutex);
    if (std::FILE* file = std::fopen(path, "a")) {
        std::fprintf(file, "[%10u] %s\n", GetTickCount(), text);
        std::fclose(file);
    }
}

// Routes RmlUi's own log (font/document/RCSS/render errors) into the file.
class MenuSystemInterface : public Rml::SystemInterface {
public:
    bool LogMessage(Rml::Log::Type type, const Rml::String& message) override {
        menuLog("rmlui(%d): %s", static_cast<int>(type), message.c_str());
        return true;
    }
};
MenuSystemInterface g_systemInterface;

// Menu percentages (0..100) to the adapter parameter ranges.
float percentToRange(float percent, float low, float high) {
    return low + clamp01(percent / 100.0f) * (high - low);
}
float rangeToPercent(float value, float low, float high) {
    if (high <= low) return 0.0f;
    return clamp01((value - low) / (high - low)) * 100.0f;
}

bool keyIsDown(int vk) {
    if (g_keyDown && g_keyDown(vk)) return true;
    return (GetAsyncKeyState(vk) & 0x8000) != 0;
}

void queueAgModule(int moduleId, bool enable) {
    const int index = moduleId;
    if (index >= 0 && index < static_cast<int>(ag::kModuleCount))
        g_moduleShadow[index] = enable;
    std::lock_guard<std::mutex> lock(g_pendingMutex);
    if (g_pendingToggles.size() < 32) g_pendingToggles.emplace_back(moduleId, enable);
}

void syncModuleShadow() {
    if (g_moduleShadowReady || !g_moduleStateReader) return;
    for (int i = 0; i < static_cast<int>(ag::kModuleCount); ++i)
        g_moduleShadow[i] = g_moduleStateReader(i);
    g_moduleShadowReady = true;
}

bool shadowModule(int moduleId) {
    if (moduleId < 0 || moduleId >= static_cast<int>(ag::kModuleCount)) return false;
    return g_moduleShadow[moduleId];
}

// ---------------------------------------------------------------------------
// Switch registry: every <button class="switch"> in both documents.
// ---------------------------------------------------------------------------

enum class SwKind {
    AgModule, ChinaHat, Trail, Jump, Katana,
    ChFill, ChRainbow, TrFill, TrRainbow, JrRainbow
};

struct SwitchDef {
    const char* id;
    SwKind kind;
    int ag;  // ag::ModuleId for SwKind::AgModule
};

const SwitchDef kSwitches[] = {
    // Home window: game HUD modules.
    {"sw-fps", SwKind::AgModule, static_cast<int>(ag::ModuleId::Fps)},
    {"sw-cps", SwKind::AgModule, static_cast<int>(ag::ModuleId::Cps)},
    {"sw-clock", SwKind::AgModule, static_cast<int>(ag::ModuleId::Clock)},
    {"sw-keys", SwKind::AgModule, static_cast<int>(ag::ModuleId::Keystrokes)},
    {"sw-watermark", SwKind::AgModule, static_cast<int>(ag::ModuleId::Watermark)},
    {"sw-coords", SwKind::AgModule, static_cast<int>(ag::ModuleId::Coordinates)},
    {"sw-res", SwKind::AgModule, static_cast<int>(ag::ModuleId::Resolution)},
    {"sw-perf", SwKind::AgModule, static_cast<int>(ag::ModuleId::Cpu)},
    {"sw-perf", SwKind::AgModule, static_cast<int>(ag::ModuleId::Ram)},
    {"sw-modlist", SwKind::AgModule, static_cast<int>(ag::ModuleId::ActiveModules)},
    // Visuals window: one master checkbox per effect, inside its section
    // (v6 removed the duplicate master rows at the top of the window).
    {"sw-gfx-chinahat", SwKind::ChinaHat, 0},
    {"sw-gfx-trail", SwKind::Trail, 0},
    {"sw-gfx-jump", SwKind::Jump, 0},
    {"sw-kt-master", SwKind::Katana, 0},
    {"sw-ch-fill", SwKind::ChFill, 0},
    {"sw-ch-rainbow", SwKind::ChRainbow, 0},
    {"sw-tr-fill", SwKind::TrFill, 0},
    {"sw-tr-rainbow", SwKind::TrRainbow, 0},
    {"sw-jr-rainbow", SwKind::JrRainbow, 0},
};

const SwitchDef* findSwitch(const Rml::String& id) {
    for (const SwitchDef& def : kSwitches)
        if (id == def.id) return &def;
    return nullptr;
}

bool readSwitch(const SwitchDef& def) {
    switch (def.kind) {
        case SwKind::AgModule: return shadowModule(def.ag);
        case SwKind::ChinaHat: return ChinaHatEnabled();
        case SwKind::Trail: return TrailEnabled();
        case SwKind::Jump: return JumpEnabled();
        case SwKind::Katana: return KatanaEnabled();
        case SwKind::ChFill: return HatFillEnabled();
        case SwKind::ChRainbow: return HatRainbowEnabled();
        case SwKind::TrFill: return TrailFillEnabled();
        case SwKind::TrRainbow: return TrailRainbowEnabled();
        case SwKind::JrRainbow: return JumpRainbowEnabled();
    }
    return false;
}

void applySwitch(const SwitchDef& def, bool on) {
    switch (def.kind) {
        case SwKind::AgModule: queueAgModule(def.ag, on); break;
        case SwKind::ChinaHat: SetChinaHatEnabled(on); break;
        case SwKind::Trail: SetTrailEnabled(on); break;
        case SwKind::Jump: SetJumpEnabled(on); break;
        case SwKind::Katana: SetKatanaEnabled(on); break;
        case SwKind::ChFill: SetHatFillEnabled(on); break;
        case SwKind::ChRainbow: SetHatRainbowEnabled(on); break;
        case SwKind::TrFill: SetTrailFillEnabled(on); break;
        case SwKind::TrRainbow: SetTrailRainbowEnabled(on); break;
        case SwKind::JrRainbow: SetJumpRainbowEnabled(on); break;
    }
}

// ---------------------------------------------------------------------------
// Slider registry: every <input type="range"> in both documents.
// ---------------------------------------------------------------------------

enum class SlScale {
    Direct,          // DOM value == parameter value
    Percent,         // DOM 0..100 -> parameter 0..1
    RainbowPercent   // DOM 0..100 -> parameter 0.03..0.8
};

struct SliderDef {
    const char* id;       // DOM id of the <input type="range">
    VisualParam param;    // adapter visual parameter
    SlScale scale;
    const char* fmt;      // readout printf format
};

const SliderDef kSliders[] = {
    // China hat (graphics tab).
    {"sl-ch-rainbow", VisualParam::HatRainbowSpeed, SlScale::RainbowPercent, "%.0f%%"},
    {"sl-ch-opacity", VisualParam::HatOpacity, SlScale::Percent, "%.0f%%"},
    {"sl-ch-radius", VisualParam::HatRadius, SlScale::Direct, "%.2f"},
    {"sl-ch-height", VisualParam::HatConeHeight, SlScale::Direct, "%.2f"},
    {"sl-ch-offset", VisualParam::HatOffset, SlScale::Direct, "%.2f"},
    {"sl-ch-amp", VisualParam::HatBobAmplitude, SlScale::Direct, "%.2f"},
    {"sl-ch-floatspeed", VisualParam::HatBobSpeed, SlScale::Direct, "%.2f"},
    {"sl-ch-r", VisualParam::HatRed, SlScale::Direct, "%.2f"},
    {"sl-ch-g", VisualParam::HatGreen, SlScale::Direct, "%.2f"},
    {"sl-ch-b", VisualParam::HatBlue, SlScale::Direct, "%.2f"},
    // Trail.
    {"sl-tr-rainbow", VisualParam::TrailRainbowSpeed, SlScale::RainbowPercent, "%.0f%%"},
    {"sl-tr-opacity", VisualParam::TrailOpacity, SlScale::Percent, "%.0f%%"},
    {"sl-tr-height", VisualParam::TrailHeight, SlScale::Direct, "%.2f"},
    {"sl-tr-life", VisualParam::TrailSeconds, SlScale::Direct, "%.2f"},
    {"sl-tr-r", VisualParam::TrailRed, SlScale::Direct, "%.2f"},
    {"sl-tr-g", VisualParam::TrailGreen, SlScale::Direct, "%.2f"},
    {"sl-tr-b", VisualParam::TrailBlue, SlScale::Direct, "%.2f"},
    // Jump rings.
    {"sl-jr-rainbow", VisualParam::JumpRainbowSpeed, SlScale::RainbowPercent, "%.0f%%"},
    {"sl-jr-opacity", VisualParam::JumpOpacity, SlScale::Percent, "%.0f%%"},
    {"sl-jr-radius", VisualParam::JumpRadius, SlScale::Direct, "%.2f"},
    {"sl-jr-duration", VisualParam::JumpSeconds, SlScale::Direct, "%.2f"},
    {"sl-jr-r", VisualParam::JumpRed, SlScale::Direct, "%.2f"},
    {"sl-jr-g", VisualParam::JumpGreen, SlScale::Direct, "%.2f"},
    {"sl-jr-b", VisualParam::JumpBlue, SlScale::Direct, "%.2f"},
    // Katana (graphics tab).
    {"sl-kt-scale", VisualParam::KatanaScale, SlScale::Direct, "%.2f"},
    {"sl-kt-tilt", VisualParam::KatanaTilt, SlScale::Direct, "%.2f"},
    {"sl-kt-side", VisualParam::KatanaSide, SlScale::Direct, "%.2f"},
    {"sl-kt-height", VisualParam::KatanaHeight, SlScale::Direct, "%.2f"},
    {"sl-kt-forward", VisualParam::KatanaForward, SlScale::Direct, "%.2f"},
    {"sl-kt-opacity", VisualParam::KatanaOpacity, SlScale::Direct, "%.2f"},
    {"sl-kt-fpside", VisualParam::KatanaFirstSide, SlScale::Direct, "%.2f"},
    {"sl-kt-fpforward", VisualParam::KatanaFirstForward, SlScale::Direct, "%.2f"},
};

float domFromParam(const SliderDef& def, float param) {
    switch (def.scale) {
        case SlScale::Direct: return param;
        case SlScale::Percent: return clamp01(param) * 100.0f;
        case SlScale::RainbowPercent: return rangeToPercent(param, 0.03f, 0.80f);
    }
    return param;
}

float paramFromDom(const SliderDef& def, float dom) {
    switch (def.scale) {
        case SlScale::Direct: return dom;
        case SlScale::Percent: return clamp01(dom / 100.0f);
        case SlScale::RainbowPercent: return percentToRange(dom, 0.03f, 0.80f);
    }
    return dom;
}

const SliderDef* findSlider(const Rml::String& id) {
    for (const SliderDef& def : kSliders)
        if (id == def.id) return &def;
    return nullptr;
}

// Readout id: "sl-xxx" -> "ro-xxx".
Rml::String readoutIdFrom(const char* sliderId) {
    Rml::String out(sliderId);
    if (out.size() >= 2) out[1] = 'o';
    return out;
}

struct SliderElem {
    Rml::Element* input = nullptr;
    Rml::Element* readout = nullptr;
    const SliderDef* def = nullptr;
    std::string readoutCache;
};

struct SwitchElem {
    Rml::Element* el = nullptr;
    const SwitchDef* def = nullptr;
};

std::vector<SliderElem> g_sliderElems;
std::vector<SwitchElem> g_switchElems;

// ---------------------------------------------------------------------------
// Event listeners.
// ---------------------------------------------------------------------------

class SliderChangeListener : public Rml::EventListener {
public:
    explicit SliderChangeListener(const SliderDef* def) : m_def(def) {}

    void ProcessEvent(Rml::Event& event) override {
        Rml::Element* element = event.GetCurrentElement();
        if (!element || !m_def) return;
        const Rml::Variant* value = element->GetAttribute("value");
        if (!value) return;
        const float dom = value->Get<float>();
        SetVisualParam(m_def->param, paramFromDom(*m_def, dom));
    }

private:
    const SliderDef* m_def;
};

class MenuClickListener : public Rml::EventListener {
public:
    void ProcessEvent(Rml::Event& event) override;
};

MenuClickListener g_clickListener;
std::vector<std::unique_ptr<SliderChangeListener>> g_sliderListeners;
std::vector<std::unique_ptr<class WindowListener>> g_winListeners;

// Height the window renders at right now: the interpolated value while a
// fold animation is running, otherwise the settled folded/unfolded height.
float currentWinH(const WinState& win) {
    if (win.animActive) return win.animH;
    return win.folded ? kFoldedHeight : win.shownH;
}

void applyWin(WinState& win) {
    if (!win.el) return;
    win.el->SetProperty("left", Rml::CreateString("%dpx", static_cast<int>(win.x + 0.5f)));
    win.el->SetProperty("top", Rml::CreateString("%dpx", static_cast<int>(win.y + 0.5f)));
    win.el->SetProperty("width", Rml::CreateString("%dpx", static_cast<int>(win.w + 0.5f)));
    win.el->SetProperty("height", Rml::CreateString("%dpx", static_cast<int>(
        currentWinH(win) + 0.5f)));
}

// Begin (or reverse mid-flight) the fold/unfold animation towards the
// window's current folded target. The start height is passed in by the
// caller, which must capture it BEFORE flipping the folded flag (otherwise
// the start would equal the target). The "folding" class keeps the body
// visible and scroll-locked for the duration.
void startWinAnim(WinState& win, float from) {
    if (!win.el) return;
    const float to = win.folded ? kFoldedHeight : win.shownH;
    if (std::fabs(to - from) < 0.5f) {
        win.animActive = false;
        win.el->SetClass("folding", false);
        win.el->SetClass("folded", win.folded);
        applyWin(win);
        return;
    }
    win.animFrom = from;
    win.animTo = to;
    win.animH = from;
    win.animStart = GetTickCount();
    win.animActive = true;
    win.el->SetClass("folding", true);
    applyWin(win);
}

// Advance every running fold animation. GetTickCount wraps after 49 days;
// the unsigned subtraction still measures short deltas correctly across the
// wrap.
void stepWindowAnims() {
    if (!g_menuShown) {
        for (WinState& win : g_win) {
            if (win.animActive) {
                win.animActive = false;
                if (win.el) win.el->SetClass("folding", false);
            }
        }
        return;
    }
    bool any = false;
    for (WinState& win : g_win)
        if (win.animActive) any = true;
    if (!any) return;
    const DWORD now = GetTickCount();
    for (WinState& win : g_win) {
        if (!win.animActive || !win.el) continue;
        const float t = clamp01(static_cast<float>(now - win.animStart) /
                                static_cast<float>(kFoldAnimMs));
        const float eased = t * (2.0f - t);  // ease-out quad
        win.animH = win.animFrom + (win.animTo - win.animFrom) * eased;
        if (t >= 1.0f) {
            win.animActive = false;
            win.el->SetClass("folding", false);
        }
        applyWin(win);
    }
}

void focusWin(WinState& win) {
    for (WinState& other : g_win)
        if (other.el) other.el->SetClass("focus", &other == &win);
    win.el->SetProperty("z-index", Rml::CreateString("%d", ++g_zTop));
}

// Tile the windows from the shared, harness-verified layout math. force
// re-tilts everything (initial layout, "reset layout" button); otherwise
// windows the user dragged keep their position and are only clamped back
// into the viewport. While the 3D preview is on, a strip on the right is
// reserved for the compat layer's preview window.
void layoutWindows(float viewW, float viewH, bool force) {
    // The document body carries the dim backdrop; it only paints over the
    // area its box covers, and with nothing but absolutely-positioned
    // children that box would be zero-sized. Size it to the viewport.
    if (Rml::Element* body = g_menuDoc->GetElementById("doc-body")) {
        body->SetProperty("width", Rml::CreateString("%dpx", static_cast<int>(viewW + 0.5f)));
        body->SetProperty("height", Rml::CreateString("%dpx", static_cast<int>(viewH + 0.5f)));
    }
    const float inset = g_previewOn.load(std::memory_order_relaxed) &&
        viewW > kPreviewInsetPx * 2.0f ? kPreviewInsetPx : 0.0f;
    float natW[kWindowCount];
    float natH[kWindowCount];
    menu_layout::WindowRect rects[kWindowCount];
    for (int i = 0; i < kWindowCount; ++i) {
        natW[i] = g_win[i].w;
        natH[i] = g_win[i].h;
    }
    menu_layout::computeMenuWindows(natW, natH, kWindowCount, viewW, viewH, rects, inset);
    for (int i = 0; i < kWindowCount; ++i) {
        WinState& win = g_win[i];
        if (!win.el) continue;
        // Re-tiling retargets the geometry; a running fold animation would
        // chase a stale height, so animations are settled here.
        if (win.animActive) {
            win.animActive = false;
            win.el->SetClass("folding", false);
        }
        if (force || !win.dragged) {
            if (force) win.dragged = false;
            win.x = rects[i].x;
            win.y = rects[i].y;
        }
        win.shownH = rects[i].h;
        menu_layout::WindowRect clamped{win.x, win.y, win.w,
                                        win.folded ? kFoldedHeight : win.shownH};
        menu_layout::clampDraggedWindow(clamped, viewW, viewH);
        win.x = clamped.x;
        win.y = clamped.y;
        applyWin(win);
    }
}

// Keep every window where the user left it, only pushing it back inside the
// viewport (and out of the preview strip when the preview is on). Used when
// the 3D preview toggles, so switching it never visibly re-arranges the
// windows.
void clampWindowsToViewport() {
    const float viewW = static_cast<float>(g_width);
    const float viewH = static_cast<float>(g_height);
    const float inset = g_previewOn.load(std::memory_order_relaxed) &&
        viewW > kPreviewInsetPx * 2.0f ? kPreviewInsetPx : 0.0f;
    for (WinState& win : g_win) {
        if (!win.el) continue;
        if (win.animActive) {
            win.animActive = false;
            win.el->SetClass("folding", false);
        }
        menu_layout::WindowRect clamped{win.x, win.y, win.w, currentWinH(win)};
        menu_layout::clampDraggedWindow(clamped, viewW - inset, viewH);
        win.x = clamped.x;
        win.y = clamped.y;
        applyWin(win);
    }
}

// Title-bar drag + focus. Attached to the window (mousedown: focus, bring to
// front) and to the title bar (dragstart/drag: move, clamped so the bar can
// never leave the screen).
class WindowListener : public Rml::EventListener {
public:
    explicit WindowListener(WinState* win) : m_win(win) {}

    void ProcessEvent(Rml::Event& event) override {
        if (!m_win || !m_win->el) return;
        const Rml::EventId id = event.GetId();
        if (id == Rml::EventId::Mousedown) {
            focusWin(*m_win);
            return;
        }
        if (id == Rml::EventId::Dragstart) {
            focusWin(*m_win);
            m_win->grabDx = event.GetParameter<float>("mouse_x", 0.0f) - m_win->x;
            m_win->grabDy = event.GetParameter<float>("mouse_y", 0.0f) - m_win->y;
            return;
        }
        if (id == Rml::EventId::Drag) {
            m_win->x = event.GetParameter<float>("mouse_x", 0.0f) - m_win->grabDx;
            m_win->y = event.GetParameter<float>("mouse_y", 0.0f) - m_win->grabDy;
            m_win->dragged = true;
            menu_layout::WindowRect clamped{m_win->x, m_win->y, m_win->w,
                                            currentWinH(*m_win)};
            menu_layout::clampDraggedWindow(clamped, static_cast<float>(g_width),
                                            static_cast<float>(g_height));
            m_win->x = clamped.x;
            m_win->y = clamped.y;
            applyWin(*m_win);
        }
    }

private:
    WinState* m_win;
};

void MenuClickListener::ProcessEvent(Rml::Event& event) {
    // Walk up from the click target to the nearest element with an id: clicks
    // land on the check mark span, the fold glyph spans, etc.
    Rml::Element* element = event.GetTargetElement();
    while (element && element->GetId().empty()) element = element->GetParentNode();
    if (!element || !g_menuDoc) return;
    const Rml::String id = element->GetId();

    if (id == "btn-close") {
        g_hiddenByClose.store(true, std::memory_order_relaxed);
        return;
    }
    if (id == "btn-hudeditor") {
        g_hudEditorRequest.store(true, std::memory_order_relaxed);
        g_hiddenByClose.store(true, std::memory_order_relaxed);
        return;
    }
    if (id == "sw-preview") {
        const bool on = !g_previewOn.load(std::memory_order_relaxed);
        g_previewOn.store(on, std::memory_order_relaxed);
        if (g_previewSwitch) g_previewSwitch->SetClass("on", on);
        // v7: toggling the preview keeps every window where it is (the user
        // complained the windows "jump together" on re-tiling); only clamp
        // them away from the reserved strip / screen edges.
        clampWindowsToViewport();
        return;
    }
    if (id.size() >= 4 && id.compare(0, 4, "sec-") == 0) {
        // Only the section head toggles the fold. Clicks inside the body
        // (slider labels, rows, padding) used to walk up to the section and
        // collapse it mid-interaction, which made the sliders feel dead.
        Rml::Element* target = event.GetTargetElement();
        bool inHead = false;
        for (Rml::Element* e = target; e; e = e->GetParentNode())
            if (e->IsClassSet("sec-head")) { inHead = true; break; }
        if (inHead) {
            if (Rml::Element* section = g_menuDoc->GetElementById(id)) {
                const bool folded = !section->IsClassSet("folded");
                section->SetClass("folded", folded);
            }
        }
        return;
    }
    if (id.size() == 6 && id.compare(0, 5, "fold-") == 0 && id[5] >= '0' && id[5] <= '9') {
        const int index = id[5] - '0';
        if (index < kWindowCount && g_win[index].el) {
            WinState& win = g_win[index];
            const float fromH = currentWinH(win);
            win.folded = !win.folded;
            win.el->SetClass("folded", win.folded);
            focusWin(win);
            startWinAnim(win, fromH);
        }
        return;
    }
    if (id == "btn-layout") {
        layoutWindows(static_cast<float>(g_width), static_cast<float>(g_height), true);
        return;
    }
    if (const SwitchDef* def = findSwitch(id)) {
        applySwitch(*def, !readSwitch(*def));
        return;
    }
}

// ---------------------------------------------------------------------------
// Fonts and documents.
// ---------------------------------------------------------------------------

bool loadFontBuffer(const char* path, std::vector<char>& out) {
    std::FILE* file = std::fopen(path, "rb");
    if (!file) return false;
    std::fseek(file, 0, SEEK_END);
    const long size = std::ftell(file);
    std::fseek(file, 0, SEEK_SET);
    if (size <= 0) {
        std::fclose(file);
        return false;
    }
    out.resize(static_cast<std::size_t>(size));
    const std::size_t read = out.empty() ? 0 : std::fread(out.data(), 1, out.size(), file);
    std::fclose(file);
    return read == out.size();
}

// LoadDocumentFromMemory does not fetch external style sheets, so the RCSS is
// injected into the document's empty <style> element before parsing.
Rml::ElementDocument* loadDocumentWithStyle(Rml::Context* context, const char* rml,
                                            const char* rcss, const char* sourceName) {
    std::string text(rml);
    const std::size_t start = text.find("<style>");
    if (start != std::string::npos) {
        const std::size_t end = text.find("</style>", start);
        if (end != std::string::npos) {
            text.insert(start + 7, "\n");
            text.insert(start + 8, rcss);
            text.insert(start + 8 + std::strlen(rcss), "\n");
        }
    }
    Rml::ElementDocument* document = context->LoadDocumentFromMemory(Rml::String(text), sourceName);
    if (!document) return nullptr;
    document->Hide();
    // The context owns the document (it is a child of the context root), so
    // the returned pointer stays valid for the context's lifetime.
    return document;
}

bool buildRegistry() {
    if (!g_menuDoc) return false;
    g_ttStatus = g_menuDoc->GetElementById("tt-status");
    g_ttDiag = g_menuDoc->GetElementById("tt-diag");
    g_previewSwitch = g_menuDoc->GetElementById("sw-preview");
    if (!g_ttStatus || !g_ttDiag || !g_previewSwitch) return false;
    g_previewSwitch->SetClass("on", g_previewOn.load(std::memory_order_relaxed));

    g_menuDoc->AddEventListener("click", &g_clickListener);

    // Windows: geometry is applied by layoutWindows() right after init; the
    // listeners carry focus, z-order and title-bar dragging. Natural sizes
    // carry the UI scale so the RCSS zoom and the layout math agree.
    kFoldedHeight = kFoldedHeightRef * g_uiScale;
    for (int i = 0; i < kWindowCount; ++i) {
        WinState& win = g_win[i];
        win.el = g_menuDoc->GetElementById(kWindows[i].id);
        win.head = g_menuDoc->GetElementById(kWindows[i].headId);
        if (!win.el || !win.head) return false;
        win.w = kWindows[i].w * g_uiScale;
        win.h = kWindows[i].h * g_uiScale;
        win.shownH = win.h;
        win.el->SetProperty("z-index", Rml::CreateString("%d", 10 + i));
        g_winListeners.push_back(std::make_unique<WindowListener>(&win));
        WindowListener* listener = g_winListeners.back().get();
        win.el->AddEventListener("mousedown", listener);
        win.head->AddEventListener("dragstart", listener);
        win.head->AddEventListener("drag", listener);
    }

    for (const SwitchDef& def : kSwitches) {
        Rml::Element* element = g_menuDoc->GetElementById(def.id);
        if (!element) continue;
        SwitchElem item;
        item.el = element;
        item.def = &def;
        g_switchElems.push_back(item);
    }

    for (const SliderDef& def : kSliders) {
        Rml::Element* input = g_menuDoc->GetElementById(def.id);
        if (!input) continue;
        Rml::Element* readout = g_menuDoc->GetElementById(readoutIdFrom(def.id));
        SliderElem item;
        item.input = input;
        item.readout = readout;
        item.def = &def;
        g_sliderElems.push_back(item);
        g_sliderListeners.push_back(std::make_unique<SliderChangeListener>(&def));
        input->AddEventListener("change", g_sliderListeners.back().get());
    }
    return !g_switchElems.empty() && !g_sliderElems.empty();
}

bool initBackend(ID3D11Device* device, unsigned width, unsigned height) {
    menuLog("init: begin, viewport %ux%u", width, height);
    g_uiScale = computeUiScale(width, height);
    menuLog("init: ui scale %.3f", g_uiScale);
    // Segoe UI covers Cyrillic and is present on every Windows install; the
    // buffers must outlive Rml::Shutdown, so they are file-scope vectors.
    if (!loadFontBuffer("C:/Windows/Fonts/segoeui.ttf", g_fontNormal)) {
        menuLog("init: FAILED, C:/Windows/Fonts/segoeui.ttf unreadable");
        return false;
    }
    if (!loadFontBuffer("C:/Windows/Fonts/segoeuib.ttf", g_fontBold)) {
        menuLog("init: FAILED, C:/Windows/Fonts/segoeuib.ttf unreadable");
        return false;
    }
    menuLog("init: fonts read, %zu + %zu bytes", g_fontNormal.size(), g_fontBold.size());

    g_renderer = new RenderInterface_DX11(device);
    if (!g_renderer) return false;
    Rml::SetRenderInterface(g_renderer);
    // The custom system interface only routes RmlUi log messages into
    // gayclient-menu.log; everything else (elapsed time) stays default.
    Rml::SetSystemInterface(&g_systemInterface);
    // The renderer's internal viewport defaults to zero; without this call
    // BeginFrame would build 0x0 render targets and nothing would show.
    g_renderer->SetViewport(static_cast<int>(width), static_cast<int>(height));
    if (!Rml::Initialise()) {
        menuLog("init: FAILED, Rml::Initialise returned false");
        return false;
    }
    menuLog("init: Rml::Initialise ok");
    g_rmlContext = Rml::CreateContext("gayclient", Rml::Vector2i(static_cast<int>(width),
                                                                 static_cast<int>(height)));
    if (!g_rmlContext) {
        menuLog("init: FAILED, CreateContext returned null");
        return false;
    }
    menuLog("init: context created");

    if (!Rml::LoadFontFace(Rml::Span<const Rml::byte>(
            reinterpret_cast<const Rml::byte*>(g_fontNormal.data()), g_fontNormal.size()),
            "GAY", Rml::Style::FontStyle::Normal, Rml::Style::FontWeight::Auto, true)) {
        menuLog("init: FAILED, LoadFontFace (regular)");
        return false;
    }
    if (!Rml::LoadFontFace(Rml::Span<const Rml::byte>(
            reinterpret_cast<const Rml::byte*>(g_fontBold.data()), g_fontBold.size()),
            "GAY", Rml::Style::FontStyle::Normal, Rml::Style::FontWeight::Bold, true)) {
        menuLog("init: FAILED, LoadFontFace (bold)");
        return false;
    }
    menuLog("init: font faces loaded");

    const std::string scaledRcss = scaleRcssPx(kMenuRcss, g_uiScale);
    g_menuDoc = loadDocumentWithStyle(g_rmlContext, kMenuRml, scaledRcss.c_str(), "gayclient-menu.rml");
    if (!g_menuDoc) {
        menuLog("init: FAILED, menu document did not parse");
        return false;
    }
    menuLog("init: menu document parsed");
    if (!buildRegistry()) {
        menuLog("init: FAILED, element registry incomplete (%zu switches, %zu sliders)",
                g_switchElems.size(), g_sliderElems.size());
        return false;
    }
    // Start faded: the very first Show() then fades the menu in instead of
    // popping.
    if (Rml::Element* body = g_menuDoc->GetElementById("doc-body"))
        body->SetClass("faded", true);
    g_width = width;
    g_height = height;
    menuLog("init: complete, %d windows, %zu switches, %zu sliders",
            kWindowCount, g_switchElems.size(), g_sliderElems.size());
    return true;
}


// ---------------------------------------------------------------------------
// Per-frame synchronization and input.
// ---------------------------------------------------------------------------

void syncSwitches() {
    syncModuleShadow();
    for (const SwitchElem& item : g_switchElems) {
        if (!item.el || !item.def) continue;
        const bool on = readSwitch(*item.def);
        if (item.el->IsClassSet("on") != on) item.el->SetClass("on", on);
    }
}

void syncSliders() {
    for (SliderElem& item : g_sliderElems) {
        if (!item.input || !item.def) continue;
        const float dom = domFromParam(*item.def, GetVisualParam(item.def->param));
        // Only write when the value actually moved, so the DOM never fights
        // an in-progress drag; writes are stable round-trips by construction.
        const Rml::Variant* current = item.input->GetAttribute("value");
        if (!current || std::fabs(current->Get<float>() - dom) > 0.0005f)
            item.input->SetAttribute("value", dom);
        if (item.readout) {
            char text[32];
            std::snprintf(text, sizeof(text), item.def->fmt, dom);
            if (item.readoutCache != text) {
                item.readoutCache = text;
                item.readout->SetInnerRML(text);
            }
        }
    }
    // Color chips follow the live RGB parameters.
    struct ChipDef { const char* id; VisualParam r, g, b; };
    static const ChipDef kChips[] = {
        {"chip-ch", VisualParam::HatRed, VisualParam::HatGreen, VisualParam::HatBlue},
        {"chip-tr", VisualParam::TrailRed, VisualParam::TrailGreen, VisualParam::TrailBlue},
        {"chip-jr", VisualParam::JumpRed, VisualParam::JumpGreen, VisualParam::JumpBlue},
    };
    for (const ChipDef& chip : kChips) {
        Rml::Element* element = g_menuDoc->GetElementById(chip.id);
        if (!element) continue;
        const int r = static_cast<int>(clamp01(GetVisualParam(chip.r)) * 255.0f + 0.5f);
        const int g = static_cast<int>(clamp01(GetVisualParam(chip.g)) * 255.0f + 0.5f);
        const int b = static_cast<int>(clamp01(GetVisualParam(chip.b)) * 255.0f + 0.5f);
        element->SetProperty("background-color", Rml::CreateString("rgb(%d, %d, %d)", r, g, b));
    }
}

const char* katanaStatusText(int status) {
    switch (status) {
        case 1: return "Katana: рисуется (world pass)";
        case 2: return "Katana: отрисовка недоступна; оставлен оригинал";
        case 9: return "Katana: локальный игрок не найден";
        case 10: return "Katana: меч не сопоставлен";
        case 11: return "Katana: кандидат меча";
        case 12: return "Katana: третья персона ещё не подтверждена";
        case 13: return "Katana: поворот нечитаем; оставлен оригинал";
        case 14: return "Katana: виден; заменяю меч";
        case 15: return "Katana: модель недействительна";
        default: return "Katana: ожидание алмазного меча в руке";
    }
}

void syncTtStatus() {
    if (!g_menuShown || !g_ttStatus || !g_ttDiag) return;
    const std::string status = katanaStatusText(KatanaStatus());
    if (status != g_ttStatusCache) {
        g_ttStatusCache = status;
        g_ttStatus->SetInnerRML(status);
    }
    std::string diag;
    if (KatanaEnabled()) {
        const KatanaDiagnostics d = KatanaDebugSnapshot();
        char line[256];
        std::snprintf(line, sizeof(line),
            "P:%u A:%u B:%u H:%u%s yaw %+.2f pitch %+.2f",
            d.sources & 1 ? 1u : 0u, d.sources & 2 ? 1u : 0u,
            d.sources & 4 ? 1u : 0u, d.sources & 8 ? 1u : 0u,
            d.wrapped ? " (wrap)" : "", d.yaw, d.pitchUp);
        diag = line;
        std::snprintf(line, sizeof(line), "<br/>F:%u V:%u S:%u C:%u R:%u X:%u D:%u O:%u P:%u",
            KatanaEvidence() ? 1u : 0u, !!(KatanaEvidence() & 2), !!(KatanaEvidence() & 16),
            static_cast<unsigned>((!!(KatanaEvidence() & 32)) | (!!(KatanaEvidence() & 64) << 1)),
            !!(KatanaEvidence() & 256), !!(KatanaEvidence() & 512), !!(KatanaEvidence() & 128),
            !!(KatanaEvidence() & 2048),
            static_cast<unsigned>((!!(KatanaEvidence() & 4096)) | (!!(KatanaEvidence() & 8192) << 1)));
        diag += line;
        std::snprintf(line, sizeof(line), "<br/>CI:%llX PL:%llX H58:%llX",
            d.clientInstance, d.player, d.rotationHolder);
        diag += line;
        std::snprintf(line, sizeof(line), "<br/>TP in:%d out:%d tex:%llX learned:%llX",
            d.tpCallsInPass, d.tpCallsOutOfPass, d.tpLastTexture, d.learnedTexture);
        diag += line;
    } else {
        diag = "Katana выключен";
    }
    if (diag != g_ttDiagCache) {
        g_ttDiagCache = diag;
        g_ttDiag->SetInnerRML(diag);
    }
}

void feedInput(unsigned width, unsigned height) {
    if (g_pointerValid.load(std::memory_order_relaxed)) {
        const float w = g_pointerW.load(std::memory_order_relaxed);
        const float h = g_pointerH.load(std::memory_order_relaxed);
        if (w > 1.0f && h > 1.0f) {
            const float mx = g_pointerX.load(std::memory_order_relaxed) *
                (static_cast<float>(width) / w);
            const float my = g_pointerY.load(std::memory_order_relaxed) *
                (static_cast<float>(height) / h);
            g_rmlContext->ProcessMouseMove(static_cast<int>(mx), static_cast<int>(my), 0);
        }
    }
    // RmlUi processes every call as a fresh event, so buttons and keys are
    // forwarded on state transitions only (mirroring the window event model).
    for (int button = 0; button < 2; ++button) {
        const bool down = keyIsDown(button == 0 ? VK_LBUTTON : VK_RBUTTON);
        if (down == g_mouseStates[button]) continue;
        g_mouseStates[button] = down;
        if (down) g_rmlContext->ProcessMouseButtonDown(button, 0);
        else g_rmlContext->ProcessMouseButtonUp(button, 0);
    }
    const float wheelX = g_wheelX.exchange(0.0f, std::memory_order_relaxed);
    const float wheelY = g_wheelY.exchange(0.0f, std::memory_order_relaxed);
    // compat sends one unit per wheel notch (positive = wheel rolled
    // forward/up, the Windows convention); RmlUi expects positive deltas to
    // scroll toward the content bottom, so the sign is flipped here.
    if (wheelX != 0.0f || wheelY != 0.0f)
        g_rmlContext->ProcessMouseWheel(Rml::Vector2f(-wheelX, -wheelY), 0);

    const bool escape = keyIsDown(VK_ESCAPE);
    if (escape != g_escState) {
        g_escState = escape;
        if (escape) g_rmlContext->ProcessKeyDown(Rml::Input::KeyIdentifier::KI_ESCAPE, 0);
        else g_rmlContext->ProcessKeyUp(Rml::Input::KeyIdentifier::KI_ESCAPE, 0);
    }
}

void updateDocVisibility(bool menuVisible) {
    if (menuVisible != g_menuShown) {
        g_menuShown = menuVisible;
        // The fade runs through the body.faded class: class flips start CSS
        // transitions in RmlUi, inline SetProperty does not.
        Rml::Element* body = g_menuDoc->GetElementById("doc-body");
        if (menuVisible) {
            g_fadeOutFrames = 0;
            g_menuDoc->Show();
            if (body) body->SetClass("faded", false);
        } else {
            if (body) body->SetClass("faded", true);
            g_fadeOutFrames = kFadeOutFrames;
        }
    }
    if (!g_menuShown && g_fadeOutFrames > 0) {
        --g_fadeOutFrames;
        if (g_fadeOutFrames == 0) g_menuDoc->Hide();
    }
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

bool ReplacesLegacyMenu() { return g_renderOk.load(std::memory_order_relaxed); }
bool ConsumeHudEditorRequest() { return g_hudEditorRequest.exchange(false, std::memory_order_relaxed); }
bool VisualPreviewActive() { return g_previewOn.load(std::memory_order_relaxed); }
// True when the user closed the menu with the X button (or entered the HUD
// editor) while the client still considers its menu open. The compat layer
// uses this to clear the client's menu flag, otherwise the game stays in
// menu mode with input suppressed and its own screens misbehave.
bool IsHiddenByClose() { return g_hiddenByClose.load(std::memory_order_relaxed); }

bool ReadPointer(float& x, float& y, float& w, float& h) {
    if (!g_pointerValid.load(std::memory_order_relaxed)) return false;
    x = g_pointerX.load(std::memory_order_relaxed);
    y = g_pointerY.load(std::memory_order_relaxed);
    w = g_pointerW.load(std::memory_order_relaxed);
    h = g_pointerH.load(std::memory_order_relaxed);
    return w > 1.0f && h > 1.0f;
}

void RenderOverlay(ID3D11Device* device, ID3D11RenderTargetView* rtv,
                   unsigned width, unsigned height) {
    if (g_initFailed || !device || !rtv || width < 200 || height < 200) return;
    if (!g_ready) {
        if (!initBackend(device, width, height)) {
            g_initFailed = true;
            return;
        }
        g_ready = true;
        layoutWindows(static_cast<float>(width), static_cast<float>(height), true);
    }
    if (width != g_width || height != g_height) {
        g_width = width;
        g_height = height;
        g_rmlContext->SetDimensions(Rml::Vector2i(static_cast<int>(width),
                                                  static_cast<int>(height)));
        g_renderer->SetViewport(static_cast<int>(width), static_cast<int>(height));
        layoutWindows(static_cast<float>(width), static_cast<float>(height), false);
    }

    const bool menuVisible = g_gameMenuOpen.load(std::memory_order_relaxed) &&
        !g_hiddenByClose.load(std::memory_order_relaxed);
    updateDocVisibility(menuVisible);
    // A few frames after closing, the document still renders so the fade-out
    // transition is seen, but input is cut immediately.
    if (!menuVisible && g_fadeOutFrames == 0) return;

    if (menuVisible) {
        feedInput(width, height);
        syncSwitches();
        syncSliders();
        syncTtStatus();
    }
    stepWindowAnims();
    g_rmlContext->Update();
    g_renderer->BeginFrame();
    g_rmlContext->Render();
    // EndFrame binds the back-buffer render target view, resolves the frame
    // and restores the DirectX state it changed, so the game's own Present
    // path is untouched.
    g_renderer->EndFrame(rtv);
    if (!g_renderOk.exchange(true, std::memory_order_relaxed))
        menuLog("render: first frame complete, new menu is now live");
}

} // namespace menu_rml
