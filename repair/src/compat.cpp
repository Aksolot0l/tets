// Experimental compatibility bootstrap for the exact legacy DLL documented in README.md.
// The legacy library contains the UI and MinHook but does not start its hooks or
// initialize its ImGui renderer. This adapter must be loaded as GAYClientInternal.dll
// alongside the original renamed to GAYClientCore.dll. Windows x64 only.
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <d3d11.h>
#include <dxgi1_2.h>
#include <cstdio>
#include "imgui.h"
#include "settings.h"
#include "hud_module_list.h"
#include "hud_theme.h"
#include "hud_visual_kit.h"
#include "pointer_mapping.h"
#include "uwp_extra.h"
#include "page_scroll.h"
#include "wheel_bridge.h"
#include "wheel_counter.h"
#include "coordinates_115.h"
#include "mouse_diagnostic_patch.h"
#include "disable_hotkey.h"
#include "china_hat_native.h"
#include "menu_mouse_native.h"
#include "visual_avatar_preview.h"
#include "visual_color.h"
#include "visual_player_skin.h"
#include "visual_player_replay.h"
#include <atomic>
#include <array>
#include <cstring>

static_assert(offsetof(ag::Settings, uiScale) == 192, "legacy Settings ABI changed");
static_assert(sizeof(std::array<bool, 256>) == 256, "legacy watched key ABI changed");

using PresentFn = HRESULT (STDMETHODCALLTYPE*)(IDXGISwapChain*, UINT, UINT);
using Present1Fn = HRESULT (STDMETHODCALLTYPE*)(IDXGISwapChain1*, UINT, UINT, const DXGI_PRESENT_PARAMETERS*);
using InitFn = int (WINAPI*)();
using HookFn = int (WINAPI*)(void*, void*, void**);
using ToggleFn = int (WINAPI*)(void*);
using LegacyPresentFn = HRESULT (*)(IDXGISwapChain*, UINT, UINT);
using NewFrameFn = void (*)();
using RendererInitFn = bool (*)(ID3D11Device*, ID3D11DeviceContext*);
using GetIOFn = ImGuiIO& (*)();
using UpdateFn = void (*)(void*, const void*, bool, bool);
using BridgeInitFn = void (*)();
using BridgeActiveFn = void (*)(bool);
using AddFontFn = ImFont* (*)(float, bool);
using PointerFn = bool (*)(float&, float&, float&, float&);
using FindWindowFn = HWND (*)(bool);
using SettingsCtorFn = void (*)(ag::Settings*);
using TickFn = HRESULT (*)(void*);
using AddWheelFn = void (*)(ImGuiIO*, float, float);
using WatchedKeysFn = void (*)(const std::array<bool, 256>&);
using KeyDownFn = bool (*)(int);
using TextFn = void (*)(const char*, const char*);
using DrawGuiFn = void (*)(void*);
using SetModuleFn = void (*)(void*, ag::ModuleId, bool);
using BeginFn = bool (*)(const char*, bool*, int);
using EndFn = void (*)();
using CheckboxFn = bool (*)(const char*, bool*);
using SliderFloatFn = bool (*)(const char*, float*, float, float, const char*, int);
using SetNextPosFn = void (*)(const ImVec2&, int, const ImVec2&);
using SetNextSizeFn = void (*)(const ImVec2&, int);
using BeginTabBarFn = bool (*)(const char*, int);
using BeginTabItemFn = bool (*)(const char*, bool*, int);
using EndTabFn = void (*)();
using ColorEdit3Fn = bool (*)(const char*, float*, int);
using PushColorFn = void (*)(int, const ImVec4&);
using PopColorFn = void (*)(int);
using FontScaleFn = void (*)(float);
using DrawHudFn = void (*)(void*);
using NotificationDrawFn = void (*)(void*, const ag::Settings&);
using ButtonFn = bool (*)(const char*,const ImVec2&);
using PushWidthFn = void (*)(float);
using PopWidthFn = void (*)();
using GetDrawListFn = ImDrawList* (*)();
using GetCursorFn = ImVec2 (*)();
using DummyFn = void (*)(const ImVec2&);
using DrawLineFn = void (*)(ImDrawList*,const ImVec2&,const ImVec2&,ImU32,float);
using DrawTriangleFn = void (*)(ImDrawList*,const ImVec2&,const ImVec2&,const ImVec2&,ImU32);
using DrawQuadFn = void (*)(ImDrawList*,const ImVec2&,const ImVec2&,const ImVec2&,const ImVec2&,ImU32);
using DrawRectFn = void (*)(ImDrawList*,const ImVec2&,const ImVec2&,ImU32,float,int);
using DrawGradientRectFn = void (*)(ImDrawList*,const ImVec2&,const ImVec2&,ImU32,ImU32,ImU32,ImU32);
using DrawImageFn = void (*)(ImDrawList*,ImTextureID,const ImVec2&,const ImVec2&,const ImVec2&,const ImVec2&,ImU32);
using DrawImageQuadFn = void (*)(ImDrawList*,ImTextureID,const ImVec2&,const ImVec2&,const ImVec2&,const ImVec2&,const ImVec2&,const ImVec2&,const ImVec2&,const ImVec2&,ImU32);
using PushVarFloatFn = void (*)(int,float);
using PopVarFn = void (*)(int);
using SameLineFn = void (*)(float,float);
using PushTextWrapFn = void (*)(float);
using PopTextWrapFn = void (*)();

static HMODULE selfModule = nullptr;
static HMODULE core = nullptr;
static PresentFn realPresent = nullptr;
static Present1Fn realPresent1 = nullptr;
static LegacyPresentFn legacyPresent = nullptr;
static NewFrameFn realNewFrame = nullptr;
static RendererInitFn rendererInit = nullptr;
static GetIOFn getIO = nullptr;
static UpdateFn realUpdate = nullptr;
static PointerFn realPointer = nullptr;
static FindWindowFn findGameWindow = nullptr;
static SettingsCtorFn realSettingsCtor = nullptr;
static AddFontFn addFont = nullptr;
static TickFn realTick = nullptr;
static AddWheelFn addWheel = nullptr;
static WatchedKeysFn realWatchedKeys = nullptr;
static KeyDownFn keyDown = nullptr;
static bool pageKeysReady = false;
static BridgeActiveFn bridgeActive = nullptr;
static std::atomic<bool> overlayDisabled{false};
static DisableHotkey disableHotkey;
static PageScrollRepeat pageScroll;
static TextFn realText = nullptr;
static DrawGuiFn realDrawGui = nullptr;
static SetModuleFn setModule = nullptr;
static BeginFn guiBegin = nullptr, realGuiBegin = nullptr;
static EndFn guiEnd = nullptr;
static CheckboxFn guiCheckbox = nullptr;
static SliderFloatFn guiSliderFloat = nullptr;
static SetNextPosFn guiSetNextPos = nullptr;
static SetNextSizeFn guiSetNextSize = nullptr;
static BeginTabBarFn guiBeginTabBar = nullptr;
static BeginTabItemFn guiBeginTabItem = nullptr;
static EndTabFn guiEndTabBar = nullptr, guiEndTabItem = nullptr;
static ColorEdit3Fn guiColorEdit3 = nullptr;
static PushColorFn guiPushColor = nullptr;
static PopColorFn guiPopColor = nullptr;
static FontScaleFn guiFontScale = nullptr;
static DrawHudFn realDrawHud = nullptr;
static NotificationDrawFn realNotificationDraw = nullptr;
static ButtonFn guiButton = nullptr;
static PushWidthFn guiPushWidth = nullptr;
static PopWidthFn guiPopWidth = nullptr;
static GetDrawListFn guiDrawList = nullptr;
static GetCursorFn guiCursor = nullptr;
static DummyFn guiDummy = nullptr;
static DrawLineFn drawLine = nullptr;
static DrawTriangleFn drawTriangle = nullptr;
static DrawQuadFn drawQuad = nullptr;
static DrawRectFn drawRect = nullptr;
static DrawGradientRectFn drawGradientRect = nullptr;
static DrawImageFn drawImage = nullptr;
static DrawImageQuadFn drawImageQuad = nullptr;
static PushVarFloatFn guiPushVarFloat = nullptr;
static PopVarFn guiPopVar = nullptr;
static SameLineFn guiSameLine = nullptr;
static PushTextWrapFn guiPushTextWrap = nullptr;
static PopTextWrapFn guiPopTextWrap = nullptr;
static std::atomic<bool> hudThemeReady{false};
static thread_local bool insideLegacyHud = false;
static thread_local float legacyOpacity = 1.f;
enum class VisualEditor { None, Hat, Trail, Options };
static std::atomic<VisualEditor> visualEditor{VisualEditor::None};
static std::atomic<bool> hudHooksReady{false}, notificationHookReady{false};
static bool hudEditorOpen = false, moduleHudEnabled = false;
static std::atomic<bool> hudEditorActive{false};
// New Module List HUD settings are per-session; they do not overwrite profiles.
static float hudScale = 1.f, hudOpacity = .78f, hudX = .78f, hudY = .05f;
static int hudPalette=0;
static float hudRounding=8.f;
static float widgetOpacity = .72f;
static float hudColor[3] = {77.f/255.f,1.f,136.f/255.f};
static std::atomic<const WheelBridge*> externalWheel{nullptr};
static SRWLOCK externalWheelLock = SRWLOCK_INIT;
static HWND pointerWindow = nullptr;
static thread_local bool insidePresent = false;
static thread_local bool seenInput = false;
static thread_local bool pointerInUse = false;
static thread_local ID3D11Device* frameDevice = nullptr;
static thread_local ID3D11DeviceContext* frameContext = nullptr;
static thread_local UINT frameWidth = 0, frameHeight = 0;
// One SRV reference is retained until the ImGui draw data is submitted.
static thread_local ID3D11ShaderResourceView* previewFrameView = nullptr;
static bool previewGameModelTest=false; // explicitly opt-in; reset on restart
static LARGE_INTEGER lastFrame{};
static LARGE_INTEGER frequency{};

static void log(const char* text) { OutputDebugStringA("GAYClient repair: "); OutputDebugStringA(text); OutputDebugStringA("\n"); }

template<typename T> static T symbol(const char* name) { return reinterpret_cast<T>(GetProcAddress(core, name)); }
template<class T> static void release(T*& p) { if (p) { p->Release(); p = nullptr; } }

// Runs before our Present/ImGui detours are enabled, so no thread can be
// executing drawMarker through this adapter during the code change.
static bool suppressMouseDiagnostic() {
    auto* image = reinterpret_cast<unsigned char*>(core);
    IMAGE_DOS_HEADER dos{}; IMAGE_NT_HEADERS64 nt{};
    SIZE_T count = 0;
    if (!ReadProcessMemory(GetCurrentProcess(), image, &dos, sizeof(dos), &count) || count != sizeof(dos) ||
        dos.e_magic != IMAGE_DOS_SIGNATURE || dos.e_lfanew <= 0 || dos.e_lfanew > 0x10000 ||
        !ReadProcessMemory(GetCurrentProcess(), image + dos.e_lfanew, &nt, sizeof(nt), &count) || count != sizeof(nt) ||
        nt.Signature != IMAGE_NT_SIGNATURE || nt.FileHeader.Machine != IMAGE_FILE_MACHINE_AMD64 ||
        nt.OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR64_MAGIC || nt.OptionalHeader.SizeOfImage != 0x2af000 ||
        reinterpret_cast<unsigned char*>(legacyPresent) != image + 0x1bf0 ||
        !mouse_diagnostic_patch::matches(image, nt.OptionalHeader.SizeOfImage)) return false;
    auto* branch = image + mouse_diagnostic_patch::branchRva;
    DWORD oldProtect = 0;
    if (!VirtualProtect(branch, 6, PAGE_EXECUTE_READWRITE, &oldProtect)) return false;
    mouse_diagnostic_patch::apply(image);
    bool flushed = FlushInstructionCache(GetCurrentProcess(), branch, 6) != 0;
    if (!flushed) {
        // No hooks are active yet. Revert if the CPU cache could not be synced.
        std::memcpy(branch, mouse_diagnostic_patch::code + (mouse_diagnostic_patch::branchRva - mouse_diagnostic_patch::codeRva), 6);
        FlushInstructionCache(GetCurrentProcess(), branch, 6);
    }
    DWORD ignored = 0;
    if (!VirtualProtect(branch, 6, oldProtect, &ignored)) log("mouse diagnostic page protection restore failed");
    return flushed;
}

