// Autoload companion DLL, imported by the patched original client.exe.
// It runs inside the GUI process: no second EXE or console is started.
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <tlhelp32.h>
#include <cstdio>
#include <cwchar>
#include <cstdint>
#include <cstdarg>
#include <atomic>
#include "wheel_bridge.h"
#include "game_lifecycle.h"
#include "launcher_ui.h"

static HINSTANCE loaderModule = nullptr;
static HANDLE wheelMapping = nullptr;
static WheelBridge* wheelView = nullptr;
static std::atomic<DWORD> foregroundGamePid{0};
static std::atomic<bool> wheelHookReady{false};

static void logLine(const char* format, ...) {
    char line[512]{};
    va_list args; va_start(args, format);
    int count = std::vsnprintf(line, sizeof(line) - 2, format, args);
    va_end(args);
    if (count < 0) return;
    size_t length = static_cast<size_t>(count);
    if (length > sizeof(line) - 3) length = sizeof(line) - 3;
    while (length && (line[length - 1] == '\n' || line[length - 1] == '\r')) --length;
    line[length++] = '\r'; line[length++] = '\n';
    wchar_t logPath[32768]{};
    DWORD n = GetModuleFileNameW(nullptr, logPath, 32768);
    if (!n || n >= 32768) return;
    wchar_t* slash = wcsrchr(logPath, L'\\');
    if (!slash || static_cast<size_t>(slash - logPath) + 24 >= 32768) return;
    wcscpy(slash + 1, L"client-autoload.log");
    HANDLE file = CreateFileW(logPath, FILE_APPEND_DATA, FILE_SHARE_READ | FILE_SHARE_WRITE,
                              nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) return;
    DWORD written = 0; WriteFile(file, line, static_cast<DWORD>(length), &written, nullptr);
    CloseHandle(file);
}
static void showError(const wchar_t* text) {
    MessageBoxW(nullptr, text, L"GAYClient", MB_OK | MB_ICONERROR | MB_SETFOREGROUND);
}
// Error dialogs must not block the game-monitoring thread. The caller passes
// static string literals that stay valid until this process exits.
static DWORD WINAPI showErrorThread(void* text) { showError(static_cast<const wchar_t*>(text)); return 0; }
static void showErrorAsync(const wchar_t* text) {
    HANDLE thread=CreateThread(nullptr,0,showErrorThread,const_cast<wchar_t*>(text),0,nullptr);
    if (thread) CloseHandle(thread); else logLine("Could not show launcher error dialog (%lu).",GetLastError());
}

