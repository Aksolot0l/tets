// Optional companion launcher: start the unchanged GUI and load the bundled
// adapter once a Minecraft.Windows.exe process appears. Windows x64 only.
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <tlhelp32.h>
#include <cstdio>
#include <cwchar>
#include <cstdint>
#include <cstdarg>

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
    if (!slash || static_cast<size_t>(slash - logPath) + 18 >= 32768) return;
    wcscpy(slash + 1, L"START_CLIENT.log");
    HANDLE file = CreateFileW(logPath, FILE_APPEND_DATA, FILE_SHARE_READ | FILE_SHARE_WRITE,
                              nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) return;
    DWORD written = 0; WriteFile(file, line, static_cast<DWORD>(length), &written, nullptr);
    CloseHandle(file);
}
static void showError(const wchar_t* text) {
    MessageBoxW(nullptr, text, L"GAYClient", MB_OK | MB_ICONERROR | MB_SETFOREGROUND);
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
static DWORD uniqueGamePid(bool& multiple) {
    multiple = false; DWORD found = 0;
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE) return 0;
    PROCESSENTRY32W entry{}; entry.dwSize = sizeof(entry);
    if (Process32FirstW(snapshot, &entry)) do {
        if (_wcsicmp(entry.szExeFile, L"Minecraft.Windows.exe") == 0) {
            if (found && found != entry.th32ProcessID) { multiple = true; break; }
            found = entry.th32ProcessID;
        }
    } while (Process32NextW(snapshot, &entry));
    CloseHandle(snapshot);
    return found;
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
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
    HANDLE guard = CreateMutexW(nullptr, FALSE, L"Local\\GAYClientAutoLaunch.v1");
    if (!guard) { showError(L"Не удалось запустить автозагрузчик."); return 1; }
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        CloseHandle(guard); showError(L"Автозагрузчик уже запущен. Закройте клиент перед повторным запуском."); return 1;
    }
    wchar_t client[32768]{}, dll[32768]{};
    if (!ownPath(client, 32768, L"client.exe") || !ownPath(dll, 32768, L"GAYClientInternal.dll") ||
        !isFile(client) || !isFile(dll)) {
        logLine("Keep START_CLIENT.exe next to client.exe and GAYClientInternal.dll.");
        showError(L"Не найдены файлы клиента или DLL. Проверьте папку установки."); CloseHandle(guard); return 1;
    }
    STARTUPINFOW startup{}; startup.cb = sizeof(startup);
    PROCESS_INFORMATION process{};
    if (!CreateProcessW(client, nullptr, nullptr, nullptr, FALSE, 0, nullptr, nullptr, &startup, &process)) {
        logLine("Cannot start client.exe (%lu).", GetLastError()); showError(L"Не удалось запустить client.exe. Подробности в START_CLIENT.log."); CloseHandle(guard); return 1;
    }
    CloseHandle(process.hThread);
    logLine("Client started. In the client, start Minecraft normally.");
    logLine("Waiting for the game; automatic loader runs until the client closes.");
    bool done = false; DWORD previousPid = 0; unsigned attempts = 0;
    ULONGLONG firstSeen = 0, nextAttempt = 0;
    while (WaitForSingleObject(process.hProcess, 1000) == WAIT_TIMEOUT) {
        bool multiple = false; DWORD pid = uniqueGamePid(multiple);
        if (multiple) { logLine("Multiple Minecraft processes found; close extras and restart the client."); showError(L"Найдено несколько процессов Minecraft. Закройте лишние и перезапустите клиент."); break; }
        if (!pid) { previousPid = 0; done = false; attempts = 0; continue; }
        ULONGLONG now = GetTickCount64();
        if (pid != previousPid) {
            previousPid = pid; done = false; attempts = 0; firstSeen = now; nextAttempt = now + 10000;
            logLine("Minecraft detected (PID %lu); waiting for startup...\n", pid);
        }
        if (!done && now >= nextAttempt && now - firstSeen >= 10000) {
            ++attempts;
            logLine("Loading overlay...");
            InjectResult result = inject(pid, dll);
            if (result == InjectResult::Success || result == InjectResult::Stop) {
                done = true;
                if (result == InjectResult::Stop) showError(L"Не удалось автоматически загрузить DLL. Перезапустите Minecraft и клиент. Подробности в START_CLIENT.log.");
            }
            else if (attempts >= 3) {
                done = true;
                logLine("Automatic loading failed. Close this client and retry, or load manually.");
                showError(L"Не удалось автоматически загрузить DLL. Проверьте доступ к файлам и повторите запуск. Подробности в START_CLIENT.log.");
            } else {
                logLine("Game may still be starting; retrying in five seconds.");
                nextAttempt = GetTickCount64() + 5000;
            }
        }
    }
    CloseHandle(process.hProcess);
    logLine("Automatic loader stopped.");
    CloseHandle(guard);
    return 0;
}