static bool gameIsForeground() {
    HWND fg = GetForegroundWindow();
    if (!fg) return false;
    DWORD pid = 0;
    GetWindowThreadProcessId(fg, &pid);
    if (pid == GetCurrentProcessId()) return true;
    HWND game = findGameWindow ? findGameWindow(true) : nullptr;
    return game && (game == fg || IsChild(fg, game) || IsChild(game, fg));
}

// Receives an anonymous read-only mapping handle duplicated by our launcher.
// It contains wheel counters only; there is no keyboard or cursor recording.
extern "C" __declspec(dllexport) DWORD WINAPI GAYClientConnectWheel(void* remoteHandle) {
    HANDLE handle = static_cast<HANDLE>(remoteHandle);
    if (!handle) return 0;
    auto* view = static_cast<const WheelBridge*>(MapViewOfFile(handle, FILE_MAP_READ, 0, 0, sizeof(WheelBridge)));
    CloseHandle(handle);
    if (!view) return 0;
    if (view->magic != kWheelMagic || view->version != 1) { UnmapViewOfFile(view); return 0; }
    // Reconnection is supported if the GUI is restarted while the game stays open.
    AcquireSRWLockExclusive(&externalWheelLock);
    const WheelBridge* old = externalWheel.exchange(view, std::memory_order_acq_rel);
    if (old) UnmapViewOfFile(old);
    ReleaseSRWLockExclusive(&externalWheelLock);
    return 1;
}
static bool takeExternalWheel(int& x, int& y) {
    AcquireSRWLockShared(&externalWheelLock);
    const WheelBridge* view = externalWheel.load(std::memory_order_acquire);
    if (!view) { ReleaseSRWLockShared(&externalWheelLock); return false; }
    // The producer uses InterlockedExchangeAdd; aligned 32-bit reads are atomic
    // on Windows x64. Modular differences handle signed counter wraparound.
    static const WheelBridge* lastView = nullptr;
    static WheelCounter counter;
    if (lastView != view) { lastView = view; counter.reset(); }
    counter.take(static_cast<std::uint32_t>(view->horizontal), static_cast<std::uint32_t>(view->vertical), x, y);
    ReleaseSRWLockShared(&externalWheelLock);
    return true;
}

static bool readGameBytes(std::uint64_t address, void* output, size_t length) {
    SIZE_T received = 0;
    return address && ReadProcessMemory(GetCurrentProcess(), reinterpret_cast<const void*>(address),
                                        output, length, &received) && received == length;
}
static bool supportedGame(std::uint64_t& base) {
    HMODULE game = GetModuleHandleW(nullptr);
    if (!game) return false;
    wchar_t path[32768]{};
    DWORD n = GetModuleFileNameW(game, path, 32768);
    if (!n || n >= 32768) return false;
    const wchar_t* name = wcsrchr(path, L'\\');
    if (_wcsicmp(name ? name + 1 : path, L"Minecraft.Windows.exe") != 0) return false;
    base = reinterpret_cast<std::uint64_t>(game);
    IMAGE_DOS_HEADER dos{}; IMAGE_NT_HEADERS64 nt{};
    if (!readGameBytes(base, &dos, sizeof(dos)) || dos.e_magic != IMAGE_DOS_SIGNATURE ||
        dos.e_lfanew <= 0 || dos.e_lfanew > 0x10000 ||
        !readGameBytes(base + dos.e_lfanew, &nt, sizeof(nt))) return false;
    return nt.Signature == IMAGE_NT_SIGNATURE && nt.FileHeader.Machine == IMAGE_FILE_MACHINE_AMD64 &&
           nt.FileHeader.TimeDateStamp == 0x5976da7b && nt.OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC &&
           nt.OptionalHeader.SizeOfImage > 0x019431e0 + sizeof(std::uint64_t);
}
static void patchedText(const char* text, const char* textEnd) {
    constexpr char placeholder[] = "XYZ: unavailable";
    bool isCoords = text && (textEnd ? (textEnd >= text && static_cast<size_t>(textEnd - text) == sizeof(placeholder) - 1 &&
                                          std::memcmp(text, placeholder, sizeof(placeholder) - 1) == 0)
                                     : std::strcmp(text, placeholder) == 0);
    if (isCoords) {
        // Cache briefly, but never display an old position if the read fails.
        static ULONGLONG next = 0;
        static bool supported = false, valid = false;
        static Position115 position{};
        ULONGLONG now = GetTickCount64();
        if (now >= next) {
            next = now + 100;
            std::uint64_t base = 0;
            supported = supportedGame(base);
            valid = supported && readPosition115(base, readGameBytes, position);
        }
        if (valid) {
            char formatted[96]{};
            const auto blocks = displayPosition115(position);
            std::snprintf(formatted, sizeof(formatted), "XYZ (test): %d %d %d", blocks.x, blocks.y, blocks.z);
            realText(formatted, nullptr); return;
        }
        realText(supported ? "XYZ: not ready" : "XYZ: unsupported game build", nullptr);
        return;
    }
    realText(text, textEnd);
}
// This panel uses the same ImGui context and functions exported by the legacy
// core. It is a separate window, not a modification of the closed-source
// original category list. Values are kept only for this game session.
// Labels are placed above full-width controls, so narrow panels do not clip.
static void controlLabel(const char* label) {
    const char* end=std::strstr(label,"##"); realText(label,end);
}
static void visualSlider(const char* label,VisualParam p,const char* fmt) {
    if (!guiSliderFloat) { realText("Slider unavailable",nullptr); return; }
    const auto i=static_cast<std::size_t>(p);
    if (i>=static_cast<std::size_t>(VisualParam::Count)) return;
    float value=GetVisualParam(p); const auto spec=visualParamSpecs[i];
    char id[160]{}; std::snprintf(id,sizeof(id),"##%s",label);
    controlLabel(label);
    if (guiPushWidth && guiPopWidth) guiPushWidth(-1.f);
    if (guiSliderFloat(id,&value,spec.low,spec.high,fmt,0)) SetVisualParam(p,value);
    if (guiPushWidth && guiPopWidth) guiPopWidth();
}
static void visualColor(const char* label,VisualParam r,VisualParam g,VisualParam b) {
    if (!guiColorEdit3) { realText("Color picker unavailable",nullptr); return; }
    float color[3]={GetVisualParam(r),GetVisualParam(g),GetVisualParam(b)};
    char id[160]{}; std::snprintf(id,sizeof(id),"##%s",label); controlLabel(label);
    if (guiPushWidth && guiPopWidth) guiPushWidth(-1.f);
    if (guiColorEdit3(id,color,0)) {
        SetVisualParam(r,color[0]); SetVisualParam(g,color[1]); SetVisualParam(b,color[2]);
    }
    if (guiPushWidth && guiPopWidth) guiPopWidth();
}
static void drawHatSettings() {
    bool enabled=ChinaHatEnabled();
    if (guiCheckbox("China Hat (test)",&enabled)) SetChinaHatEnabled(enabled);
    bool fill=HatFillEnabled();
    if (guiCheckbox("Filled hat (test)",&fill)) SetHatFillEnabled(fill);
    bool rainbow=HatRainbowEnabled();
    if (guiCheckbox("Rainbow hat",&rainbow)) SetHatRainbowEnabled(rainbow);
    if (rainbow) visualSlider("Rainbow speed##hat",VisualParam::HatRainbowSpeed,"%.2f");
    else visualColor("Hat color",VisualParam::HatRed,VisualParam::HatGreen,VisualParam::HatBlue);
    visualSlider("Opacity (test)##hat",VisualParam::HatOpacity,"%.2f");
    visualSlider("Radius",VisualParam::HatRadius,"%.2f");
    visualSlider("Cone height",VisualParam::HatConeHeight,"%.2f");
    visualSlider("Hat height",VisualParam::HatOffset,"%.2f");
    visualSlider("Float amount",VisualParam::HatBobAmplitude,"%.2f");
    visualSlider("Float speed",VisualParam::HatBobSpeed,"%.2f");
    if (ChinaHatEnabled()) {
        const int status=ChinaHatStatus();
        realText(status==1 ? "Hat: submitted" : status==2 ? "Hat: renderer not ready" : "Hat: waiting for player",nullptr);
        if (fill) { const int fs=HatFillStatus();
            realText(fs==1?"Fill: submitted (visibility unverified)":fs==2?"Fill: renderer busy":"Fill: waiting",nullptr);
        }
    }
}
static void drawTrailSettings() {
    bool enabled=TrailEnabled();
    if (guiCheckbox("Trail (test)",&enabled)) SetTrailEnabled(enabled);
    bool fill=TrailFillEnabled();
    if (guiCheckbox("Filled trail (test)",&fill)) SetTrailFillEnabled(fill);
    bool rainbow=TrailRainbowEnabled();
    if (guiCheckbox("Rainbow trail",&rainbow)) SetTrailRainbowEnabled(rainbow);
    if (rainbow) visualSlider("Rainbow speed##trail",VisualParam::TrailRainbowSpeed,"%.2f");
    else visualColor("Trail color",VisualParam::TrailRed,VisualParam::TrailGreen,VisualParam::TrailBlue);
    visualSlider("Opacity (test)##trail",VisualParam::TrailOpacity,"%.2f");
    visualSlider("Trail height",VisualParam::TrailHeight,"%.2f");
    visualSlider("Lifetime (seconds)",VisualParam::TrailSeconds,"%.1f");
    if (TrailEnabled()) {
        const int status=TrailStatus();
        realText(status==1 ? "Trail: submitted" : status==2 ? "Trail: renderer not ready" : "Trail: move to create trail",nullptr);
        if (fill) { const int fs=TrailFillStatus();
            realText(fs==1?"Fill: submitted (visibility unverified)":fs==2?"Fill: renderer busy":"Fill: waiting",nullptr);
        }
    }
}
static const char* depthCompareLabel(int value) {
    switch (value) { case 1:return "never"; case 2:return "less"; case 3:return "equal";
    case 4:return "less equal"; case 5:return "greater"; case 6:return "not equal";
    case 7:return "greater equal"; case 8:return "always"; default:return "unavailable"; }
}
static void drawVisualOptions() {
    bool tint=VisualTintEnabled();
    if (guiCheckbox("Enable colors",&tint)) SetVisualTintEnabled(tint);
    bool legacy=VisualLegacyMode();
    if (guiCheckbox("Old overlay (see-through)",&legacy)) SetVisualLegacyMode(legacy);
    if (!legacy) {
        if (HatPlayerPass() && !TrailEnabled()) realText("Hat uses player pass below.",nullptr);
        else {
            const int status=VisualOcclusionStatus();
            realText(status==1?"World draw: submitted":status==2?"World draw: no depth":status==3?"World draw: hooks unavailable":status==4?"World draw: blocked":status==5?"World draw: no GPU draw":"World draw: waiting",nullptr);
        }
    } else realText("Old overlay can show through walls.",nullptr);
    if (!legacy) {
        bool playerTiming=HatPlayerPass();
        if (guiCheckbox("Hat at player pass (test)",&playerTiming)) SetHatPlayerPass(playerTiming);
        if (playerTiming) {
            const int ps=HatPlayerPassStatus();
            realText(ps==1?"Player pass: submitted":ps==2?"Player pass: no depth":ps==3?"Player pass: hooks unavailable":ps==4?"Player pass: blocked":ps==5?"Player pass: no GPU draw":"Player pass: waiting for local player",nullptr);
            char line[96]{}; std::snprintf(line,sizeof(line),"Player compare: %s",depthCompareLabel(HatPlayerDepthCompare())); realText(line,nullptr);
        }
    }
    bool gradient=VertexGradientEnabled();
    if (guiCheckbox("Vertex gradient (test)",&gradient)) SetVertexGradientEnabled(gradient);
    realText("Gradient/opacity depend on material.",nullptr);
    auto diagnostic=[](const char* label,int bits) {
        char line[120]{};
        realText(label,nullptr);
        if (!(bits&1)) std::snprintf(line,sizeof(line),"  no context/data");
        else {
            std::snprintf(line,sizeof(line),"  DSV %s",bits&2?"bound":"none");
            realText(line,nullptr);
            std::snprintf(line,sizeof(line),"  depth %s / blend %s",bits&4?"on":"off",bits&8?"on":"off");
        }
        realText(line,nullptr);
    };
    diagnostic("World entities",VisualDepthWorld());
    char compareLine[96]{}; std::snprintf(compareLine,sizeof(compareLine),"World compare: %s",depthCompareLabel(WorldDepthCompare())); realText(compareLine,nullptr);
    diagnostic("Before UI",VisualDepthBefore()); diagnostic("After UI",VisualDepthAfter());
    realText("Diagnostic only, not proof of depth.",nullptr);
    realText("Resets after game closes.",nullptr);
    realText("Fill: turn off to use lines.",nullptr);
}
// ---------------------------------------------------------------------------
// VISUAL LAYER. Helpers below only set colours, spacing and text hierarchy.
// They never read or write ag::Settings, never call game addresses and never
// change which controls exist.
// ---------------------------------------------------------------------------
static hud_theme::Tokens visualTokens() {
    return hud_theme::tokens(hudPalette, hud_theme::editorOpacity(widgetOpacity),
                             hudRounding, 1.f);
}
static ImVec4 toVec(const hud_theme::Rgba& c) { return ImVec4(c.r,c.g,c.b,c.a); }
static void tintedText(const char* value,const hud_theme::Rgba& color) {
    if (!value) return;
    if (guiPushColor && guiPopColor) { guiPushColor(ImGuiCol_Text,toVec(color)); realText(value,nullptr); guiPopColor(1); }
    else realText(value,nullptr);
}
struct VisualStyleScope {
    int pushedColors=0, pushedVars=0;
    void visualPush(const hud_theme::Rgba& c,int slot) {
        if (guiPushColor && guiPopColor) { guiPushColor(slot,toVec(c)); ++pushedColors; }
    }
    void visualVar(int slot,float value) {
        if (guiPushVarFloat && guiPopVar) { guiPushVarFloat(slot,value); ++pushedVars; }
    }
    VisualStyleScope() {

    if (!guiPushColor || !guiPopColor) return;
    const auto t=visualTokens();
    visualVar(ImGuiStyleVar_WindowBorderSize,t.borderSize);
    visualVar(ImGuiStyleVar_WindowRounding,t.rounding);
    visualVar(ImGuiStyleVar_ChildRounding,t.rounding);
    visualVar(ImGuiStyleVar_FrameRounding,t.roundingSmall);
    visualVar(ImGuiStyleVar_PopupRounding,t.roundingSmall);
    visualVar(ImGuiStyleVar_ScrollbarRounding,8.f);
    visualPush(t.window,ImGuiCol_WindowBg);
    visualPush(t.panel,ImGuiCol_ChildBg);
    visualPush(t.window,ImGuiCol_PopupBg);
    visualPush(t.border,ImGuiCol_Border);
    visualPush(t.text,ImGuiCol_Text);
    visualPush(t.textDim,ImGuiCol_TextDisabled);
    visualPush(t.panelSoft,ImGuiCol_FrameBg);
    visualPush(hud_theme::withAlpha(t.accent,.10f),ImGuiCol_FrameBgHovered);
    visualPush(hud_theme::withAlpha(t.accent,.16f),ImGuiCol_FrameBgActive);
    visualPush(t.window,ImGuiCol_TitleBg);
    visualPush(hud_theme::withAlpha(t.accent,.12f),ImGuiCol_TitleBgActive);
    visualPush(t.window,ImGuiCol_TitleBgCollapsed);
    visualPush(hud_theme::withAlpha(t.accent,.08f),ImGuiCol_Button);
    visualPush(hud_theme::withAlpha(t.accent,.18f),ImGuiCol_ButtonHovered);
    visualPush(hud_theme::withAlpha(t.accent,.26f),ImGuiCol_ButtonActive);
    visualPush(t.accent,ImGuiCol_CheckMark);
    visualPush(t.accent,ImGuiCol_SliderGrab);
    visualPush(t.accent,ImGuiCol_SliderGrabActive);
    visualPush(hud_theme::withAlpha(t.accent,.13f),ImGuiCol_Header);
    visualPush(hud_theme::withAlpha(t.accent,.20f),ImGuiCol_HeaderHovered);
    visualPush(hud_theme::withAlpha(t.accent,.26f),ImGuiCol_HeaderActive);
    visualPush(t.borderSoft,ImGuiCol_Separator);
    visualPush(t.inset,ImGuiCol_ScrollbarBg);
    visualPush(hud_theme::withAlpha(t.accent,.24f),ImGuiCol_ScrollbarGrab);
}
    ~VisualStyleScope() {
        if (pushedColors>0 && guiPopColor) guiPopColor(pushedColors);
        if (pushedVars>0 && guiPopVar) guiPopVar(pushedVars);
    }
    VisualStyleScope(const VisualStyleScope&)=delete;
    VisualStyleScope& operator=(const VisualStyleScope&)=delete;
};