struct WindowSearch { DWORD pid; bool found; };
static BOOL CALLBACK findGameChild(HWND window, LPARAM param) {
    auto* search = reinterpret_cast<WindowSearch*>(param);
    DWORD pid = 0; GetWindowThreadProcessId(window, &pid);
    if (pid == search->pid) { search->found = true; return FALSE; }
    return TRUE;
}
static bool gameIsForeground(DWORD pid) {
    HWND foreground = GetForegroundWindow();
    if (!foreground || !pid) return false;
    DWORD windowPid = 0; GetWindowThreadProcessId(foreground, &windowPid);
    if (windowPid == pid) return true;
    WindowSearch search{pid, false};
    EnumChildWindows(foreground, findGameChild, reinterpret_cast<LPARAM>(&search));
    return search.found;
}
// This desktop-wide hook ignores everything except wheel events while the
// selected Minecraft window is foreground. It never suppresses or logs input.
static LRESULT CALLBACK mouseHook(int code, WPARAM message, LPARAM data) {
    if (code == HC_ACTION && wheelView && (message == WM_MOUSEWHEEL || message == WM_MOUSEHWHEEL) &&
        gameIsForeground(foregroundGamePid.load(std::memory_order_relaxed))) {
        const auto* input = reinterpret_cast<const MSLLHOOKSTRUCT*>(data);
        LONG delta = static_cast<short>(HIWORD(input->mouseData));
        // Win32 positive horizontal means right; ImGui positive means left.
        if (message == WM_MOUSEHWHEEL) delta = -delta;
        InterlockedExchangeAdd(message == WM_MOUSEWHEEL ? &wheelView->vertical : &wheelView->horizontal, delta);
    }
    return CallNextHookEx(nullptr, code, message, data);
}
static DWORD WINAPI wheelHookThread(void*) {
    HHOOK hook = SetWindowsHookExW(WH_MOUSE_LL, mouseHook, loaderModule, 0);
    if (!hook) { logLine("Wheel hook could not start (%lu).", GetLastError()); return 1; }
    wheelHookReady.store(true, std::memory_order_release);
    logLine("Wheel input bridge started; only Minecraft foreground wheel events are forwarded.");
    MSG message{};
    while (GetMessageW(&message, nullptr, 0, 0) > 0) { TranslateMessage(&message); DispatchMessageW(&message); }
    UnhookWindowsHookEx(hook);
    return 0;
}
static bool startWheelBridge() {
    wheelMapping = CreateFileMappingW(INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE, 0, sizeof(WheelBridge), nullptr);
    if (!wheelMapping) return false;
    wheelView = static_cast<WheelBridge*>(MapViewOfFile(wheelMapping, FILE_MAP_WRITE, 0, 0, sizeof(WheelBridge)));
    if (!wheelView) { CloseHandle(wheelMapping); wheelMapping = nullptr; return false; }
    wheelView->magic = kWheelMagic; wheelView->version = 1;
    wheelView->horizontal = wheelView->vertical = 0;
    HANDLE worker = CreateThread(nullptr, 0, wheelHookThread, nullptr, 0, nullptr);
    if (!worker) { UnmapViewOfFile(wheelView); wheelView = nullptr; CloseHandle(wheelMapping); wheelMapping = nullptr; return false; }
    CloseHandle(worker);
    return true;
}
static bool ownPath(wchar_t* out, size_t capacity, const wchar_t* name) {
    DWORD n = GetModuleFileNameW(nullptr, out, static_cast<DWORD>(capacity));
    if (!n || n >= capacity) return false;
    wchar_t* slash = wcsrchr(out, L'\\');
    if (!slash || static_cast<size_t>(slash - out + 1) + wcslen(name) >= capacity) return false;
    wcscpy(slash + 1, name);
    return true;
}
static bool isFile(const wchar_t* name) {
    DWORD attributes = GetFileAttributesW(name);
    return attributes != INVALID_FILE_ATTRIBUTES && !(attributes & FILE_ATTRIBUTE_DIRECTORY);
}
// Distinguish an empty successful snapshot from an enumeration failure.
static DWORD uniqueGamePid(bool& multiple, bool& snapshotOk) {
    multiple=false; snapshotOk=false; DWORD found=0;
    HANDLE snapshot=CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS,0);
    if (snapshot==INVALID_HANDLE_VALUE) return 0;
    PROCESSENTRY32W entry{}; entry.dwSize=sizeof(entry);
    if (!Process32FirstW(snapshot,&entry)) {
        snapshotOk=GetLastError()==ERROR_NO_MORE_FILES;
        CloseHandle(snapshot); return 0;
    }
    do {
        if (_wcsicmp(entry.szExeFile,L"Minecraft.Windows.exe")==0) {
            if (found && found!=entry.th32ProcessID) { multiple=true; break; }
            found=entry.th32ProcessID;
        }
    } while (Process32NextW(snapshot,&entry));
    snapshotOk=multiple || GetLastError()==ERROR_NO_MORE_FILES;
    CloseHandle(snapshot); return found;
}
// Keep an actual process handle, not just a PID, so PID reuse cannot be
// mistaken for the death of the Minecraft process we observed.
static HANDLE openGameLifetime(DWORD pid) {
    HANDLE process=OpenProcess(SYNCHRONIZE|PROCESS_QUERY_LIMITED_INFORMATION,FALSE,pid);
    if (!process) return nullptr;
    wchar_t path[32768]{}; DWORD length=32768;
    if (!QueryFullProcessImageNameW(process,0,path,&length)) { CloseHandle(process); return nullptr; }
    const wchar_t* name=wcsrchr(path,L'\\'); name=name?name+1:path;
    if (_wcsicmp(name,L"Minecraft.Windows.exe")!=0) { CloseHandle(process); return nullptr; }
    return process;
}
static uintptr_t remoteModule(DWORD pid, const wchar_t* moduleName, const wchar_t* expectedPath = nullptr) {
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, pid);
    if (snapshot == INVALID_HANDLE_VALUE) return 0;
    MODULEENTRY32W entry{}; entry.dwSize = sizeof(entry);
    uintptr_t address = 0;
    if (Module32FirstW(snapshot, &entry)) do {
        if (_wcsicmp(entry.szModule, moduleName) == 0 && (!expectedPath || _wcsicmp(entry.szExePath, expectedPath) == 0)) {
            address = reinterpret_cast<uintptr_t>(entry.modBaseAddr); break;
        }
    } while (Module32NextW(snapshot, &entry));
    CloseHandle(snapshot);
    return address;
}
// Install an unnamed mapping in Minecraft by duplicating only a read handle.
// Resolve the export RVA without executing the adapter in the launcher process.
static bool connectWheelBridge(DWORD pid, const wchar_t* dll) {
    if (!wheelMapping || !wheelHookReady.load(std::memory_order_acquire)) return false;
    HMODULE image = LoadLibraryExW(dll, nullptr, DONT_RESOLVE_DLL_REFERENCES);
    if (!image) { logLine("Cannot inspect wheel bridge export (%lu).", GetLastError()); return false; }
    FARPROC fn = GetProcAddress(image, "GAYClientConnectWheel");
    uintptr_t offset = fn ? reinterpret_cast<uintptr_t>(fn) - reinterpret_cast<uintptr_t>(image) : 0;
    FreeLibrary(image);
    if (!fn || !offset) { logLine("Wheel bridge export is unavailable."); return false; }
    uintptr_t remoteBase = remoteModule(pid, L"GAYClientInternal.dll", dll);
    if (!remoteBase) { logLine("Wheel bridge could not find loaded overlay module."); return false; }
    HANDLE process = OpenProcess(PROCESS_DUP_HANDLE | PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION |
                                 PROCESS_VM_OPERATION | PROCESS_VM_WRITE | PROCESS_VM_READ, FALSE, pid);
    if (!process) { logLine("Wheel bridge could not open game (%lu).", GetLastError()); return false; }
    HANDLE remoteHandle = nullptr;
    if (!DuplicateHandle(GetCurrentProcess(), wheelMapping, process, &remoteHandle, FILE_MAP_READ, FALSE, 0)) {
        logLine("Wheel bridge could not duplicate mapping (%lu).", GetLastError()); CloseHandle(process); return false;
    }
    HANDLE thread = CreateRemoteThread(process, nullptr, 0,
        reinterpret_cast<LPTHREAD_START_ROUTINE>(remoteBase + offset), remoteHandle, 0, nullptr);
    if (!thread) {
        DWORD error = GetLastError(); HANDLE local = nullptr;
        if (DuplicateHandle(process, remoteHandle, GetCurrentProcess(), &local, 0, FALSE,
                            DUPLICATE_CLOSE_SOURCE | DUPLICATE_SAME_ACCESS) && local) CloseHandle(local);
        CloseHandle(process); logLine("Wheel bridge could not start in game (%lu).", error); return false;
    }
    const DWORD wait = WaitForSingleObject(thread, 5000);
    DWORD result = 0;
    const bool ok = wait == WAIT_OBJECT_0 && GetExitCodeThread(thread, &result) && result == 1;
    CloseHandle(thread); CloseHandle(process);
    logLine(ok ? "Wheel bridge connected." : "Wheel bridge could not connect to the overlay.");
    return ok;
}

