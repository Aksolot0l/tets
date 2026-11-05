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
#include "pointer_mapping.h"
#include "uwp_extra.h"
#include "page_scroll.h"
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
static PageScrollRepeat pageScroll;
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

// Request only two additional keys from the existing UWP polling bridge.
// No key or wheel activity is stored on disk.
static void patchedWatchedKeys(const std::array<bool, 256>& requested) {
    auto keys = requested;
    keys[VK_PRIOR] = true;
    keys[VK_NEXT] = true;
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
    SampleUwpInput();
    return result;
}

// Increase the default only. A saved profile may supply its own scale and the
// user can still change it in Settings -> UI Scale.
static void patchedSettingsCtor(ag::Settings* settings) {
    realSettingsCtor(settings);
    settings->uiScale = 1.20f;
}

static void draw(IDXGISwapChain* chain, UINT flags) {
    if (!chain || (flags & DXGI_PRESENT_TEST)) return;
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
    auto bridgeInit = symbol<BridgeInitFn>("_ZN2ag19InitializeUwpBridgeEv");
    auto bridgeActive = symbol<BridgeActiveFn>("_ZN2ag17SetUwpInputActiveEb");
    void* newFrameTarget = reinterpret_cast<void*>(GetProcAddress(core, "_Z23ImGui_ImplDX11_NewFramev"));
    void* updateTarget = reinterpret_cast<void*>(GetProcAddress(core, "_ZN2ag10InputState6UpdateERKNS_8SettingsEbb"));
    void* pointerTarget = reinterpret_cast<void*>(GetProcAddress(core, "_ZN2ag10UwpPointerERfS0_S0_S0_"));
    void* settingsTarget = reinterpret_cast<void*>(GetProcAddress(core, "_ZN2ag8SettingsC1Ev"));
    void* watchedTarget = reinterpret_cast<void*>(GetProcAddress(core, "_ZN2ag17UwpSetWatchedKeysERKSt5arrayIbLy256EE"));
    keyDown = symbol<KeyDownFn>("_ZN2ag10UwpKeyDownEi");
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
    void* present = nullptr; void* present1 = nullptr;
    if (!findSwapChainMethods(present, present1)) return 1;
    if (mhInit() != 0) { log("MinHook initialization failed"); return 1; }
    if (mhCreate(newFrameTarget, reinterpret_cast<void*>(&patchedNewFrame), reinterpret_cast<void**>(&realNewFrame)) != 0 ||
        mhCreate(updateTarget, reinterpret_cast<void*>(&patchedUpdate), reinterpret_cast<void**>(&realUpdate)) != 0 ||
        mhCreate(pointerTarget, reinterpret_cast<void*>(&patchedPointer), reinterpret_cast<void**>(&realPointer)) != 0 ||
        mhCreate(settingsTarget, reinterpret_cast<void*>(&patchedSettingsCtor), reinterpret_cast<void**>(&realSettingsCtor)) != 0 ||
        mhCreate(present, reinterpret_cast<void*>(&hookPresent), reinterpret_cast<void**>(&realPresent)) != 0 ||
        mhCreate(present1, reinterpret_cast<void*>(&hookPresent1), reinterpret_cast<void**>(&realPresent1)) != 0) { log("hook creation failed"); return 1; }
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
        mhQueueEnable(present) || mhQueueEnable(present1) || mhApply()) {
        mhDisable(present1); mhDisable(present); if (tickHook) mhDisable(tickTarget); if (pageKeyHook) mhDisable(watchedTarget); mhDisable(settingsTarget);
        mhDisable(pointerTarget); mhDisable(updateTarget); mhDisable(newFrameTarget);
        log("hook enable failed; attempted to disable hooks"); return 1;
    }
    log("hooks enabled");
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