// Visual helpers: section header, hint, status dot, full-width button.
static void sectionTitle(const char* title,const char* subtitle=nullptr) {
    const auto t=visualTokens();
    // Accent bar drawn with the existing draw list; purely decorative.
    if (guiDrawList && guiCursor && drawRect) {
        const ImVec2 at=guiCursor();
        const auto& bar=hud_visual::section(t).accentBar;
        drawRect(guiDrawList(),ImVec2(at.x,at.y+2.f),ImVec2(at.x+2.f,at.y+11.f),
                 IM_COL32(int(bar.r*255),int(bar.g*255),int(bar.b*255),int(bar.a*255)),1.f,0);
        if (guiDummy) guiDummy(ImVec2(7.f,0.f));
        if (guiSameLine) guiSameLine(0.f,0.f);
    }
    tintedText(title,t.text);
    if (subtitle) {
        const bool wrap=guiPushTextWrap && guiPopTextWrap;
        if (wrap) guiPushTextWrap(0.f);
        const auto s=hud_visual::section(t); tintedText(subtitle,s.subtitle);
        if (wrap) guiPopTextWrap();
    }
}
static void visualHint(const char* value) {
    const bool wrap=guiPushTextWrap && guiPopTextWrap;
    if (wrap) guiPushTextWrap(0.f);
    tintedText(value,visualTokens().textDim);
    if (wrap) guiPopTextWrap();
}
static void fullWidthSlider(const char* label,float* value,float low,float high) {
    if (!guiSliderFloat) return;
    const bool wrap=guiPushTextWrap && guiPopTextWrap;
    if (wrap) guiPushTextWrap(0.f);
    controlLabel(label);
    if (wrap) guiPopTextWrap();
    char id[128]{}; std::snprintf(id,sizeof(id),"##hud_%s",label);
    if (guiPushWidth && guiPopWidth) guiPushWidth(-1.f);
    guiSliderFloat(id,value,low,high,"%.2f",0);
    if (guiPushWidth && guiPopWidth) guiPopWidth();
}
static bool canVisualPreview() {
    return hudHooksReady && guiButton && guiDrawList && guiCursor && guiDummy &&
        drawLine && drawTriangle && drawQuad && drawRect && drawGradientRect && drawImage && drawImageQuad;
}
static void drawVisualPanel() {
    guiSetNextPos(ImVec2(548.f,35.f), ImGuiCond_FirstUseEver, ImVec2(0,0));
    if (guiSetNextSize) guiSetNextSize(ImVec2(252.f,0.f),ImGuiCond_FirstUseEver);
    VisualStyleScope style;
    if (guiBegin("Visuals",nullptr,ImGuiWindowFlags_NoSavedSettings|ImGuiWindowFlags_NoCollapse|ImGuiWindowFlags_AlwaysAutoResize)) {
        if (!ChinaHatAvailable()) realText("Visuals unavailable",nullptr);
        else {
            sectionTitle("СТИЛЬ ЭФФЕКТОВ","экспериментально · session only");
            sectionTitle("CHINA HAT");
            bool hat=ChinaHatEnabled(); if (guiCheckbox("China Hat",&hat)) SetChinaHatEnabled(hat);
            if (canVisualPreview() && guiButton("Settings##open_hat",ImVec2(-1.f,0.f))) visualEditor=VisualEditor::Hat;
            sectionTitle("TRAIL");
            bool trace=TrailEnabled(); if (guiCheckbox("Trail",&trace)) SetTrailEnabled(trace);
            if (canVisualPreview() && guiButton("Settings##open_trail",ImVec2(-1.f,0.f))) visualEditor=VisualEditor::Trail;
            if (canVisualPreview() && guiButton("Options",ImVec2(-1.f,0.f))) visualEditor=VisualEditor::Options;
            if (!canVisualPreview()) {
                realText("Preview unavailable; inline settings:",nullptr);
                drawHatSettings(); drawTrailSettings(); drawVisualOptions();
            }
            visualHint("Прозрачность и depth-test не изменяются этим оформлением.");
        }
    }
    guiEnd();
}