enum class InjectResult { Success, Retry, Stop };
static InjectResult inject(DWORD pid, const wchar_t* dll) {
    // Do not load another adapter with the same module name from a different folder.
    if (remoteModule(pid, L"GAYClientInternal.dll")) {
        if (remoteModule(pid, L"GAYClientInternal.dll", dll)) {
            logLine("This overlay DLL is already loaded."); return InjectResult::Success;
        }
        logLine("Another overlay DLL is already loaded. Restart Minecraft first.");
        return InjectResult::Stop;
    }
    HMODULE kernel = GetModuleHandleW(L"kernel32.dll");
    FARPROC load = kernel ? GetProcAddress(kernel, "LoadLibraryW") : nullptr;
    HMODULE loadModule = nullptr;
    if (!load || !GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                                     reinterpret_cast<LPCWSTR>(load), &loadModule)) {
        logLine("Cannot locate LoadLibraryW (%lu).\n", GetLastError()); return InjectResult::Retry;
    }
    wchar_t modulePath[32768]{};
    DWORD len = GetModuleFileNameW(loadModule, modulePath, 32768);
    if (!len || len >= 32768) { logLine("Cannot determine the system module path."); return InjectResult::Retry; }
    const wchar_t* name = wcsrchr(modulePath, L'\\'); name = name ? name + 1 : modulePath;
    uintptr_t otherBase = remoteModule(pid, name);
    if (!otherBase) { logLine("Cannot inspect game modules (%lu).\n", GetLastError()); return InjectResult::Retry; }
    uintptr_t offset = reinterpret_cast<uintptr_t>(load) - reinterpret_cast<uintptr_t>(loadModule);
    auto remoteLoad = reinterpret_cast<LPTHREAD_START_ROUTINE>(otherBase + offset);
    HANDLE process = OpenProcess(PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION | PROCESS_VM_OPERATION |
                                 PROCESS_VM_WRITE | PROCESS_VM_READ, FALSE, pid);
    if (!process) { logLine("Cannot open Minecraft PID %lu (%lu).\n", pid, GetLastError()); return InjectResult::Retry; }
    BOOL wow64 = FALSE;
    if (!IsWow64Process(process, &wow64) || wow64) { logLine("Minecraft must be a 64-bit process."); CloseHandle(process); return InjectResult::Stop; }
    SIZE_T size = (wcslen(dll) + 1) * sizeof(wchar_t);
    void* memory = VirtualAllocEx(process, nullptr, size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    if (!memory) { logLine("Cannot allocate in game (%lu).\n", GetLastError()); CloseHandle(process); return InjectResult::Retry; }
    SIZE_T written = 0;
    if (!WriteProcessMemory(process, memory, dll, size, &written) || written != size) {
        logLine("Cannot write DLL path (%lu).\n", GetLastError());
        VirtualFreeEx(process, memory, 0, MEM_RELEASE); CloseHandle(process); return InjectResult::Retry;
    }
    HANDLE thread = CreateRemoteThread(process, nullptr, 0, remoteLoad, memory, 0, nullptr);
    if (!thread) {
        logLine("Cannot start loading thread (%lu).\n", GetLastError());
        VirtualFreeEx(process, memory, 0, MEM_RELEASE); CloseHandle(process); return InjectResult::Retry;
    }
    DWORD result = WaitForSingleObject(thread, 20000);
    DWORD exitCode = 0;
    bool finished = result == WAIT_OBJECT_0 && GetExitCodeThread(thread, &exitCode);
    // Never free the path while a timed-out remote thread may still be reading it.
    if (finished) VirtualFreeEx(process, memory, 0, MEM_RELEASE);
    CloseHandle(thread); CloseHandle(process);
    if (!finished) { logLine("Loading thread timed out; not retrying automatically."); return InjectResult::Stop; }
    if (!exitCode) { logLine("Minecraft could not load the DLL. Check access permissions."); return InjectResult::Retry; }
    logLine("DLL load returned successfully. The overlay initialization still needs to succeed.");
    return InjectResult::Success;
}
static DWORD WINAPI loaderThread(void*) {
    HANDLE guard = CreateMutexW(nullptr, FALSE, L"Local\\GAYClientAutoLaunch.v1");
    if (!guard) { showError(L"Не удалось запустить автозагрузку."); return 1; }
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        CloseHandle(guard);
        showError(L"Клиент уже запущен. Если вы только что закрыли игру, подождите несколько секунд и попробуйте снова.");
        launcherRequestShutdown(); // Do not leave a second, unmonitored client process behind.
        return 1;
    }
    wchar_t classicFlag[32768]{};
    if (ownPath(classicFlag, 32768, L"launcher-classic.flag") && isFile(classicFlag))
        logLine("Classic launcher mode enabled by local flag.");
    else if (!launcherStartUi())
        logLine("New launcher window could not start; the original remains available.");
    wchar_t dll[32768]{};
    if (!ownPath(dll, 32768, L"GAYClientInternal.dll") || !isFile(dll)) {
        launcherSetStatus(LauncherStatus::Failed);
        logLine("Keep GAYClientInternal.dll next to client.exe.");
        showError(L"Не найдена DLL клиента. Проверьте папку установки."); CloseHandle(guard); return 1;
    }
    logLine("Client started. Waiting for Minecraft.");
    if (!startWheelBridge()) logLine("Wheel input bridge unavailable; retaining UWP input.");
    bool done=false, multipleWarned=false, watchFailureLogged=false, shuttingDown=false;
    DWORD previousPid=0, watchedPid=0; unsigned attempts=0;
    HANDLE watchedGame=nullptr;
    UnwatchedGameExit fallback;
    ULONGLONG firstSeen=0, nextAttempt=0;
    while (true) {
        Sleep(1000);
        bool multiple=false, snapshotOk=false;
        DWORD pid=uniqueGamePid(multiple,snapshotOk);
        DWORD wait=watchedGame?WaitForSingleObject(watchedGame,0):WAIT_TIMEOUT;
        if (wait==WAIT_FAILED) {
            logLine("Could not wait for Minecraft process (%lu); retrying monitoring.",GetLastError());
            CloseHandle(watchedGame); watchedGame=nullptr; watchedPid=0; fallback.markObserved();
        }
        auto exitAction=gameExitAction(snapshotOk,multiple,pid,watchedPid,wait==WAIT_OBJECT_0);
        if (exitAction==GameExitAction::CloseLauncher) {
            logLine("Minecraft exited; closing the launcher process.");
            foregroundGamePid.store(0,std::memory_order_relaxed);
            CloseHandle(watchedGame); watchedGame=nullptr;
            shuttingDown=true; launcherRequestShutdown();
            break;
        }
        if (exitAction==GameExitAction::WatchReplacement) {
            logLine("The observed Minecraft process exited; another remains, monitoring it instead.");
            CloseHandle(watchedGame); watchedGame=nullptr; watchedPid=0; previousPid=0; done=false; attempts=0;
        }
        if (!watchedGame && fallback.update(snapshotOk,multiple,pid)) {
            logLine("Minecraft absent from three consecutive complete snapshots; closing the launcher.");
            foregroundGamePid.store(0,std::memory_order_relaxed);
            shuttingDown=true; launcherRequestShutdown();
            break;
        }
        if (!snapshotOk) continue; // Never treat an enumeration error as a closed game.
        foregroundGamePid.store(multiple ? 0 : pid,std::memory_order_relaxed);
        if (multiple) {
            launcherSetStatus(LauncherStatus::MultipleGames);
            if (!multipleWarned) {
                multipleWarned=true;
                logLine("Multiple Minecraft processes found; close extras before connecting.");
                showErrorAsync(L"Найдено несколько процессов Minecraft. Закройте лишние окна игры.");
            }
            continue;
        }
        if (multipleWarned) { multipleWarned=false; previousPid=0; done=false; attempts=0; }
        if (watchedGame && wait==WAIT_OBJECT_0) continue; // Wait until the snapshot stops showing that PID.
        if (watchedGame && pid!=watchedPid) continue; // Inconsistent snapshot; do not infer an exit.
        if (!pid) { launcherSetStatus(LauncherStatus::Waiting); previousPid=0; done=false; attempts=0; continue; }
        if (!watchedGame) {
            watchedGame=openGameLifetime(pid);
            if (watchedGame) { watchedPid=pid; fallback.reset(); watchFailureLogged=false; logLine("Monitoring Minecraft PID %lu until it exits.",pid); }
            else {
                fallback.markObserved();
                if (!watchFailureLogged) { watchFailureLogged=true; logLine("Cannot monitor Minecraft lifetime yet (%lu); retrying.",GetLastError()); }
            }
        }
        ULONGLONG now = GetTickCount64();
        if (pid != previousPid) {
            previousPid = pid; done = false; attempts = 0; firstSeen = now; nextAttempt = now + 10000;
            launcherSetStatus(LauncherStatus::GameDetected);
            logLine("Minecraft detected (PID %lu); waiting for startup...\n", pid);
        }
        if (!done && now >= nextAttempt && now - firstSeen >= 10000) {
            ++attempts;
            launcherSetStatus(LauncherStatus::Loading);
            logLine("Loading overlay...");
            InjectResult result = inject(pid, dll);
            if (result == InjectResult::Success || result == InjectResult::Stop) {
                done = true;
                if (result == InjectResult::Success) { connectWheelBridge(pid, dll); launcherSetStatus(LauncherStatus::Loaded); }
                if (result == InjectResult::Stop) { launcherSetStatus(LauncherStatus::Failed); showErrorAsync(L"Не удалось автоматически загрузить DLL. Перезапустите Minecraft и клиент. Подробности в client-autoload.log."); }
            }
            else if (attempts >= 3) {
                done = true;
                launcherSetStatus(LauncherStatus::Failed);
                logLine("Automatic loading failed. Close this client and retry, or load manually.");
                showErrorAsync(L"Не удалось автоматически загрузить DLL. Проверьте доступ к файлам и повторите запуск. Подробности в client-autoload.log.");
            } else {
                logLine("Game may still be starting; retrying in five seconds.");
                nextAttempt = GetTickCount64() + 5000;
            }
        }
    }
    if (watchedGame) CloseHandle(watchedGame);
    logLine("Automatic loader stopped.");
    // Hold the single-instance guard until this process actually exits; closing
    // it here would allow another launcher while the old UI is still closing.
    if (!shuttingDown) CloseHandle(guard);
    return 0;
}

// Imported by the patched EXE to ensure the Windows loader starts this DLL.
extern "C" __declspec(dllexport) void GAYClientAutoloadImport() {}

BOOL WINAPI DllMain(HINSTANCE module, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        loaderModule = module;
        DisableThreadLibraryCalls(module);
        HANDLE thread = CreateThread(nullptr, 0, loaderThread, nullptr, 0, nullptr);
        if (!thread) return FALSE; // Let Windows report a startup failure instead of silently doing nothing.
        CloseHandle(thread);
    }
    return TRUE;
}
