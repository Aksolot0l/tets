#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define UNICODE
#define _UNICODE
#include <windows.h>
#include <appmodel.h>
#include <tlhelp32.h>
#include "launcher_profiles_win.h"
#include <vector>

std::wstring toWide(const std::string& s) {
    if (s.empty()) return {};
    int n=MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,s.data(),static_cast<int>(s.size()),nullptr,0);
    if (!n) return {};
    std::wstring result(n,L'\0');
    if (!MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,s.data(),static_cast<int>(s.size()),&result[0],n)) return {};
    return result;
}
std::string toUtf8(const std::wstring& s) {
    if (s.empty()) return {};
    int n=WideCharToMultiByte(CP_UTF8,WC_ERR_INVALID_CHARS,s.data(),static_cast<int>(s.size()),nullptr,0,nullptr,nullptr);
    if (!n) return {};
    std::string result(n,'\0');
    if (!WideCharToMultiByte(CP_UTF8,WC_ERR_INVALID_CHARS,s.data(),static_cast<int>(s.size()),&result[0],n,nullptr,nullptr)) return {};
    return result;
}
static std::wstring localAppData() {
    DWORD size=GetEnvironmentVariableW(L"LOCALAPPDATA",nullptr,0);
    if (!size || size>32000) return {};
    std::wstring result(size,L'\0'); DWORD n=GetEnvironmentVariableW(L"LOCALAPPDATA",&result[0],size);
    if (!n || n>=size) return {}; result.resize(n); return result;
}
// 1 = present/read, 0 = absent, -1 = error. Existing bad files stay untouched.
static int readFile(const std::wstring& path,std::string& out) {
    HANDLE h=CreateFileW(path.c_str(),GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
    if(h==INVALID_HANDLE_VALUE) return GetLastError()==ERROR_FILE_NOT_FOUND || GetLastError()==ERROR_PATH_NOT_FOUND ? 0 : -1;
    LARGE_INTEGER size{};
    if (!GetFileSizeEx(h,&size) || size.QuadPart<0 || size.QuadPart>2*1024*1024) { CloseHandle(h); return -1; }
    out.resize(static_cast<size_t>(size.QuadPart)); DWORD got=0;
    bool ok=out.empty() || (ReadFile(h,&out[0],static_cast<DWORD>(out.size()),&got,nullptr) && got==out.size());
    CloseHandle(h); return ok?1:-1;
}
bool loadLauncherProfiles(LauncherProfiles& profiles) {
    auto local=localAppData(); if(local.empty()) return false;
    std::string data;
    int result=readFile(local+L"\\GAYClientLauncher\\profiles.json",data);
    if(result<0) return false;
    if(result>0) return readProfiles(data,false,profiles);
    result=readFile(local+L"\\AogiriGreenLauncher\\settings.json",data);
    if(result<0) return false;
    if(result>0) {
        LauncherProfiles imported;
        if (!readProfiles(data,true,imported)) return false;
        profiles=std::move(imported);
    }
    return true;
}
bool saveLauncherProfiles(const LauncherProfiles& profiles) {
    if (profiles.entries.empty() || profiles.entries.size()>200 || profiles.selected>=profiles.entries.size()) return false;
    for(const auto& p:profiles.entries) if(!validProfile(p)) return false;
    auto local=localAppData(); if(local.empty()) return false;
    auto dir=local+L"\\GAYClientLauncher";
    if(!CreateDirectoryW(dir.c_str(),nullptr) && GetLastError()!=ERROR_ALREADY_EXISTS) return false;
    wchar_t tmp[MAX_PATH]{};
    if(!GetTempFileNameW(dir.c_str(),L"gcp",0,tmp)) return false;
    HANDLE h=CreateFileW(tmp,GENERIC_WRITE,0,nullptr,TRUNCATE_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
    if(h==INVALID_HANDLE_VALUE) { DeleteFileW(tmp); return false; }
    std::string bytes;
    try { bytes=writeProfiles(profiles); } catch (...) { CloseHandle(h); DeleteFileW(tmp); return false; }
    DWORD wrote=0;
    bool ok=WriteFile(h,bytes.data(),static_cast<DWORD>(bytes.size()),&wrote,nullptr) && wrote==bytes.size() && FlushFileBuffers(h);
    CloseHandle(h);
    if(ok) ok=MoveFileExW(tmp,(dir+L"\\profiles.json").c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH)!=0;
    if(!ok) DeleteFileW(tmp);
    return ok;
}
bool validAppId(const std::wstring& s) {
    if (s.empty() || s.size()>512) return false;
    unsigned bangs=0;
    for (wchar_t c:s) {
        if(c==L'!') { ++bangs; continue; }
        if(!((c>=L'a'&&c<=L'z')||(c>=L'A'&&c<=L'Z')||(c>=L'0'&&c<=L'9')||c==L'.'||c==L'_'||c==L'-')) return false;
    }
    return bangs==1 && s.front()!=L'!' && s.back()!=L'!';
}
std::wstring detectMinecraftAppId() {
    HANDLE snapshot=CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS,0);
    if(snapshot==INVALID_HANDLE_VALUE) return {};
    PROCESSENTRY32W entry{}; entry.dwSize=sizeof(entry); DWORD id=0; bool multiple=false;
    if(Process32FirstW(snapshot,&entry)) do {
        if(_wcsicmp(entry.szExeFile,L"Minecraft.Windows.exe")==0) { if(id && id!=entry.th32ProcessID) multiple=true; id=entry.th32ProcessID; }
    } while(Process32NextW(snapshot,&entry));
    CloseHandle(snapshot); if (!id || multiple) return {};
    HANDLE process=OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION,FALSE,id); if(!process) return {};
    UINT32 size=0; LONG rc=GetApplicationUserModelId(process,&size,nullptr);
    if(rc!=ERROR_INSUFFICIENT_BUFFER || size<2 || size>513) { CloseHandle(process); return {}; }
    std::vector<wchar_t> buffer(size); rc=GetApplicationUserModelId(process,&size,buffer.data()); CloseHandle(process);
    if(rc!=ERROR_SUCCESS) return {};
    std::wstring result(buffer.data()); return validAppId(result)?result:std::wstring();
}