// Example avatar, not an extracted game skin. These calls only draw UI geometry.
static void drawFallbackAvatar(ImDrawList* d,ImVec2 o,float scale) {
    auto p=[&](float x,float y,float z) { auto v=visual_avatar_preview::project({x,y,z},scale); return ImVec2(o.x+v.x,o.y+v.y); };
    const ImU32 skin=IM_COL32(178,121,88,255), hair=IM_COL32(57,39,31,255);
    // Torso and shoulders. Camera is nearly front-on, with a small angle so
    // the top and side of the head are visibly volumetric.
    drawQuad(d,p(.31f,.02f,-.19f),p(.31f,.02f,.05f),p(.31f,-.35f,.05f),p(.31f,-.35f,-.19f),IM_COL32(23,105,106,255));
    drawRect(d,p(-.31f,.02f,.05f),p(.31f,-.35f,.05f),IM_COL32(34,144,143,255),1.f,0);
    drawRect(d,p(-.41f,-.02f,.05f),p(-.31f,-.34f,.05f),skin,0.f,0);
    drawRect(d,p(.31f,-.02f,.05f),p(.41f,-.34f,.05f),skin,0.f,0);
    // Cube head: a shaded top and an original pixel-style face inspired by Steve.
    drawQuad(d,p(-.27f,.64f,-.27f),p(.27f,.64f,-.27f),p(.27f,.64f,.27f),p(-.27f,.64f,.27f),hair);
    drawQuad(d,p(.27f,.64f,-.27f),p(.27f,.64f,.27f),p(.27f,.10f,.27f),p(.27f,.10f,-.27f),IM_COL32(142,94,65,255));
    drawQuad(d,p(.27f,.64f,-.27f),p(.27f,.64f,.27f),p(.27f,.51f,.27f),p(.27f,.51f,-.27f),hair);
    ImVec2 top=p(-.27f,.64f,.27f), bottom=p(.27f,.10f,.27f);
    drawRect(d,top,bottom,skin,0.f,0);
    const float cell=(bottom.x-top.x)/8.f, ch=(bottom.y-top.y)/8.f;
    const int pattern[8][8]={
      {1,1,1,1,1,1,1,1},{1,1,1,1,1,1,1,1},{1,0,0,0,0,0,0,1},
      {0,2,3,0,0,3,2,0},{0,0,0,0,0,0,0,0},{0,0,0,4,4,0,0,0},
      {0,0,4,4,4,4,0,0},{0,0,0,4,4,0,0,0}
    };
    const ImU32 palette[]={skin,hair,IM_COL32(223,222,213,255),IM_COL32(58,66,142,255),IM_COL32(107,70,52,255)};
    for (int y=0;y<8;++y) for (int x=0;x<8;++x) if (pattern[y][x])
        drawRect(d,ImVec2(top.x+x*cell,top.y+y*ch),ImVec2(top.x+(x+1)*cell+.2f,top.y+(y+1)*ch+.2f),palette[pattern[y][x]],0.f,0);
}
// Candidate texture UVs use the conventional 64x64 / 64x32 skin layout.
// This is an independently projected sample model, not the game's model.
static ImVec2 skinUv(const visual_player_skin::Snapshot& s,float x,float y) {
    return ImVec2(x/64.f,y/64.f*visual_skin_policy::vScale(s.width,s.height));
}
static void skinRect(ImDrawList* d,const visual_player_skin::Snapshot& s,ImVec2 a,ImVec2 b,float x0,float y0,float x1,float y1,ImU32 tint=IM_COL32_WHITE) {
    drawImage(d,static_cast<ImTextureID>(reinterpret_cast<std::uintptr_t>(s.view)),a,b,skinUv(s,x0,y0),skinUv(s,x1,y1),tint);
}
static void skinQuad(ImDrawList* d,const visual_player_skin::Snapshot& s,ImVec2 a,ImVec2 b,ImVec2 c,ImVec2 e,float x0,float y0,float x1,float y1,ImU32 tint=IM_COL32_WHITE) {
    drawImageQuad(d,static_cast<ImTextureID>(reinterpret_cast<std::uintptr_t>(s.view)),a,b,c,e,skinUv(s,x0,y0),skinUv(s,x1,y0),skinUv(s,x1,y1),skinUv(s,x0,y1),tint);
}
static void drawAvatar(ImDrawList* d,ImVec2 o,float scale,const visual_player_skin::Snapshot& s) {
    if (!s.view) { drawFallbackAvatar(d,o,scale); return; }
    auto p=[&](float x,float y,float z) { auto v=visual_avatar_preview::project({x,y,z},scale); return ImVec2(o.x+v.x,o.y+v.y); };
    // Upper torso and arms with actual sampled texture; preserves the same
    // near-front view and geometric dimensions as the fallback model.
    skinQuad(d,s,p(.31f,.02f,-.19f),p(.31f,.02f,.05f),p(.31f,-.35f,.05f),p(.31f,-.35f,-.19f),16,20,20,32,IM_COL32(190,190,190,255));
    skinRect(d,s,p(-.31f,.02f,.05f),p(.31f,-.35f,.05f),20,20,28,32);
    skinRect(d,s,p(-.41f,-.02f,.05f),p(-.31f,-.34f,.05f),44,20,48,32);
    skinRect(d,s,p(.31f,-.02f,.05f),p(.41f,-.34f,.05f),44,20,48,32);
    // Head top, visible side and front. The outer head layer can carry hair,
    // glasses, hats etc.; custom Bedrock geometry is not captured.
    skinQuad(d,s,p(-.27f,.64f,-.27f),p(.27f,.64f,-.27f),p(.27f,.64f,.27f),p(-.27f,.64f,.27f),8,0,16,8,IM_COL32(210,210,210,255));
    skinQuad(d,s,p(.27f,.64f,-.27f),p(.27f,.64f,.27f),p(.27f,.10f,.27f),p(.27f,.10f,-.27f),16,8,24,16,IM_COL32(190,190,190,255));
    skinRect(d,s,p(-.27f,.64f,.27f),p(.27f,.10f,.27f),8,8,16,16);
    skinRect(d,s,p(-.27f,.64f,.27f),p(.27f,.10f,.27f),40,8,48,16);
}
static ImU32 previewMaterial(bool hat,std::uint64_t now,china_hat::Color vertex) {
    const bool colors=VisualTintEnabled(), rainbow=hat?HatRainbowEnabled():TrailRainbowEnabled();
    const float opacity=GetVisualParam(hat?VisualParam::HatOpacity:VisualParam::TrailOpacity);
    const float speed=GetVisualParam(hat?VisualParam::HatRainbowSpeed:VisualParam::TrailRainbowSpeed);
    auto tint=visual_color::animated(colors,rainbow,
      hat?std::array<float,3>{{GetVisualParam(VisualParam::HatRed),GetVisualParam(VisualParam::HatGreen),GetVisualParam(VisualParam::HatBlue)}}:
          std::array<float,3>{{GetVisualParam(VisualParam::TrailRed),GetVisualParam(VisualParam::TrailGreen),GetVisualParam(VisualParam::TrailBlue)}},speed,now);
    if (colors && rainbow && VertexGradientEnabled()) tint={{vertex.r/255.f,vertex.g/255.f,vertex.b/255.f,1.f}};
    return IM_COL32(static_cast<int>(tint[0]*255.f),static_cast<int>(tint[1]*255.f),static_cast<int>(tint[2]*255.f),static_cast<int>(opacity*255.f));
}
static void drawHatPreview(ImDrawList* d,ImVec2 o,std::uint64_t now,const visual_player_skin::Snapshot& skin) {
    visual_avatar_preview::HatSettings s{GetVisualParam(VisualParam::HatRadius),GetVisualParam(VisualParam::HatConeHeight),
      GetVisualParam(VisualParam::HatOffset),GetVisualParam(VisualParam::HatBobAmplitude),GetVisualParam(VisualParam::HatBobSpeed),
      GetVisualParam(VisualParam::HatRainbowSpeed),VisualTintEnabled() && HatRainbowEnabled() && VertexGradientEnabled()};
    china_hat::Mesh mesh{}; if (!visual_avatar_preview::hat(mesh,s,now)) return;
    const float scale=visual_avatar_preview::hatScale(s);
    auto p=[&](china_hat::Point v) { auto v2=visual_avatar_preview::project(v,scale); return ImVec2(o.x+v2.x,o.y+v2.y); };
    // Back faces first, then the head, then near faces: the model masks the
    // rear of the brim like the game's depth-tested renderer should.
    auto sector=[&](int i) {
        const auto& apex=mesh.triangles[i*3]; const auto& a=mesh.triangles[i*3+1]; const auto& b=mesh.triangles[i*3+2];
        ImU32 color=previewMaterial(true,now,apex.color);
        if (HatFillEnabled()) drawTriangle(d,p(apex.position),p(a.position),p(b.position),color);
        drawLine(d,p(a.position),p(b.position),color,1.5f);
        if (i%4==0) drawLine(d,p(apex.position),p(a.position),color,1.f);
    };
    for (int i=0;i<48;++i) if (mesh.triangles[i*3+1].position.z+mesh.triangles[i*3+2].position.z<0) sector(i);
    drawAvatar(d,o,scale,skin);
    for (int i=0;i<48;++i) if (mesh.triangles[i*3+1].position.z+mesh.triangles[i*3+2].position.z>=0) sector(i);
}
static void drawTrailPreview(ImDrawList* d,ImVec2 o,std::uint64_t now,const visual_player_skin::Snapshot& skin) {
    const float seconds=GetVisualParam(VisualParam::TrailSeconds);
    const float height=visual_avatar_preview::trailHeight(GetVisualParam(VisualParam::TrailHeight));
    const float phase=visual_gradient::phase(now,GetVisualParam(VisualParam::TrailRainbowSpeed));
    auto p=[&](visual_avatar_preview::Point2 v,float up) { return ImVec2(o.x+v.x,o.y+v.y-up); };
    constexpr int n=48;
    // This is example walking motion, not the player's real trail history.
    for (int i=n-1;i>=0;--i) {
        auto a=visual_avatar_preview::trailPoint(i/static_cast<float>(n),now,seconds);
        auto b=visual_avatar_preview::trailPoint((i+1)/static_cast<float>(n),now,seconds);
        auto color=visual_gradient::trail(n-i,n+1,phase,255);
        ImU32 tint=previewMaterial(false,now,color);
        if (TrailFillEnabled()) drawQuad(d,p(a,0),p(b,0),p(b,height),p(a,height),tint);
        drawLine(d,p(a,0),p(b,0),tint,1.5f);
    }
    // Side-facing sample player: distinct from the front-facing hat preview.
    const float x=o.x+226.f, foot=o.y+183.f;
    const float step=std::sin(static_cast<double>(now)*.006)*5.f;
    if (skin.view) {
        skinRect(d,skin,ImVec2(x-14,foot-92),ImVec2(x+14,foot-65),16,8,24,16);
        skinRect(d,skin,ImVec2(x-14,foot-92),ImVec2(x+14,foot-65),48,8,56,16);
        skinRect(d,skin,ImVec2(x-13,foot-65),ImVec2(x+13,foot-29),16,20,20,32);
        skinRect(d,skin,ImVec2(x-6,foot-60),ImVec2(x+7,foot-33),40,20,44,32);
        skinRect(d,skin,ImVec2(x-12+step,foot-29),ImVec2(x-1+step,foot),0,20,4,32,IM_COL32(190,190,190,255));
        skinRect(d,skin,ImVec2(x+1-step,foot-29),ImVec2(x+12-step,foot),0,20,4,32);
    } else {
        drawRect(d,ImVec2(x-14,foot-92),ImVec2(x+14,foot-65),IM_COL32(178,121,88,255),1.f,0);
        drawRect(d,ImVec2(x-14,foot-94),ImVec2(x+14,foot-86),IM_COL32(57,39,31,255),1.f,0);
        drawRect(d,ImVec2(x+5,foot-83),ImVec2(x+10,foot-79),IM_COL32(58,66,142,255),0.f,0);
        drawRect(d,ImVec2(x-13,foot-65),ImVec2(x+13,foot-29),IM_COL32(34,144,143,255),1.f,0);
        drawRect(d,ImVec2(x-6,foot-60),ImVec2(x+7,foot-33),IM_COL32(25,117,119,255),1.f,0);
        drawRect(d,ImVec2(x-12+step,foot-29),ImVec2(x-1+step,foot),IM_COL32(48,60,120,255),1.f,0);
        drawRect(d,ImVec2(x+1-step,foot-29),ImVec2(x+12-step,foot),IM_COL32(58,72,139,255),1.f,0);
    }
}
static void drawEffectPreview(VisualEditor editor) {
    guiSetNextPos(ImVec2(18.f,90.f),ImGuiCond_Always,ImVec2(0,0));
    if (guiSetNextSize) guiSetNextSize(ImVec2(310.f,360.f),ImGuiCond_FirstUseEver);
    VisualStyleScope style;
    if (guiBegin("Preview##effect",nullptr,ImGuiWindowFlags_NoSavedSettings|ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoCollapse)) {
        if (guiCheckbox("Game model replay (test)",&previewGameModelTest)) {
            if (!previewGameModelTest) SetPlayerModelPreviewActive(false);
        }
        // ImGui consumes the texture only after GUI construction has finished.
        release(previewFrameView);
        auto* d=guiDrawList(); const auto now=GetTickCount64();
        if (previewGameModelTest) {
            auto* model=visual_player_replay::acquire(frameDevice); previewFrameView=model;
            char status[96]{};
            if (model) std::snprintf(status,sizeof(status),"%u game draws copied (verify)",visual_player_replay::lastDraws.load(std::memory_order_relaxed));
            else std::snprintf(status,sizeof(status),"Waiting for player draws");
            realText(status,nullptr);
            const ImVec2 p=guiCursor();
            if (d) {
                drawGradientRect(d,p,ImVec2(p.x+280.f,p.y+215.f),IM_COL32(19,33,37,255),IM_COL32(17,26,33,255),IM_COL32(8,14,20,255),IM_COL32(9,18,20,255));
                if (model) drawImage(d,static_cast<ImTextureID>(reinterpret_cast<std::uintptr_t>(model)),ImVec2(p.x+32.f,p.y),ImVec2(p.x+247.f,p.y+215.f),ImVec2(0,0),ImVec2(1,1),IM_COL32_WHITE);
            }
            guiDummy(ImVec2(280.f,215.f));
            realText("Use F5 for third person.",nullptr);
            realText(editor==VisualEditor::Hat?"Hat preview: enable China Hat.":"Trail preview not available yet.",nullptr);
        } else {
            auto skin=visual_player_skin::acquire(frameDevice); previewFrameView=skin.view;
            if (skin.view) {
                char label[96]{}; std::snprintf(label,sizeof(label),"Game texture %u/%u (verify)",skin.index+1,skin.count); realText(label,nullptr);
            } else realText("Waiting for player; sample avatar",nullptr);
            const ImVec2 p=guiCursor();
            if (d) {
                drawGradientRect(d,p,ImVec2(p.x+280.f,p.y+215.f),IM_COL32(19,33,37,255),IM_COL32(17,26,33,255),IM_COL32(8,14,20,255),IM_COL32(9,18,20,255));
                drawLine(d,ImVec2(p.x+15,p.y+205),ImVec2(p.x+265,p.y+205),IM_COL32(42,85,68,150),1.f);
                if (editor==VisualEditor::Hat) drawHatPreview(d,p,now,skin); else drawTrailPreview(d,p,now,skin);
            }
            guiDummy(ImVec2(280.f,215.f));
            realText("Sample model; game shader may differ.",nullptr);
            if (skin.count>1 && guiButton("Next texture##skin",ImVec2(-1.f,0.f))) visual_player_skin::next();
            if (!skin.view) realText("Use third person to detect skin.",nullptr);
        }
    }
    guiEnd();
}
static void drawVisualEditor() {
    const VisualEditor current=visualEditor;
    // Read settings after editing so the model responds in this frame.
    guiSetNextPos(ImVec2(350.f,90.f),ImGuiCond_Always,ImVec2(0,0));
    if (guiSetNextSize) guiSetNextSize(ImVec2(310.f,430.f),ImGuiCond_FirstUseEver);
    VisualStyleScope style;
    const char* title=current==VisualEditor::Hat?"China Hat settings":current==VisualEditor::Trail?"Trail settings":"Visual options";
    if (guiBegin(title,nullptr,ImGuiWindowFlags_NoSavedSettings|ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoCollapse)) {
        if (guiButton && guiButton("Back to menu",ImVec2(-1.f,0.f))) visualEditor=VisualEditor::None;
        if (current==VisualEditor::Hat) drawHatSettings();
        else if (current==VisualEditor::Trail) drawTrailSettings();
        else drawVisualOptions();
    }
    guiEnd();
    if (visualEditor==current && current!=VisualEditor::Options) drawEffectPreview(current);
}

