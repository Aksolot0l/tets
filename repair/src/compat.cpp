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
#include "pointer_mapping.h"
#include "uwp_extra.h"
#include "page_scroll.h"
#include "wheel_bridge.h"
#include "wheel_counter.h"
#include "coordinates_115.h"
#include "mouse_diagnostic_patch.h"
#include "disable_hotkey.h"
#include "china_hat_native.h"
#include "visual_preview.h"
#include "visual_color.h"
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
using PushVarFloatFn = void (*)(int,float);
using PopVarFn = void (*)(int);

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
static BeginFn guiBegin = nullptr;
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
static PushVarFloatFn guiPushVarFloat = nullptr;
static PopVarFn guiPopVar = nullptr;
enum class VisualEditor { None, Hat, Trail, Options };
static VisualEditor visualEditor=VisualEditor::None;
static std::atomic<bool> hudHooksReady{false}, notificationHookReady{false};
static bool hudEditorOpen = false, moduleHudEnabled = false;
static std::atomic<bool> hudEditorActive{false};
// New Module List HUD settings are per-session; they do not overwrite profiles.
static float hudScale = 1.f, hudOpacity = .78f, hudX = .78f, hudY = .05f;
static float hudColor[3] = {.35f,.85f,1.f};
static std::atomic<const WheelBridge*> externalWheel{nullptr};
static SRWLOCK externalWheelLock = SRWLOCK_INIT;
static HWND pointerWindow = nullptr;
static thread_local bool insidePresent = false;
static thread_local bool seenInput = false;
static thread_local bool pointerInUse = false;
static thread_local ID3D11Device* frameDevice = nullptr;
static thread_local ID3D11DeviceContext* frameContext = nullptr;
static thread_local UINT frameWidth = 0, frameHeight = 0;
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
static void drawVisualOptions() {
    bool tint=VisualTintEnabled();
    if (guiCheckbox("Enable colors",&tint)) SetVisualTintEnabled(tint);
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
    diagnostic("Before UI",VisualDepthBefore()); diagnostic("After UI",VisualDepthAfter());
    realText("Diagnostic only, not proof of depth.",nullptr);
    realText("Resets after game closes.",nullptr);
    realText("Fill: turn off to use lines.",nullptr);
}
// Match the dark/green Home and HUD panels, without modifying the closed core.
static void pushVisualStyle() {
    if (!guiPushColor || !guiPopColor) return;
    if (guiPushVarFloat && guiPopVar) {
        guiPushVarFloat(ImGuiStyleVar_WindowBorderSize,1.f);
        guiPushVarFloat(ImGuiStyleVar_WindowRounding,8.f);
    }
    guiPushColor(ImGuiCol_WindowBg,ImVec4(.035f,.065f,.055f,.96f));
    guiPushColor(ImGuiCol_Border,ImVec4(.10f,.65f,.37f,.85f));
    guiPushColor(ImGuiCol_TitleBg,ImVec4(.025f,.10f,.065f,.98f));
    guiPushColor(ImGuiCol_TitleBgActive,ImVec4(.025f,.14f,.08f,1.f));
    guiPushColor(ImGuiCol_Button,ImVec4(.02f,.22f,.11f,1.f));
    guiPushColor(ImGuiCol_ButtonHovered,ImVec4(.04f,.42f,.21f,1.f));
    guiPushColor(ImGuiCol_ButtonActive,ImVec4(.05f,.56f,.28f,1.f));
    guiPushColor(ImGuiCol_CheckMark,ImVec4(.08f,.78f,.42f,1.f));
    guiPushColor(ImGuiCol_SliderGrab,ImVec4(.08f,.72f,.38f,1.f));
    guiPushColor(ImGuiCol_SliderGrabActive,ImVec4(.10f,.92f,.49f,1.f));
}
static void popVisualStyle() {
    if (guiPushColor && guiPopColor) {
        guiPopColor(10); if (guiPushVarFloat && guiPopVar) guiPopVar(2);
    }
}
static bool canVisualPreview() {
    return hudHooksReady && guiButton && guiDrawList && guiCursor && guiDummy && drawLine && drawTriangle && drawQuad && drawRect;
}
static void drawVisualPanel() {
    guiSetNextPos(ImVec2(548.f,35.f), ImGuiCond_FirstUseEver, ImVec2(0,0));
    if (guiSetNextSize) guiSetNextSize(ImVec2(240.f,300.f),ImGuiCond_FirstUseEver);
    pushVisualStyle();
    if (guiBegin("Visuals",nullptr,ImGuiWindowFlags_NoSavedSettings|ImGuiWindowFlags_NoCollapse)) {
        if (!ChinaHatAvailable()) realText("Visuals unavailable",nullptr);
        else {
            bool hat=ChinaHatEnabled(); if (guiCheckbox("China Hat",&hat)) SetChinaHatEnabled(hat);
            if (canVisualPreview() && guiButton("Settings##open_hat",ImVec2(-1.f,0.f))) visualEditor=VisualEditor::Hat;
            bool trace=TrailEnabled(); if (guiCheckbox("Trail",&trace)) SetTrailEnabled(trace);
            if (canVisualPreview() && guiButton("Settings##open_trail",ImVec2(-1.f,0.f))) visualEditor=VisualEditor::Trail;
            if (canVisualPreview() && guiButton("Options",ImVec2(-1.f,0.f))) visualEditor=VisualEditor::Options;
            if (!canVisualPreview()) {
                realText("Preview unavailable; inline settings:",nullptr);
                drawHatSettings(); drawTrailSettings(); drawVisualOptions();
            }
        }
    }
    guiEnd(); popVisualStyle();
}

