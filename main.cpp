// main.cpp — демонстрационный запуск GAYClientMenu на DirectX 11.
// Игровой рендер отсутствует: окно очищается чёрным цветом, меню рисуется
// поверх него. В существующем клиенте этот файл не нужен — достаточно
// вызывать gayclient::RenderMenu() внутри уже идущего кадра ImGui.

#include "GAYClientMenu.h"

#include "imgui.h"
#include "backends/imgui_impl_win32.h"
#include "backends/imgui_impl_dx11.h"

#include <d3d11.h>
#include <tchar.h>
#include <windows.h>

#include <cstdio>
#include <string>

// ─────────────────────────────────────────────────────────────────────────────
// DirectX 11
// ─────────────────────────────────────────────────────────────────────────────
static ID3D11Device* g_device = nullptr;
static ID3D11DeviceContext* g_context = nullptr;
static IDXGISwapChain* g_swapChain = nullptr;
static UINT g_resizeWidth = 0;
static UINT g_resizeHeight = 0;
static ID3D11RenderTargetView* g_renderTarget = nullptr;

static void CreateRenderTarget()
{
    ID3D11Texture2D* backBuffer = nullptr;
    if (SUCCEEDED(g_swapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer))) && backBuffer != nullptr)
    {
        g_device->CreateRenderTargetView(backBuffer, nullptr, &g_renderTarget);
        backBuffer->Release();
    }
}

static void CleanupRenderTarget()
{
    if (g_renderTarget != nullptr)
    {
        g_renderTarget->Release();
        g_renderTarget = nullptr;
    }
}

static bool CreateDeviceD3D(HWND hWnd)
{
    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory(&sd, sizeof(sd));
    sd.BufferCount = 2;
    sd.BufferDesc.Width = 0;
    sd.BufferDesc.Height = 0;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    const D3D_FEATURE_LEVEL featureLevels[] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0 };
    D3D_FEATURE_LEVEL obtainedLevel = D3D_FEATURE_LEVEL_10_0;
    const HRESULT result = D3D11CreateDeviceAndSwapChain(
        nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, featureLevels, 2, D3D11_SDK_VERSION,
        &sd, &g_swapChain, &g_device, &obtainedLevel, &g_context);
    if (FAILED(result))
        return false;

    CreateRenderTarget();
    return true;
}

static void CleanupDeviceD3D()
{
    CleanupRenderTarget();
    if (g_swapChain != nullptr) { g_swapChain->Release(); g_swapChain = nullptr; }
    if (g_context != nullptr) { g_context->Release(); g_context = nullptr; }
    if (g_device != nullptr) { g_device->Release(); g_device = nullptr; }
}

static LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg)
    {
    case WM_SIZE:
        if (wParam == SIZE_MINIMIZED)
            return 0;
        g_resizeWidth = static_cast<UINT>(LOWORD(lParam));
        g_resizeHeight = static_cast<UINT>(HIWORD(lParam));
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU)
            return 0;
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    default:
        break;
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}

// ─────────────────────────────────────────────────────────────────────────────
// Демонстрационное состояние и действия.
// В настоящем клиенте эти колбэки должны вызывать его собственные функции.
// ─────────────────────────────────────────────────────────────────────────────
static gayclient::MenuState g_menuState;
static gayclient::MenuActions g_menuActions;

static void ResetHudDemo()
{
    const gayclient::MenuState defaults;
    g_menuState.hud = defaults.hud;
}

static void ResetInterfaceDemo()
{
    const gayclient::MenuState defaults;
    g_menuState.ui = defaults.ui;
}

static void ResetGraphicsDemo()
{
    const gayclient::MenuState defaults;
    g_menuState.graphics = defaults.graphics;
}

static void ResetSoundDemo()
{
    const gayclient::MenuState defaults;
    g_menuState.sound = defaults.sound;
}

static void ResetControlsDemo()
{
    const gayclient::MenuState defaults;
    g_menuState.controls = defaults.controls;
}

static void ResetAllDemo()
{
    const bool visible = g_menuState.visible;
    const gayclient::Tab tab = g_menuState.tab;
    const gayclient::Section section = g_menuState.section;
    const auto profiles = g_menuState.profiles;
    const int selectedProfile = g_menuState.selectedProfile;

    g_menuState = gayclient::MenuState{};
    g_menuState.visible = visible;
    g_menuState.tab = tab;
    g_menuState.section = section;
    g_menuState.profiles = profiles;
    g_menuState.selectedProfile = selectedProfile;
}