// Restyle only actual legacy HUD/notification windows, preserving their real
// metrics, positions, interaction and layout. Never target the game's UI.
static bool themedBegin(const char* name,bool* open,int flags) {
    if (!insideLegacyHud || !hud_theme::card(name) || !guiPushColor || !guiPopColor || !guiPushVarFloat || !guiPopVar)
        return realGuiBegin(name,open,flags);
    const auto bg=hud_theme::background(widgetOpacity*legacyOpacity,hudPalette), border=hud_theme::border(widgetOpacity*legacyOpacity,hudPalette);
    guiPushColor(ImGuiCol_WindowBg,ImVec4(bg.r,bg.g,bg.b,bg.a));
    guiPushColor(ImGuiCol_Border,ImVec4(border.r,border.g,border.b,border.a));
    guiPushVarFloat(ImGuiStyleVar_WindowRounding,hud_theme::rounding(hudRounding));
    guiPushVarFloat(ImGuiStyleVar_WindowBorderSize,1.f);
    if (std::strcmp(name,"##hud_keystrokes")==0) flags &= ~ImGuiWindowFlags_NoBackground;
    const bool result=realGuiBegin(name,open,flags);
    guiPopVar(2); guiPopColor(2);
    return result;
}
// Draw the same Module List card in the editor and during play.
static void drawModuleHud(void* runtime, bool preview) {
    if (!runtime || !guiPushColor || !guiPopColor || !guiFontScale) return;
    auto* s = reinterpret_cast<ag::Settings*>(reinterpret_cast<unsigned char*>(runtime)+0x48);
    const float x = preview ? (frameWidth>240.f?frameWidth-232.f:930.f)+hudX*20.f : hudX*static_cast<float>(frameWidth);
    const float y = preview ? 286.f+hudY*54.f : hudY*static_cast<float>(frameHeight);
    guiSetNextPos(ImVec2(x,y),ImGuiCond_Always,ImVec2(0,0));
    const auto bg=hud_theme::background(hudOpacity,hudPalette), border=hud_theme::border(hudOpacity,hudPalette);
    guiPushColor(ImGuiCol_WindowBg,ImVec4(bg.r,bg.g,bg.b,bg.a));
    guiPushColor(ImGuiCol_Border,ImVec4(border.r,border.g,border.b,border.a));
    guiPushColor(ImGuiCol_Text,ImVec4(hudColor[0],hudColor[1],hudColor[2],1.f));
    const bool vars=guiPushVarFloat && guiPopVar;
    if (vars) { guiPushVarFloat(ImGuiStyleVar_WindowRounding,hud_theme::rounding(hudRounding)); guiPushVarFloat(ImGuiStyleVar_WindowBorderSize,1.f); }
    const int flags=ImGuiWindowFlags_NoTitleBar|ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoMove|
        ImGuiWindowFlags_NoInputs|ImGuiWindowFlags_AlwaysAutoResize|ImGuiWindowFlags_NoSavedSettings|
        ImGuiWindowFlags_NoFocusOnAppearing;
    if (guiBegin(preview?"##module_hud_preview":"##module_hud",nullptr,flags)) {
        guiFontScale(hudScale);
        guiPushColor(ImGuiCol_Text,ImVec4(.53f,.95f,.65f,1.f));
        realText("MODULES",nullptr); guiPopColor(1);
        module_hud::labels(s->enabled,ChinaHatEnabled(),TrailEnabled(),[](const char* label) {
            if (guiSameLine) {
                guiPushColor(ImGuiCol_Text,ImVec4(.35f,.92f,.51f,1.f)); realText(">",nullptr); guiPopColor(1);
                guiSameLine(0.f,6.f);
            }
            realText(label,nullptr);
        });
    }
    guiEnd(); if (vars) guiPopVar(2); guiPopColor(3);
}
static void patchedDrawHud(void* runtime) {
    const bool menu=runtime && *(reinterpret_cast<unsigned char*>(runtime)+0x551);
    const bool editing=module_hud::editing(menu,hudEditorOpen || visualEditor!=VisualEditor::None);
    hudEditorActive.store(editing,std::memory_order_release);
    if (!editing) {
        if (hudThemeReady.load(std::memory_order_acquire) && guiPushColor && guiPopColor) {
            const auto* settings=runtime ? reinterpret_cast<const ag::Settings*>(reinterpret_cast<const unsigned char*>(runtime)+0x48) : nullptr;
            const bool previous=insideLegacyHud; const float oldOpacity=legacyOpacity;
            legacyOpacity=settings ? hud_theme::alpha(settings->opacity) : 1.f;
            guiPushColor(ImGuiCol_Text,ImVec4(hudColor[0]*(settings ? hud_theme::alpha(settings->text[0]) : 1.f),
              hudColor[1]*(settings ? hud_theme::alpha(settings->text[1]) : 1.f),
              hudColor[2]*(settings ? hud_theme::alpha(settings->text[2]) : 1.f),1.f));
            insideLegacyHud=true; realDrawHud(runtime); insideLegacyHud=previous; legacyOpacity=oldOpacity;
            guiPopColor(1);
        } else realDrawHud(runtime);
        if (moduleHudEnabled) drawModuleHud(runtime,false);
    }
}
static void patchedNotificationDraw(void* queue, const ag::Settings& s) {
    if (hudEditorActive.load(std::memory_order_acquire)) return;
    if (hudThemeReady.load(std::memory_order_acquire) && guiPushColor && guiPopColor) {
        guiPushColor(ImGuiCol_Text,ImVec4(hudColor[0]*hud_theme::alpha(s.text[0]),hudColor[1]*hud_theme::alpha(s.text[1]),hudColor[2]*hud_theme::alpha(s.text[2]),1.f));
        const bool previous=insideLegacyHud; const float oldOpacity=legacyOpacity;
        insideLegacyHud=true; legacyOpacity=hud_theme::alpha(s.opacity);
        realNotificationDraw(queue,s);
        insideLegacyHud=previous; legacyOpacity=oldOpacity; guiPopColor(1);
    } else realNotificationDraw(queue,s);
}
static bool editorVisible[5] = {true,true,true,true,true};
static int editorSelected = 0;
static float editorWidth() { return frameWidth ? float(frameWidth) : 1280.f; }
static float editorHeight() { return frameHeight ? float(frameHeight) : 720.f; }
static const char* editorPanelName(int index) {
    constexpr const char* names[5] = {"Home","HUD","Modules","Appearance","Settings"};
    return index>=0 && index<5 ? names[index] : "Panel";
}
static void editorColumn(int index) {
    const float width=editorWidth(), height=editorHeight();
    const bool wide=hud_theme::columnsFit(width);
    const float x=wide ? hud_theme::panelX(index,width) : hud_theme::editorMetrics.panelLeft;
    guiSetNextPos(ImVec2(x,hud_theme::panelTop(width)),ImGuiCond_Always,ImVec2(0,0));
    if (guiSetNextSize) guiSetNextSize(ImVec2(wide ? hud_theme::panelWidthFor(width) : hud_theme::compactPanelWidth(width),
                                               hud_theme::panelHeight(index,width,height)),ImGuiCond_Always);
}
static void editorToolbar() {
    const float width=editorWidth();
    const bool wide=hud_theme::columnsFit(width);
    guiSetNextPos(ImVec2(hud_theme::editorMetrics.panelLeft,18.f),ImGuiCond_Always,ImVec2(0,0));
    VisualStyleScope style;
    if (guiBegin("##gc_editor_toolbar",nullptr,ImGuiWindowFlags_NoTitleBar|ImGuiWindowFlags_NoResize|
                 ImGuiWindowFlags_NoMove|ImGuiWindowFlags_AlwaysAutoResize|ImGuiWindowFlags_NoSavedSettings|
                 ImGuiWindowFlags_NoFocusOnAppearing)) {
        const auto t=visualTokens();
        tintedText("AOGIRI GREEN",t.accent);
        if (guiSameLine) guiSameLine(0.f,8.f);
        if (guiButton && guiButton("Panels",ImVec2(58.f,0.f))) {
            const bool any=editorVisible[0]||editorVisible[1]||editorVisible[2]||editorVisible[3]||editorVisible[4];
            for (bool& visible:editorVisible) visible=!any;
        }
        if (guiSameLine) guiSameLine(0.f,4.f);
        if (guiButton && guiButton("Back to menu",ImVec2(100.f,0.f))) hudEditorOpen=false;
        if (!wide && guiButton) {
            // One panel at a time fits on narrow screens; buttons remain
            // available even if a user hid every panel via Settings.
            for (int i=0;i<5;++i) {
                if (i>0 && guiSameLine && (width>=480.f || (i%2)!=0)) guiSameLine(0.f,4.f);
                if (guiButton(editorPanelName(i),ImVec2(0,0))) { editorSelected=i; editorVisible[i]=true; }
            }
        }
    }
    guiEnd();
}
static void editorHome() {
    editorColumn(0);
    VisualStyleScope style;
    if (guiBegin("Home##gc",nullptr,ImGuiWindowFlags_NoSavedSettings|ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoCollapse)) {
        const auto t=visualTokens();
        tintedText("AOGIRI",t.accent); tintedText("GREEN",t.accent);
        visualHint("GAYClient · HUD redesign");
        sectionTitle("СЕССИЯ");
        tintedText(ag::kVersion,t.textMuted);
        visualHint("Только визуальный слой");
        tintedText(ChinaHatAvailable()?"Human Visuals доступны":"Visuals unavailable",t.textDim);
        if (!notificationHookReady) visualHint("Notifications may remain visible.");
    }
    guiEnd();
}
static void editorHud(void* runtime) {
    editorColumn(1);
    VisualStyleScope style;
    if (guiBegin("HUD##gc",nullptr,ImGuiWindowFlags_NoSavedSettings|ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoCollapse)) {
        sectionTitle("ВИДЖЕТЫ");
        if (runtime && setModule) {
            auto* settings=reinterpret_cast<ag::Settings*>(reinterpret_cast<unsigned char*>(runtime)+0x48);
            for (const auto& entry:module_hud::entries) {
                bool enabled=settings->IsEnabled(entry.id);
                if (guiCheckbox(entry.name,&enabled)) setModule(runtime,entry.id,enabled);
            }
        }
        visualHint("Module List: Modules");
    }
    guiEnd();
}
static void editorModules(void* runtime) {
    editorColumn(2);
    VisualStyleScope style;
    if (guiBegin("Modules##gc",nullptr,ImGuiWindowFlags_NoSavedSettings|ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoCollapse)) {
        sectionTitle("MODULE LIST HUD");
        guiCheckbox("Module List HUD",&moduleHudEnabled);
        visualHint("Preview on the right");
        fullWidthSlider("Module list scale",&hudScale,.7f,1.6f);
        fullWidthSlider("Module list opacity",&hudOpacity,0.f,1.f);
        fullWidthSlider("Module list X",&hudX,0.f,.9f);
        fullWidthSlider("Module list Y",&hudY,0.f,.9f);
    }
    guiEnd();
}
static void editorAppearance() {
    editorColumn(3);
    VisualStyleScope style;
    if (guiBegin("Appearance##gc",nullptr,ImGuiWindowFlags_NoSavedSettings|ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoCollapse)) {
        sectionTitle("ТЕМА","Выберите цветовую схему");
        for (int i=0;i<3;i++) {
            if (guiButton && guiButton(hud_theme::palette(i).name,ImVec2(-1.f,0.f))) {
                hudPalette=i; const auto c=hud_theme::palette(i).accent;
                hudColor[0]=c.r; hudColor[1]=c.g; hudColor[2]=c.b;
            }
        }
        sectionTitle("ОФОРМЛЕНИЕ");
        fullWidthSlider("Card rounding",&hudRounding,0.f,20.f);
        if (hudThemeReady.load(std::memory_order_acquire)) fullWidthSlider("Widget background opacity",&widgetOpacity,0.f,1.f);
        else realText("Widget theme unavailable; original widgets retained.",nullptr);
        if (guiColorEdit3) { realText("HUD text color",nullptr); if (guiPushWidth && guiPopWidth) guiPushWidth(-1.f);
            guiColorEdit3("##hud_color",hudColor,0); if (guiPushWidth && guiPopWidth) guiPopWidth(); }
    }
    guiEnd();
}
static void editorSettings() {
    editorColumn(4);
    VisualStyleScope style;
    if (guiBegin("Settings##gc",nullptr,ImGuiWindowFlags_NoSavedSettings|ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoCollapse)) {
        sectionTitle("ПАНЕЛИ");
        for (int i=0;i<5;++i) guiCheckbox(editorPanelName(i),&editorVisible[i]);
        sectionTitle("СЕССИЯ");
        visualHint("Style resets after game exit.");
        if (!notificationHookReady) realText("Notifications may remain visible.",nullptr);
        visualHint("Профиль ядра визуальный слой не перезаписывает.");
    }
    guiEnd();
}
static void editorPreviewCaption() {
    const float x=hud_theme::previewX(editorWidth());
    guiSetNextPos(ImVec2(x,hud_theme::panelTop(editorWidth())),ImGuiCond_Always,ImVec2(0,0));
    if (guiSetNextSize) guiSetNextSize(ImVec2(hud_theme::editorMetrics.previewWidth,0.f),ImGuiCond_FirstUseEver);
    VisualStyleScope style;
    if (guiBegin("Preview##gc_caption",nullptr,ImGuiWindowFlags_NoTitleBar|ImGuiWindowFlags_NoResize|
                 ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoSavedSettings|ImGuiWindowFlags_AlwaysAutoResize)) {
        const auto t=visualTokens();
        tintedText("ПРЕДПРОСМОТР",t.accent);
        visualHint("Module List · реальные модули");
    }
    guiEnd();
}
static void drawHudEditor(void* runtime) {
    editorToolbar();
    const bool wide=hud_theme::columnsFit(editorWidth());
    if (editorVisible[0] && (wide || editorSelected==0)) editorHome();
    if (editorVisible[1] && (wide || editorSelected==1)) editorHud(runtime);
    if (editorVisible[2] && (wide || editorSelected==2)) editorModules(runtime);
    if (editorVisible[3] && (wide || editorSelected==3)) editorAppearance();
    if (editorVisible[4] && (wide || editorSelected==4)) editorSettings();
    if (editorWidth()>=600.f && editorHeight()>=350.f) {
        editorPreviewCaption();
        if (hudEditorOpen) drawModuleHud(runtime,true);
    }
}
/*
 * VISUAL-ONLY NOTE
 * The editor above is a reskin of the existing controls: the same widget
 * checkboxes, the same three palettes, the same rounding/opacity/colour
 * sliders and the same Module List settings, now grouped into five columns.
 * No setting, hook or render path was added.
 */