static ImU32 previewColor(bool hat,float phase,float turn,std::uint64_t now) {
    const bool tint=VisualTintEnabled(), rainbow=hat?HatRainbowEnabled():TrailRainbowEnabled();
    const float opacity=GetVisualParam(hat?VisualParam::HatOpacity:VisualParam::TrailOpacity);
    auto rgb=visual_color::animated(tint,rainbow,
      hat?std::array<float,3>{{GetVisualParam(VisualParam::HatRed),GetVisualParam(VisualParam::HatGreen),GetVisualParam(VisualParam::HatBlue)}}:
          std::array<float,3>{{GetVisualParam(VisualParam::TrailRed),GetVisualParam(VisualParam::TrailGreen),GetVisualParam(VisualParam::TrailBlue)}},
      GetVisualParam(hat?VisualParam::HatRainbowSpeed:VisualParam::TrailRainbowSpeed),now);
    if (tint && rainbow && VertexGradientEnabled()) {
        auto c=china_hat::rainbow(phase+turn*.8f,255); rgb={{c.r/255.f,c.g/255.f,c.b/255.f,1.f}};
    }
    return IM_COL32(static_cast<int>(rgb[0]*255),static_cast<int>(rgb[1]*255),static_cast<int>(rgb[2]*255),static_cast<int>(opacity*255));
}
static void drawEffectPreview(VisualEditor editor) {
    if (!canVisualPreview()) return;
    guiSetNextPos(ImVec2(18.f,90.f),ImGuiCond_Always,ImVec2(0,0));
    if (guiSetNextSize) guiSetNextSize(ImVec2(310.f,310.f),ImGuiCond_FirstUseEver);
    pushVisualStyle();
    if (guiBegin("Preview##effect",nullptr,ImGuiWindowFlags_NoSavedSettings|ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoCollapse)) {
        realText("Animated illustration",nullptr);
        auto* d=guiDrawList(); ImVec2 o=guiCursor(); const auto now=GetTickCount64();
        auto point=[&](float x,float y) { return ImVec2(o.x+x,o.y+y); };
        if (d) {
            // Stylized player; not an in-game screenshot or the game's shader.
            drawRect(d,point(116,145),point(164,205),IM_COL32(105,112,117,255),4.f,0);
            drawRect(d,point(123,105),point(157,140),IM_COL32(170,176,177,255),4.f,0);
            if (editor==VisualEditor::Hat) {
                const float y=visual_preview::hatY(now,GetVisualParam(VisualParam::HatOffset),GetVisualParam(VisualParam::HatBobAmplitude),GetVisualParam(VisualParam::HatBobSpeed));
                const float r=GetVisualParam(VisualParam::HatRadius), h=GetVisualParam(VisualParam::HatConeHeight);
                const float phase=visual_gradient::phase(now,GetVisualParam(VisualParam::HatRainbowSpeed));
                const ImVec2 apex=point(140.f,y-h*95.f);
                for (int i=0;i<48;++i) {
                    auto a=visual_preview::hatRim(i/48.f,r,y), b=visual_preview::hatRim((i+1)/48.f,r,y);
                    const ImU32 c=previewColor(true,phase,i/48.f,now);
                    if (HatFillEnabled()) drawTriangle(d,apex,point(a.x,a.y),point(b.x,b.y),c);
                    drawLine(d,point(a.x,a.y),point(b.x,b.y),c,1.5f);
                    if (i%4==0) drawLine(d,apex,point(a.x,a.y),c,1.f);
                }
            } else if (editor==VisualEditor::Trail) {
                const float phase=visual_gradient::phase(now,GetVisualParam(VisualParam::TrailRainbowSpeed));
                const float h=GetVisualParam(VisualParam::TrailHeight)*82.f;
                for (int i=0;i<24;++i) {
                    float t=i/24.f, t2=(i+1)/24.f;
                    auto a=visual_preview::trailPoint(t,now,GetVisualParam(VisualParam::TrailSeconds));
                    auto b=visual_preview::trailPoint(t2,now,GetVisualParam(VisualParam::TrailSeconds));
                    ImU32 c=previewColor(false,phase,t,now);
                    if (TrailFillEnabled()) drawQuad(d,point(a.x,a.y),point(b.x,b.y),point(b.x,b.y-h),point(a.x,a.y-h),c);
                    drawLine(d,point(a.x,a.y),point(b.x,b.y),c,2.f);
                }
            }
        }
        guiDummy(ImVec2(280.f,220.f));
        realText("Game material may look different.",nullptr);
    }
    guiEnd(); popVisualStyle();
}
static void drawVisualEditor() {
    const VisualEditor current=visualEditor;
    // Draw settings before the preview so edits are reflected in the same frame.
    guiSetNextPos(ImVec2(350.f,90.f),ImGuiCond_Always,ImVec2(0,0));
    if (guiSetNextSize) guiSetNextSize(ImVec2(310.f,430.f),ImGuiCond_FirstUseEver);
    pushVisualStyle();
    const char* title=current==VisualEditor::Hat?"China Hat settings":current==VisualEditor::Trail?"Trail settings":"Visual options";
    if (guiBegin(title,nullptr,ImGuiWindowFlags_NoSavedSettings|ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoCollapse)) {
        if (guiButton && guiButton("Back to menu",ImVec2(-1.f,0.f))) visualEditor=VisualEditor::None;
        if (current==VisualEditor::Hat) drawHatSettings();
        else if (current==VisualEditor::Trail) drawTrailSettings();
        else drawVisualOptions();
    }
    guiEnd(); popVisualStyle();
    if (visualEditor==current && current!=VisualEditor::Options) drawEffectPreview(current);
}