static void WireDemoActions()
{
    using namespace gayclient;

    // Здесь клиент получает все изменения. Пример — вывод в отладочный лог.
    g_menuActions.settingChanged = [](const char* path)
    {
        std::string message = "[GAYClient UI] setting changed: ";
        message += (path != nullptr ? path : "");
        message += "\n";
        OutputDebugStringA(message.c_str());
    };

    g_menuActions.tabChanged = [](Tab tab)
    {
        (void)tab;
        OutputDebugStringA("[GAYClient UI] tab changed\n");
    };

    g_menuActions.sectionChanged = [](Section section)
    {
        (void)section;
        OutputDebugStringA("[GAYClient UI] section changed\n");
    };

    g_menuActions.moduleToggled = [](ModuleId id, bool enabled)
    {
        char buffer[96];
        std::snprintf(buffer, sizeof(buffer), "[GAYClient UI] module %d -> %s\n",
                      static_cast<int>(id), enabled ? "on" : "off");
        OutputDebugStringA(buffer);
    };

    g_menuActions.profileSelected = [](int index) { (void)index; };
    g_menuActions.profileActivated = [](int index) { (void)index; };
    g_menuActions.profileCreated = [](const char* name) { (void)name; };
    g_menuActions.profileRenamed = [](int index, const char* name) { (void)index; (void)name; };
    g_menuActions.profileDeleted = [](int index) { (void)index; };
    g_menuActions.profileSaveRequested = [](int index) { (void)index; };

    g_menuActions.closeRequested = [] { g_menuState.visible = false; };
    g_menuActions.hudEditorRequested = [] { OutputDebugStringA("[GAYClient UI] HUD editor requested\n"); };
    g_menuActions.previewTextureRequested = []
    {
        // Клиент должен передать свою текстуру:
        // g_menuState.preview.texture = (ImTextureID)myShaderResourceView;
        // g_menuState.preview.textureSize = ImVec2(width, height);
        // g_menuState.preview.label = "Текстура клиента";
        OutputDebugStringA("[GAYClient UI] preview texture requested\n");
    };

    g_menuActions.resetHudRequested = ResetHudDemo;
    g_menuActions.resetInterfaceRequested = ResetInterfaceDemo;
    g_menuActions.resetGraphicsRequested = ResetGraphicsDemo;
    g_menuActions.resetSoundRequested = ResetSoundDemo;
    g_menuActions.resetControlsRequested = ResetControlsDemo;
    g_menuActions.resetAllRequested = ResetAllDemo;
    g_menuActions.statsRefreshRequested = []
    {
        OutputDebugStringA("[GAYClient UI] stats refresh requested (no data source)\n");
    };
}

// ─────────────────────────────────────────────────────────────────────────────
int main(int, char**)
{
    WNDCLASSEXW wc = { sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(nullptr), nullptr, nullptr,
                       nullptr, nullptr, L"GAYClientMenuDemo", nullptr };
    ::RegisterClassExW(&wc);
    HWND hwnd = ::CreateWindowW(wc.lpszClassName, L"GAYClient 1.1.5 — визуальный интерфейс (демо)",
                                WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU,
                                CW_USEDEFAULT, CW_USEDEFAULT, 1680, 1000,
                                nullptr, nullptr, wc.hInstance, nullptr);

    if (!CreateDeviceD3D(hwnd))
    {
        CleanupDeviceD3D();
        ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
        ::MessageBoxW(hwnd, L"Не удалось создать устройство DirectX 11.", L"GAYClient", MB_OK | MB_ICONERROR);
        return 1;
    }

    ::ShowWindow(hwnd, SW_SHOWDEFAULT);
    ::UpdateWindow(hwnd);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.IniFilename = nullptr;

    // Кириллица: один раз до инициализации бэкенда.
    g_menuState.cyrillicFontLoaded = gayclient::LoadCyrillicFont(io, 17.0f);

    ImGui::StyleColorsDark();
    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(g_device, g_context);
    WireDemoActions();

    bool running = true;
    while (running)
    {
        MSG msg;
        while (::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE))
        {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
            if (msg.message == WM_QUIT)
                running = false;
        }
        if (!running)
            break;

        if (g_resizeWidth != 0 && g_resizeHeight != 0)
        {
            CleanupRenderTarget();
            g_swapChain->ResizeBuffers(0, g_resizeWidth, g_resizeHeight, DXGI_FORMAT_UNKNOWN, 0);
            g_resizeWidth = 0;
            g_resizeHeight = 0;
            CreateRenderTarget();
        }

        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        // ── Единственный вызов меню внутри существующего кадра ImGui ──────
        const bool menuDrawn = gayclient::RenderMenu(g_menuState, g_menuActions);

        if (!menuDrawn)
        {
            // Демонстрационная «закрытая» плашка; в клиенте меню просто скрыто.
            ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f),
                                    ImGuiCond_Always, ImVec2(0.5f, 0.5f));
            ImGui::Begin("##closed", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoSavedSettings |
                                             ImGuiWindowFlags_AlwaysAutoResize);
            ImGui::TextUnformatted("Меню GAYClient закрыто");
            ImGui::Spacing();
            if (ImGui::Button("Открыть меню", ImVec2(180, 32)))
                g_menuState.visible = true;
            ImGui::TextDisabled("Right Shift подключается на стороне клиента.");
            ImGui::End();
        }

        ImGui::Render();

        const float clearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f }; // чёрный фон — за меню
        g_context->OMSetRenderTargets(1, &g_renderTarget, nullptr);
        g_context->ClearRenderTargetView(g_renderTarget, clearColor);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
        g_swapChain->Present(1, 0); // vsync
    }

    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
    CleanupDeviceD3D();
    ::DestroyWindow(hwnd);
    ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
    return 0;
}