static void drawHudEntry() {
    if (!hudHooksReady) return;
    guiSetNextPos(ImVec2(18.f,130.f),ImGuiCond_FirstUseEver,ImVec2(0,0));
    if (guiSetNextSize) guiSetNextSize(ImVec2(166.f,0.f),ImGuiCond_FirstUseEver);
    VisualStyleScope style;
    if (guiBegin("HUD",nullptr,ImGuiWindowFlags_AlwaysAutoResize|ImGuiWindowFlags_NoSavedSettings|ImGuiWindowFlags_NoCollapse)) {
        sectionTitle("GAYCLIENT","HUD editor");
        guiCheckbox("Open HUD editor",&hudEditorOpen);
        guiCheckbox("Module List HUD",&moduleHudEnabled);
    }
    guiEnd();
}

static void patchedDrawGui(void* runtime) {
    // The old loop never calls RequestUnload for Delete. This is an explicit
    // one-way overlay stop, NOT a DLL unload: callbacks and hooks remain pinned.
    bool down = pageKeysReady && keyDown && keyDown(VK_DELETE);
    bool editing = getIO && getIO().WantTextInput;
    if (disableHotkey.update(down, down && gameIsForeground(), editing)) {
        overlayDisabled.store(true, std::memory_order_release);
        SetMenuMouseOpen(false);
        SetChinaHatEnabled(false);
        SetTrailEnabled(false);
        SetPlayerSkinPreviewActive(false);
        SetPlayerModelPreviewActive(false);
        SetPlayerPreviewHatActive(false);
        if (bridgeActive) bridgeActive(false);
        DisableUwpExtraInput();
        log("overlay disabled by Delete; DLLs remain loaded until game exit");
        return;
    }
    const bool menuOpen=runtime && *(reinterpret_cast<unsigned char*>(runtime)+0x551);
    SetMenuMouseOpen(menuOpen);
    if (!menuOpen) { hudEditorOpen=false; visualEditor=VisualEditor::None; }
    const bool editingVisual=menuOpen && visualEditor!=VisualEditor::None && canVisualPreview();
    SetPlayerSkinPreviewActive(editingVisual && visualEditor!=VisualEditor::Options);
    SetPlayerModelPreviewActive(editingVisual && visualEditor!=VisualEditor::Options && previewGameModelTest);
    SetPlayerPreviewHatActive(editingVisual && visualEditor==VisualEditor::Hat && previewGameModelTest);
    if (!menuOpen || (!hudEditorOpen && !editingVisual)) {
        if (menuOpen) { VisualStyleScope style; realDrawGui(runtime); }
        else realDrawGui(runtime);
    }
    if (editingVisual) { drawVisualEditor(); return; }
    if (menuOpen && hudEditorOpen && hudHooksReady) { drawHudEditor(runtime); return; }
    // Exact legacy build layout, verified against the original binary.
    if (!runtime || !*(reinterpret_cast<unsigned char*>(runtime) + 0x551)) return;
    auto* settings = reinterpret_cast<ag::Settings*>(reinterpret_cast<unsigned char*>(runtime) + 0x48);
    bool enabled = settings->enabled[static_cast<size_t>(ag::ModuleId::Coordinates)];
    guiSetNextPos(ImVec2(18.0f, 235.0f), ImGuiCond_FirstUseEver, ImVec2(0, 0));
    const int flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                      ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoCollapse;
    if (guiBegin("##coordinate_switch_115", nullptr, flags)) {
        if (guiCheckbox("Coordinates (test)", &enabled)) setModule(runtime, ag::ModuleId::Coordinates, enabled);
        realText(MenuMouseStatus() ? "Menu mouse: test enabled" : "Menu mouse: unavailable", nullptr);
    }
    guiEnd();
    drawVisualPanel();
    drawHudEntry();
}