// Draw exactly the same new Module List card in the editor and during play.
// Existing legacy HUD widgets are not restyled by these settings.
static void drawModuleHud(void* runtime, bool preview) {
    if (!runtime || !guiPushColor || !guiPopColor || !guiFontScale) return;
    auto* s = reinterpret_cast<ag::Settings*>(reinterpret_cast<unsigned char*>(runtime)+0x48);
    // Position is scaled to a small preview area so X/Y sliders also give
    // immediate feedback without covering the editor itself.
    const float x = preview ? 390.f+hudX*180.f : hudX*static_cast<float>(frameWidth);
    const float y = preview ? 90.f+hudY*180.f : hudY*static_cast<float>(frameHeight);
    guiSetNextPos(ImVec2(x,y),ImGuiCond_Always,ImVec2(0,0));
    guiPushColor(ImGuiCol_WindowBg,ImVec4(.045f,.055f,.085f,hudOpacity));
    guiPushColor(ImGuiCol_Text,ImVec4(hudColor[0],hudColor[1],hudColor[2],1.f));
    const int flags=ImGuiWindowFlags_NoTitleBar|ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoMove|
        ImGuiWindowFlags_NoInputs|ImGuiWindowFlags_AlwaysAutoResize|ImGuiWindowFlags_NoSavedSettings|
        ImGuiWindowFlags_NoFocusOnAppearing;
    if (guiBegin(preview?"##module_hud_preview":"##module_hud",nullptr,flags)) {
        guiFontScale(hudScale);
        realText("MODULES",nullptr);
        module_hud::labels(s->enabled,ChinaHatEnabled(),TrailEnabled(),[](const char* label) { realText(label,nullptr); });
    }
    guiEnd(); guiPopColor(2);
}
static void patchedDrawHud(void* runtime) {
    const bool menu=runtime && *(reinterpret_cast<unsigned char*>(runtime)+0x551);
    const bool editing=module_hud::editing(menu,hudEditorOpen || visualEditor!=VisualEditor::None);
    hudEditorActive.store(editing,std::memory_order_release);
    if (!editing) {
        realDrawHud(runtime);
        if (moduleHudEnabled) drawModuleHud(runtime,false);
    }
}
static void patchedNotificationDraw(void* queue, const ag::Settings& s) {
    if (!hudEditorActive.load(std::memory_order_acquire)) realNotificationDraw(queue,s);
}
static void drawHudEditor(void* runtime) {
    guiSetNextPos(ImVec2(18.f,90.f),ImGuiCond_Always,ImVec2(0,0));
    if (guiSetNextSize) guiSetNextSize(ImVec2(350.f,350.f),ImGuiCond_FirstUseEver);
    if (guiBegin("HUD editor",nullptr,ImGuiWindowFlags_NoSavedSettings|ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoCollapse)) {
        guiCheckbox("Open editor",&hudEditorOpen);
        guiCheckbox("Module List HUD",&moduleHudEnabled);
        auto slider=[](const char* label,float* value,float low,float high) {
            if (!guiSliderFloat) return;
            controlLabel(label); char id[128]{}; std::snprintf(id,sizeof(id),"##hud_%s",label);
            if (guiPushWidth && guiPopWidth) guiPushWidth(-1.f);
            guiSliderFloat(id,value,low,high,"%.2f",0);
            if (guiPushWidth && guiPopWidth) guiPopWidth();
        };
        slider("Scale",&hudScale,.7f,1.6f);
        slider("Background opacity",&hudOpacity,0.f,1.f);
        slider("Position X",&hudX,0.f,.9f);
        slider("Position Y",&hudY,0.f,.9f);
        if (guiColorEdit3) { realText("Text color",nullptr); if (guiPushWidth && guiPopWidth) guiPushWidth(-1.f);
            guiColorEdit3("##hud_color",hudColor,0); if (guiPushWidth && guiPopWidth) guiPopWidth(); }
        realText("Close editor to return.",nullptr);
        realText("Settings reset after game exit.",nullptr);
        if (!notificationHookReady) realText("Notifications may remain visible.",nullptr);
    }
    guiEnd();
    if (hudEditorOpen) drawModuleHud(runtime,true);
}
static void drawHudEntry() {
    if (!hudHooksReady) return;
    guiSetNextPos(ImVec2(18.f,130.f),ImGuiCond_FirstUseEver,ImVec2(0,0));
    if (guiBegin("HUD",nullptr,ImGuiWindowFlags_AlwaysAutoResize|ImGuiWindowFlags_NoSavedSettings|ImGuiWindowFlags_NoCollapse))
        guiCheckbox("Open Module List editor",&hudEditorOpen);
    guiEnd();
}

static void patchedDrawGui(void* runtime) {
    // The old loop never calls RequestUnload for Delete. This is an explicit
    // one-way overlay stop, NOT a DLL unload: callbacks and hooks remain pinned.
    bool down = pageKeysReady && keyDown && keyDown(VK_DELETE);
    bool editing = getIO && getIO().WantTextInput;
    if (disableHotkey.update(down, down && gameIsForeground(), editing)) {
        overlayDisabled.store(true, std::memory_order_release);
        SetChinaHatEnabled(false);
        SetTrailEnabled(false);
        if (bridgeActive) bridgeActive(false);
        DisableUwpExtraInput();
        log("overlay disabled by Delete; DLLs remain loaded until game exit");
        return;
    }
    const bool menuOpen=runtime && *(reinterpret_cast<unsigned char*>(runtime)+0x551);
    if (!menuOpen) { hudEditorOpen=false; visualEditor=VisualEditor::None; }
    const bool editingVisual=menuOpen && visualEditor!=VisualEditor::None && canVisualPreview();
    if (!menuOpen || (!hudEditorOpen && !editingVisual)) realDrawGui(runtime);
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
    guiPushVarFloat = symbol<PushVarFloatFn>("_ZN5ImGui12PushStyleVarEif");
    guiPopVar = symbol<PopVarFn>("_ZN5ImGui11PopStyleVarEi");
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
    if (hudHooksReady && notifyTarget && mhCreate(notifyTarget,reinterpret_cast<void*>(&patchedNotificationDraw),reinterpret_cast<void**>(&realNotificationDraw))==0) {
        notificationHookReady = mhEnable(notifyTarget)==0;
        if (!notificationHookReady) mhRemove(notifyTarget);
    }
    // Hat is optional and must never block the already working overlay.
    // Private game-render hooks are deferred until the user checks the box.
    try { InitializeChinaHat(mhCreate,mhEnable,mhDisable,mhRemove); }
    catch (...) { log("China Hat initialization failed; overlay remains available"); }
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