// Called from legacy drawMarker, while its own render lock is held. The ImGui
// context is created there immediately before this function is first called.
static void patchedNewFrame() {
    ImGuiIO& io = getIO();
    if (!io.BackendRendererUserData && frameDevice && frameContext) {
        // The legacy bootstrap creates a context but never installs its bundled
        // Cyrillic font. Load it before DX11 builds the atlas on the first frame.
        if (io.Fonts && io.Fonts->Fonts.Size == 0) {
            ImFont* font = addFont(16.0f, false);
            if (font) io.FontDefault = font;
            else log("could not load embedded Cyrillic font");
        }
        if (!rendererInit(frameDevice, frameContext)) log("ImGui DX11 initialization failed");
    }
    io.DisplaySize = ImVec2(static_cast<float>(frameWidth), static_cast<float>(frameHeight));
    int wheelX = 0, wheelY = 0;
    TakeUwpWheel(wheelX, wheelY);
    // Prefer the external wheel-only bridge when connected; otherwise use UWP.
    // Do not add both, or one physical wheel movement could scroll twice.
    takeExternalWheel(wheelX, wheelY);
    if (pointerInUse && (wheelX || wheelY)) addWheel(&io, wheelX / 120.0f, wheelY / 120.0f);
    const float pageWheel = pageScroll.step(pointerInUse && pageKeysReady,
        pageKeysReady && keyDown(VK_PRIOR), pageKeysReady && keyDown(VK_NEXT), GetTickCount64());
    if (pageWheel != 0.0f) addWheel(&io, 0.0f, pageWheel);
    LARGE_INTEGER now{};
    QueryPerformanceCounter(&now);
    if (lastFrame.QuadPart && frequency.QuadPart && now.QuadPart > lastFrame.QuadPart) {
        double seconds = double(now.QuadPart - lastFrame.QuadPart) / double(frequency.QuadPart);
        io.DeltaTime = static_cast<float>(seconds > 0.25 ? 0.25 : seconds);
    } else io.DeltaTime = 1.0f / 60.0f;
    lastFrame = now;
    realNewFrame();
}

// The legacy loop calls Update six times; later calls erase key transitions.
static void patchedUpdate(void* input, const void* settings, bool targetFocused, bool appFocused) {
    if (insidePresent) {
        if (seenInput) return;
        seenInput = true;
    }
    realUpdate(input, settings, targetFocused, appFocused);
}

// Request the scroll and stop keys from the existing UWP polling bridge.
// No key or wheel activity is stored on disk.
static void patchedWatchedKeys(const std::array<bool, 256>& requested) {
    auto keys = requested;
    keys[VK_PRIOR] = true;
    keys[VK_NEXT] = true;
    keys[VK_DELETE] = true;
    realWatchedKeys(keys);
}

// Use coordinates relative to the actual client area, not the UWP screen /
// non-client origin. All Win32 values are obtained on the same thread, so
// Windows DPI virtualization affects cursor and client rectangle consistently.
// If these APIs are unavailable in AppContainer, retain the legacy UWP input.
static bool patchedPointer(float& x, float& y, float& w, float& h) {
    pointerInUse = true;
    // CoreWindow bounds describe the non-client origin in DIPs. Subtract it
    // on the dispatcher thread's consistent sample before scaling to DXGI pixels.
    if (GetSampledUwpPointer(x, y, w, h)) return true;
    if (!pointerWindow || !IsWindow(pointerWindow)) pointerWindow = findGameWindow(true);
    RECT client{};
    POINT position{};
    if (pointerWindow && GetClientRect(pointerWindow, &client) && GetCursorPos(&position) &&
        ScreenToClient(pointerWindow, &position) &&
        client_pointer(static_cast<float>(position.x - client.left),
                       static_cast<float>(position.y - client.top),
                       static_cast<float>(client.right - client.left),
                       static_cast<float>(client.bottom - client.top), x, y, w, h)) return true;
    return realPointer(x, y, w, h);
}

// The legacy bridge already dispatches this tick on the CoreWindow UI thread.
// Attach the wheel listener and sample CoreWindow bounds on that same thread.
static HRESULT patchedTick(void* instance) {
    HRESULT result = realTick(instance);
    if (!overlayDisabled.load(std::memory_order_acquire)) SampleUwpInput();
    return result;
}

// Increase the default only. A saved profile may supply its own scale and the
// user can still change it in Settings -> UI Scale.
static void patchedSettingsCtor(ag::Settings* settings) {
    realSettingsCtor(settings);
    settings->uiScale = 1.20f;
}

static void draw(IDXGISwapChain* chain, UINT flags) {
    if (overlayDisabled.load(std::memory_order_acquire) || !chain || (flags & DXGI_PRESENT_TEST)) return;
    ID3D11Device* device = nullptr;
    ID3D11DeviceContext* context = nullptr;
    ID3D11Texture2D* back = nullptr;
    if (SUCCEEDED(chain->GetDevice(__uuidof(ID3D11Device), reinterpret_cast<void**>(&device))) && device &&
        SUCCEEDED(chain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&back))) && back) {
        D3D11_TEXTURE2D_DESC desc{};
        back->GetDesc(&desc);
        if (desc.Width >= 320 && desc.Height >= 200) {
            device->GetImmediateContext(&context);
            if (context) {
                ObserveVisualContext(context);
                frameDevice = device; frameContext = context;
                frameWidth = desc.Width; frameHeight = desc.Height; seenInput = false; pointerInUse = false;
                legacyPresent(chain, 0, flags);
                release(previewFrameView);
                frameDevice = nullptr; frameContext = nullptr;
            }
        }
    }
    release(back); release(context); release(device);
}
static HRESULT STDMETHODCALLTYPE hookPresent(IDXGISwapChain* chain, UINT sync, UINT flags) {
    if (insidePresent) return realPresent(chain, sync, flags);
    insidePresent = true;
    draw(chain, flags);
    HRESULT result = realPresent(chain, sync, flags);
    insidePresent = false;
    return result;
}
static HRESULT STDMETHODCALLTYPE hookPresent1(IDXGISwapChain1* chain, UINT sync, UINT flags, const DXGI_PRESENT_PARAMETERS* params) {
    if (insidePresent) return realPresent1(chain, sync, flags, params);
    insidePresent = true;
    draw(chain, flags);
    HRESULT result = realPresent1(chain, sync, flags, params);
    insidePresent = false;
    return result;
}

static bool findSwapChainMethods(void*& present, void*& present1) {
    ID3D11Device* device = nullptr;
    ID3D11DeviceContext* context = nullptr;
    IDXGIDevice* dxgiDevice = nullptr;
    IDXGIAdapter* adapter = nullptr;
    IDXGIFactory2* factory = nullptr;
    IDXGISwapChain1* chain = nullptr;
    D3D_FEATURE_LEVEL level{};
    HRESULT hr = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr,
        D3D11_CREATE_DEVICE_BGRA_SUPPORT, nullptr, 0, D3D11_SDK_VERSION, &device, &level, &context);
    if (SUCCEEDED(hr)) hr = device->QueryInterface(__uuidof(IDXGIDevice), reinterpret_cast<void**>(&dxgiDevice));
    if (SUCCEEDED(hr)) hr = dxgiDevice->GetAdapter(&adapter);
    if (SUCCEEDED(hr)) hr = adapter->GetParent(__uuidof(IDXGIFactory2), reinterpret_cast<void**>(&factory));
    if (SUCCEEDED(hr)) {
        DXGI_SWAP_CHAIN_DESC1 desc{};
        desc.Width = 2; desc.Height = 2; desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
        desc.SampleDesc.Count = 1; desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        desc.BufferCount = 2; desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
        desc.AlphaMode = DXGI_ALPHA_MODE_PREMULTIPLIED;
        hr = factory->CreateSwapChainForComposition(device, &desc, nullptr, &chain);
    }
    if (SUCCEEDED(hr) && chain) {
        void** vtable = *reinterpret_cast<void***>(chain);
        present = vtable[8]; present1 = vtable[22];
    } else {
        char message[96]; std::snprintf(message, sizeof(message), "dummy DX11 swap chain failed: 0x%08lx", static_cast<unsigned long>(hr)); log(message);
    }
    release(chain); release(factory); release(adapter); release(dxgiDevice); release(context); release(device);
    return present && present1 && present != present1;
}

static DWORD WINAPI bootstrap(void*) {
    // Never unload this module while its detours are installed.
    HMODULE pinned = nullptr;
    if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_PIN,
                            reinterpret_cast<LPCWSTR>(&bootstrap), &pinned)) { log("could not pin bootstrap"); return 1; }
    wchar_t path[32768]{};
    DWORD n = GetModuleFileNameW(selfModule, path, static_cast<DWORD>(sizeof(path) / sizeof(path[0])));
    if (!n || n >= sizeof(path) / sizeof(path[0])) { log("cannot find bootstrap path"); return 1; }
    wchar_t* slash = wcsrchr(path, L'\\');
    if (!slash || static_cast<size_t>(slash - path) + 20 >= sizeof(path) / sizeof(path[0])) { log("invalid bootstrap path"); return 1; }
    wcscpy(slash + 1, L"GAYClientCore.dll");
    core = LoadLibraryW(path);
    if (!core) { log("cannot load GAYClientCore.dll; check path and AppContainer access"); return 1; }
    auto mhInit = symbol<InitFn>("MH_Initialize");
    auto mhCreate = symbol<HookFn>("MH_CreateHook");
    auto mhQueueEnable = symbol<ToggleFn>("MH_QueueEnableHook");
    auto mhApply = symbol<InitFn>("MH_ApplyQueued");
    auto mhDisable = symbol<ToggleFn>("MH_DisableHook");
    auto mhEnable = symbol<ToggleFn>("MH_EnableHook");
    auto mhRemove = symbol<ToggleFn>("MH_RemoveHook");
    auto bridgeInit = symbol<BridgeInitFn>("_ZN2ag19InitializeUwpBridgeEv");
    bridgeActive = symbol<BridgeActiveFn>("_ZN2ag17SetUwpInputActiveEb");
    void* newFrameTarget = reinterpret_cast<void*>(GetProcAddress(core, "_Z23ImGui_ImplDX11_NewFramev"));
    void* updateTarget = reinterpret_cast<void*>(GetProcAddress(core, "_ZN2ag10InputState6UpdateERKNS_8SettingsEbb"));
    void* pointerTarget = reinterpret_cast<void*>(GetProcAddress(core, "_ZN2ag10UwpPointerERfS0_S0_S0_"));
    void* settingsTarget = reinterpret_cast<void*>(GetProcAddress(core, "_ZN2ag8SettingsC1Ev"));
    void* watchedTarget = reinterpret_cast<void*>(GetProcAddress(core, "_ZN2ag17UwpSetWatchedKeysERKSt5arrayIbLy256EE"));
    keyDown = symbol<KeyDownFn>("_ZN2ag10UwpKeyDownEi");
    void* textTarget = reinterpret_cast<void*>(GetProcAddress(core, "_ZN5ImGui15TextUnformattedEPKcS1_"));
    void* drawGuiTarget = reinterpret_cast<void*>(GetProcAddress(core, "_ZN2ag12DrawClickGuiERNS_7RuntimeE"));
    setModule = symbol<SetModuleFn>("_ZN2ag7Runtime9SetModuleENS_8ModuleIdEb");
    guiBegin = symbol<BeginFn>("_ZN5ImGui5BeginEPKcPbi");
    guiEnd = symbol<EndFn>("_ZN5ImGui3EndEv");
    guiCheckbox = symbol<CheckboxFn>("_ZN5ImGui8CheckboxEPKcPb");
    guiSliderFloat = symbol<SliderFloatFn>("_ZN5ImGui11SliderFloatEPKcPfffS1_i");
    guiSetNextSize = symbol<SetNextSizeFn>("_ZN5ImGui17SetNextWindowSizeERK6ImVec2i");
    guiBeginTabBar = symbol<BeginTabBarFn>("_ZN5ImGui11BeginTabBarEPKci");
    guiEndTabBar = symbol<EndTabFn>("_ZN5ImGui9EndTabBarEv");
    guiBeginTabItem = symbol<BeginTabItemFn>("_ZN5ImGui12BeginTabItemEPKcPbi");
    guiEndTabItem = symbol<EndTabFn>("_ZN5ImGui10EndTabItemEv");
    guiColorEdit3 = symbol<ColorEdit3Fn>("_ZN5ImGui10ColorEdit3EPKcPfi");
    guiPushColor = symbol<PushColorFn>("_ZN5ImGui14PushStyleColorEiRK6ImVec4");
    guiPopColor = symbol<PopColorFn>("_ZN5ImGui13PopStyleColorEi");
    guiFontScale = symbol<FontScaleFn>("_ZN5ImGui18SetWindowFontScaleEf");
    guiButton = symbol<ButtonFn>("_ZN5ImGui6ButtonEPKcRK6ImVec2");
    guiPushWidth = symbol<PushWidthFn>("_ZN5ImGui13PushItemWidthEf");
    guiPopWidth = symbol<PopWidthFn>("_ZN5ImGui12PopItemWidthEv");
    guiDrawList = symbol<GetDrawListFn>("_ZN5ImGui17GetWindowDrawListEv");
    guiCursor = symbol<GetCursorFn>("_ZN5ImGui18GetCursorScreenPosEv");
    guiDummy = symbol<DummyFn>("_ZN5ImGui5DummyERK6ImVec2");
    drawLine = symbol<DrawLineFn>("_ZN10ImDrawList7AddLineERK6ImVec2S2_jf");
    drawTriangle = symbol<DrawTriangleFn>("_ZN10ImDrawList17AddTriangleFilledERK6ImVec2S2_S2_j");
    drawQuad = symbol<DrawQuadFn>("_ZN10ImDrawList13AddQuadFilledERK6ImVec2S2_S2_S2_j");
    drawRect = symbol<DrawRectFn>("_ZN10ImDrawList13AddRectFilledERK6ImVec2S2_jfi");
    drawGradientRect = symbol<DrawGradientRectFn>("_ZN10ImDrawList23AddRectFilledMultiColorERK6ImVec2S2_jjjj");
    drawImage = symbol<DrawImageFn>("_ZN10ImDrawList8AddImageEyRK6ImVec2S2_S2_S2_j");
    drawImageQuad = symbol<DrawImageQuadFn>("_ZN10ImDrawList12AddImageQuadEyRK6ImVec2S2_S2_S2_S2_S2_S2_S2_j");
    guiPushVarFloat = symbol<PushVarFloatFn>("_ZN5ImGui12PushStyleVarEif");
    guiPopVar = symbol<PopVarFn>("_ZN5ImGui11PopStyleVarEi");
    guiSameLine = symbol<SameLineFn>("_ZN5ImGui8SameLineEff");
    guiPushTextWrap = symbol<PushTextWrapFn>("_ZN5ImGui15PushTextWrapPosEf");
    guiPopTextWrap = symbol<PopTextWrapFn>("_ZN5ImGui14PopTextWrapPosEv");
    guiSetNextPos = symbol<SetNextPosFn>("_ZN5ImGui16SetNextWindowPosERK6ImVec2iS2_");
    findGameWindow = symbol<FindWindowFn>("_ZN2ag19FindMinecraftWindowEb");
    addFont = symbol<AddFontFn>("_ZN2ag15AddEmbeddedFontEfb");
    addWheel = symbol<AddWheelFn>("_ZN7ImGuiIO18AddMouseWheelEventEff");
    // This non-exported dispatcher callback is specific to the documented core
    // build. Verify its known prologue before treating its RVA as code.
    auto* tickTarget = reinterpret_cast<unsigned char*>(core) + 0x2060;
    static const unsigned char expectedTick[] = {0x41,0x55,0x41,0x54,0x55,0x57,0x56,0x53,0x48,0x83,0xec,0x68};
    bool tickHook = std::memcmp(tickTarget, expectedTick, sizeof(expectedTick)) == 0;
    if (!tickHook) log("unsupported core input callback; using legacy input only");
    legacyPresent = symbol<LegacyPresentFn>("_Z11PresentHookP14IDXGISwapChainjj");
    rendererInit = symbol<RendererInitFn>("_Z19ImGui_ImplDX11_InitP12ID3D11DeviceP19ID3D11DeviceContext");
    getIO = symbol<GetIOFn>("_ZN5ImGui5GetIOEv");
    if (!mhInit || !mhCreate || !mhQueueEnable || !mhApply || !mhDisable || !bridgeInit || !bridgeActive || !newFrameTarget || !updateTarget || !pointerTarget || !settingsTarget || !findGameWindow || !addFont || !addWheel || !legacyPresent || !rendererInit || !getIO) { log("legacy DLL exports do not match expected version"); return 1; }
    log(suppressMouseDiagnostic() ? "mouse diagnostic suppressed" : "mouse diagnostic patch skipped: core mismatch or protection failure");
    void* present = nullptr; void* present1 = nullptr;
    if (!findSwapChainMethods(present, present1)) return 1;
    if (mhInit() != 0) { log("MinHook initialization failed"); return 1; }
    if (mhCreate(newFrameTarget, reinterpret_cast<void*>(&patchedNewFrame), reinterpret_cast<void**>(&realNewFrame)) != 0 ||
        mhCreate(updateTarget, reinterpret_cast<void*>(&patchedUpdate), reinterpret_cast<void**>(&realUpdate)) != 0 ||
        mhCreate(pointerTarget, reinterpret_cast<void*>(&patchedPointer), reinterpret_cast<void**>(&realPointer)) != 0 ||
        mhCreate(settingsTarget, reinterpret_cast<void*>(&patchedSettingsCtor), reinterpret_cast<void**>(&realSettingsCtor)) != 0 ||
        mhCreate(present, reinterpret_cast<void*>(&hookPresent), reinterpret_cast<void**>(&realPresent)) != 0 ||
        mhCreate(present1, reinterpret_cast<void*>(&hookPresent1), reinterpret_cast<void**>(&realPresent1)) != 0) { log("hook creation failed"); return 1; }
    bool coordHooks = textTarget && drawGuiTarget && setModule && guiBegin && guiEnd && guiCheckbox && guiSetNextPos &&
        mhCreate(textTarget, reinterpret_cast<void*>(&patchedText), reinterpret_cast<void**>(&realText)) == 0 &&
        mhCreate(drawGuiTarget, reinterpret_cast<void*>(&patchedDrawGui), reinterpret_cast<void**>(&realDrawGui)) == 0;
    if (!coordHooks) log("coordinate display hooks unavailable");
    void* hudTarget = reinterpret_cast<void*>(GetProcAddress(core,"_ZN2ag7DrawHudERNS_7RuntimeE"));
    void* notifyTarget = reinterpret_cast<void*>(GetProcAddress(core,"_ZN2ag17NotificationQueue4DrawERKNS_8SettingsE"));
    // Optional HUD hooks are enabled only after the essential overlay is live;
    // failure must not prevent the verified overlay from starting.
    const bool hudHookCreated = coordHooks && mhEnable && mhRemove && hudTarget && guiPushColor && guiPopColor && guiFontScale &&
        mhCreate(hudTarget,reinterpret_cast<void*>(&patchedDrawHud),reinterpret_cast<void**>(&realDrawHud))==0;
    bool pageKeyHook = watchedTarget && keyDown &&
        mhCreate(watchedTarget, reinterpret_cast<void*>(&patchedWatchedKeys), reinterpret_cast<void**>(&realWatchedKeys)) == 0;
    pageKeysReady = pageKeyHook;
    if (!pageKeyHook) log("PageUp/PageDown fallback unavailable");
    if (tickHook && mhCreate(tickTarget, reinterpret_cast<void*>(&patchedTick), reinterpret_cast<void**>(&realTick)) != 0) {
        tickHook = false;
        log("UWP input hook unavailable; using legacy input only");
    }
    QueryPerformanceFrequency(&frequency);
    bridgeInit();
    bridgeActive(true);
    if (mhQueueEnable(newFrameTarget) || mhQueueEnable(updateTarget) ||
        mhQueueEnable(pointerTarget) || mhQueueEnable(settingsTarget) || (pageKeyHook && mhQueueEnable(watchedTarget)) || (tickHook && mhQueueEnable(tickTarget)) ||
        (coordHooks && (mhQueueEnable(textTarget) || mhQueueEnable(drawGuiTarget))) ||
        mhQueueEnable(present) || mhQueueEnable(present1) || mhApply()) {
        mhDisable(present1); mhDisable(present); if (hudHookCreated) mhRemove(hudTarget); if (coordHooks) { mhDisable(textTarget); mhDisable(drawGuiTarget); }
        if (tickHook) mhDisable(tickTarget); if (pageKeyHook) mhDisable(watchedTarget); mhDisable(settingsTarget);
        mhDisable(pointerTarget); mhDisable(updateTarget); mhDisable(newFrameTarget);
        log("hook enable failed; attempted to disable hooks"); return 1;
    }
    log("hooks enabled");
    if (hudHookCreated) {
        if (mhEnable(hudTarget)==0) hudHooksReady.store(true,std::memory_order_release);
        else { mhRemove(hudTarget); log("HUD editor unavailable"); }
    }
    if (hudHooksReady && guiBegin && guiPushVarFloat && guiPopVar &&
        mhCreate(reinterpret_cast<void*>(guiBegin),reinterpret_cast<void*>(&themedBegin),reinterpret_cast<void**>(&realGuiBegin))==0) {
        if (mhEnable(reinterpret_cast<void*>(guiBegin))==0) hudThemeReady.store(true,std::memory_order_release);
        else { mhRemove(reinterpret_cast<void*>(guiBegin)); log("HUD theme unavailable; original widgets retained"); }
    }
    if (hudHooksReady && notifyTarget && mhCreate(notifyTarget,reinterpret_cast<void*>(&patchedNotificationDraw),reinterpret_cast<void**>(&realNotificationDraw))==0) {
        notificationHookReady = mhEnable(notifyTarget)==0;
        if (!notificationHookReady) mhRemove(notifyTarget);
    }
    // Hat is optional and must never block the already working overlay.
    // Private game-render hooks are deferred until the user checks the box.
    try {
        const bool verified = InitializeChinaHat(mhCreate,mhEnable,mhDisable,mhRemove);
        if (verified && coordHooks && mhEnable && mhRemove) {
            log(InitializeMenuMouse(mhCreate,mhEnable,mhDisable,mhRemove)
                ? "experimental menu mouse hooks enabled" : "menu mouse hooks unavailable; legacy input retained");
        }
    } catch (...) { log("game adapter initialization failed; overlay remains available"); }
    return 0;
}

BOOL WINAPI DllMain(HINSTANCE module, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        selfModule = module;
        DisableThreadLibraryCalls(module);
        HANDLE thread = CreateThread(nullptr, 0, bootstrap, nullptr, 0, nullptr);
        if (thread) CloseHandle(thread);
    }
    return TRUE;
}
