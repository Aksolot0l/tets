#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define UNICODE
#define _UNICODE
#include "launcher_webview.h"
#include "launcher_profiles_win.h"
#include "../vendor/webview2/WebView2.h"
#include <shlwapi.h>
#include <atomic>
#include <functional>
#include <string>
#include <cwchar>

namespace {
constexpr wchar_t kUrl[] = L"https://launcher.gayclient.local/";
constexpr wchar_t kHeaders[] = L"Content-Type: text/html; charset=utf-8\r\nCache-Control: no-store\r\nX-Content-Type-Options: nosniff\r\nContent-Security-Policy: default-src 'none'; script-src 'unsafe-inline'; style-src 'unsafe-inline'; img-src data:; font-src data:; connect-src 'none'; base-uri 'none'; form-action 'none'; frame-src 'none'\r\n";

template<class Interface> const IID& interfaceId();
template<> const IID& interfaceId<ICoreWebView2NavigationStartingEventHandler>() { return IID_ICoreWebView2NavigationStartingEventHandler; }
template<> const IID& interfaceId<ICoreWebView2NewWindowRequestedEventHandler>() { return IID_ICoreWebView2NewWindowRequestedEventHandler; }
template<> const IID& interfaceId<ICoreWebView2WebResourceRequestedEventHandler>() { return IID_ICoreWebView2WebResourceRequestedEventHandler; }
template<> const IID& interfaceId<ICoreWebView2WebMessageReceivedEventHandler>() { return IID_ICoreWebView2WebMessageReceivedEventHandler; }
template<> const IID& interfaceId<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>() { return IID_ICoreWebView2CreateCoreWebView2ControllerCompletedHandler; }
template<> const IID& interfaceId<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>() { return IID_ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler; }
template<> const IID& interfaceId<ICoreWebView2ProcessFailedEventHandler>() { return IID_ICoreWebView2ProcessFailedEventHandler; }
template<> const IID& interfaceId<ICoreWebView2PermissionRequestedEventHandler>() { return IID_ICoreWebView2PermissionRequestedEventHandler; }

template<class Interface, class... Args> class Handler final : public Interface {
    std::atomic<ULONG> refs{1};
    std::function<HRESULT(Args...)> callback;
public:
    explicit Handler(std::function<HRESULT(Args...)> cb):callback(std::move(cb)) {}
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID id, void** output) override {
        if (!output) return E_POINTER;
        *output = nullptr;
        if (IsEqualIID(id, __uuidof(IUnknown)) || IsEqualIID(id, interfaceId<Interface>())) { *output=static_cast<Interface*>(this); AddRef(); return S_OK; }
        return E_NOINTERFACE;
    }
    ULONG STDMETHODCALLTYPE AddRef() override { return ++refs; }
    ULONG STDMETHODCALLTYPE Release() override { ULONG result=--refs; if (!result) delete this; return result; }
    HRESULT STDMETHODCALLTYPE Invoke(Args... args) override { try { return callback(args...); } catch (...) { return E_FAIL; } }
};

template<class Interface, class... Args, class Fn> auto makeHandler(Fn fn) {
    return new Handler<Interface, Args...>(std::function<HRESULT(Args...)>(fn));
}

struct Host {
    HWND parent=nullptr;
    HMODULE loader=nullptr;
    ICoreWebView2Environment* env=nullptr;
    ICoreWebView2Controller* controller=nullptr;
    ICoreWebView2* web=nullptr;
    WebDispatch dispatch=nullptr;
    WebReady onReady=nullptr, onFailure=nullptr;
    bool closed=true, ready=false, failed=false;
    ULONGLONG started=0;
    std::string lastStatus;
    void fail(const wchar_t* detail) {
        if (closed || failed) return;
        failed=true;
        OutputDebugStringW(detail);
        if (controller) controller->put_IsVisible(FALSE);
        if (onFailure) onFailure();
        MessageBoxW(parent,L"Не удалось открыть новый интерфейс WebView2. Оставлено рабочее окно лаунчера. Проверьте наличие Microsoft Edge WebView2 Runtime и файла WebView2Loader.dll рядом с client.exe.",L"GAYClient - интерфейс",MB_OK|MB_ICONWARNING);
    }
    void resize() {
        if (!controller || !parent || closed) return;
        RECT bounds{}; GetClientRect(parent,&bounds); controller->put_Bounds(bounds);
    }
    void post(const std::string& message) {
        if (!web || closed) return;
        const auto wide=toWide(message);
        if (!wide.empty()) web->PostWebMessageAsJson(wide.c_str());
    }
    void stop() {
        closed=true;
        if (controller) controller->Close();
        if (web) { web->Release(); web=nullptr; }
        if (controller) { controller->Release(); controller=nullptr; }
        if (env) { env->Release(); env=nullptr; }
        // The loader stays loaded while WebView2 async callbacks may be in flight.
        parent=nullptr; ready=false;
    }
    HRESULT resources(ICoreWebView2WebResourceRequestedEventArgs* args) {
        if (closed || !env || !args) return E_FAIL;
        ICoreWebView2WebResourceRequest* request=nullptr;
        LPWSTR uri=nullptr;
        if (SUCCEEDED(args->get_Request(&request)) && request) { request->get_Uri(&uri); request->Release(); }
        const bool allowed=uri && wcscmp(uri,kUrl)==0;
        if (uri) CoTaskMemFree(uri);
        IStream* stream=nullptr;
        if (allowed) {
            HMODULE module=nullptr;
            GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                               reinterpret_cast<LPCWSTR>(&startLauncherWebView),&module);
            HRSRC resource=module?FindResourceW(module,MAKEINTRESOURCEW(104),RT_RCDATA):nullptr;
            DWORD size=resource?SizeofResource(module,resource):0;
            HGLOBAL data=resource?LoadResource(module,resource):nullptr;
            const BYTE* bytes=data?static_cast<const BYTE*>(LockResource(data)):nullptr;
            if (!bytes || !size || size>4*1024*1024) { if(web) web->Stop(); fail(L"Embedded UI resource missing"); return E_FAIL; }
            stream=SHCreateMemStream(bytes,size);
            if (!stream) { if(web) web->Stop(); fail(L"Unable to read embedded UI resource"); return E_OUTOFMEMORY; }
        }
        ICoreWebView2WebResourceResponse* response=nullptr;
        HRESULT hr=env->CreateWebResourceResponse(stream,allowed?200:403,allowed?L"OK":L"Forbidden",allowed?kHeaders:L"Content-Type: text/plain\r\nCache-Control: no-store\r\n",&response);
        if (stream) stream->Release();
        if (SUCCEEDED(hr) && response) { hr=args->put_Response(response); response->Release(); }
        if (FAILED(hr)) { if(web) web->Stop(); fail(L"Failed to intercept web resource"); }
        return hr;
    }
    HRESULT message(ICoreWebView2WebMessageReceivedEventArgs* args) {
        if (closed || failed || !args || !dispatch) return S_OK;
        LPWSTR source=nullptr; LPWSTR json=nullptr;
        args->get_Source(&source);
        bool allowed=source && wcscmp(source,kUrl)==0;
        if (source) CoTaskMemFree(source);
        if (!allowed || FAILED(args->get_WebMessageAsJson(&json)) || !json) return S_OK;
        std::wstring input(json); CoTaskMemFree(json);
        if (input.size()>16384) return S_OK;
        std::string response=dispatch(toUtf8(input));
        post(response);
        // Only a valid native "ready" request may reveal the web UI. The
        // dispatcher emits this exact marker only for that request.
        if (!ready && response.find("\"readyMarker\":true")!=std::string::npos) {
            ready=true; if(onReady) onReady(); resize(); controller->put_IsVisible(TRUE); controller->MoveFocus(COREWEBVIEW2_MOVE_FOCUS_REASON_PROGRAMMATIC);
        }
        return S_OK;
    }
    HRESULT controllerReady(HRESULT result, ICoreWebView2Controller* value) {
        if (closed || failed) return S_OK;
        if (FAILED(result) || !value) { fail(L"WebView2 controller creation failed"); return S_OK; }
        controller=value; controller->AddRef(); controller->put_IsVisible(FALSE); resize();
        if (FAILED(controller->get_CoreWebView2(&web)) || !web) { fail(L"WebView2 instance unavailable"); return S_OK; }
        ICoreWebView2Settings* settings=nullptr;
        if (SUCCEEDED(web->get_Settings(&settings)) && settings) {
            settings->put_AreDevToolsEnabled(FALSE); settings->put_AreDefaultContextMenusEnabled(FALSE);
            settings->put_AreHostObjectsAllowed(FALSE); settings->put_IsWebMessageEnabled(TRUE);
            settings->Release();
        }
        EventRegistrationToken token{}; HRESULT hr=S_OK;
        auto nav=makeHandler<ICoreWebView2NavigationStartingEventHandler,ICoreWebView2*,ICoreWebView2NavigationStartingEventArgs*>([](auto*,auto* args) -> HRESULT {
            LPWSTR uri=nullptr; if (args) args->get_Uri(&uri); bool allowed=uri && wcscmp(uri,kUrl)==0; if(uri)CoTaskMemFree(uri); if(args&&!allowed)args->put_Cancel(TRUE); return S_OK;
        });
        hr=web->add_NavigationStarting(nav,&token); nav->Release(); if(FAILED(hr)) { fail(L"Navigation handler unavailable"); return S_OK; }
        auto popup=makeHandler<ICoreWebView2NewWindowRequestedEventHandler,ICoreWebView2*,ICoreWebView2NewWindowRequestedEventArgs*>([](auto*,auto* args) -> HRESULT { if(args)args->put_Handled(TRUE); return S_OK; });
        hr=web->add_NewWindowRequested(popup,&token); popup->Release(); if(FAILED(hr)) { fail(L"Popup handler unavailable"); return S_OK; }
        auto crashed=makeHandler<ICoreWebView2ProcessFailedEventHandler,ICoreWebView2*,ICoreWebView2ProcessFailedEventArgs*>([this](auto*,auto*) -> HRESULT { fail(L"WebView2 process failed"); return S_OK; });
        hr=web->add_ProcessFailed(crashed,&token); crashed->Release(); if(FAILED(hr)) { fail(L"Process failure handler unavailable"); return S_OK; }
        auto permission=makeHandler<ICoreWebView2PermissionRequestedEventHandler,ICoreWebView2*,ICoreWebView2PermissionRequestedEventArgs*>([](auto*,auto* args) -> HRESULT { if(args)args->put_State(COREWEBVIEW2_PERMISSION_STATE_DENY); return S_OK; });
        hr=web->add_PermissionRequested(permission,&token); permission->Release(); if(FAILED(hr)) { fail(L"Permission handler unavailable"); return S_OK; }
        auto resource=makeHandler<ICoreWebView2WebResourceRequestedEventHandler,ICoreWebView2*,ICoreWebView2WebResourceRequestedEventArgs*>([this](auto*,auto* args) -> HRESULT { return resources(args); });
        hr=web->add_WebResourceRequested(resource,&token); resource->Release(); if(FAILED(hr) || FAILED(web->AddWebResourceRequestedFilter(L"*",COREWEBVIEW2_WEB_RESOURCE_CONTEXT_ALL))) { fail(L"Resource interception unavailable"); return S_OK; }
        auto handler=makeHandler<ICoreWebView2WebMessageReceivedEventHandler,ICoreWebView2*,ICoreWebView2WebMessageReceivedEventArgs*>([this](auto*,auto* args) -> HRESULT { return message(args); });
        hr=web->add_WebMessageReceived(handler,&token); handler->Release(); if(FAILED(hr) || FAILED(web->Navigate(kUrl))) fail(L"WebView2 navigation failed");
        return S_OK;
    }
    HRESULT envReady(HRESULT result, ICoreWebView2Environment* value) {
        if (closed || failed) return S_OK;
        if (FAILED(result) || !value) { fail(L"WebView2 environment creation failed"); return S_OK; }
        env=value; env->AddRef();
        auto cb=makeHandler<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler,HRESULT,ICoreWebView2Controller*>([this](auto hr,auto* view) -> HRESULT { return controllerReady(hr,view); });
        HRESULT hr=env->CreateCoreWebView2Controller(parent,cb); cb->Release(); if(FAILED(hr)) fail(L"WebView2 controller request failed");
        return S_OK;
    }
};
Host host;
}

bool startLauncherWebView(HWND parent, WebDispatch dispatch, WebReady ready, WebReady failure) {
    if (!parent || !dispatch || !ready || !host.closed) return false;
    host.parent=parent; host.dispatch=dispatch; host.onReady=ready; host.onFailure=failure; host.closed=false; host.failed=false; host.ready=false; host.started=GetTickCount64();
    wchar_t path[32768]{}; DWORD n=GetModuleFileNameW(nullptr,path,32768);
    wchar_t* slash=n && n<32768?wcsrchr(path,L'\\'):nullptr;
    if (!slash || static_cast<size_t>(slash-path)+wcslen(L"\\WebView2Loader.dll")>=32768) { host.fail(L"Invalid loader path"); return false; }
    wcscpy(slash+1,L"WebView2Loader.dll");
    host.loader=LoadLibraryExW(path,nullptr,LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR|LOAD_LIBRARY_SEARCH_SYSTEM32);
    if (!host.loader) { host.fail(L"WebView2Loader.dll missing"); return false; }
    using CreateEnv=HRESULT (STDAPICALLTYPE*)(PCWSTR,PCWSTR,ICoreWebView2EnvironmentOptions*,ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler*);
    auto create=reinterpret_cast<CreateEnv>(GetProcAddress(host.loader,"CreateCoreWebView2EnvironmentWithOptions"));
    if (!create) { host.fail(L"WebView2Loader export missing"); return false; }
    DWORD size=GetEnvironmentVariableW(L"LOCALAPPDATA",nullptr,0);
    if (!size || size>32000) { host.fail(L"LOCALAPPDATA missing"); return false; }
    std::wstring dir(size,L'\0'); DWORD count=GetEnvironmentVariableW(L"LOCALAPPDATA",&dir[0],size);
    if (!count || count>=size) { host.fail(L"LOCALAPPDATA invalid"); return false; }
    dir.resize(count); dir+=L"\\GAYClientLauncher";
    if(!CreateDirectoryW(dir.c_str(),nullptr) && GetLastError()!=ERROR_ALREADY_EXISTS) { host.fail(L"Profile directory unavailable"); return false; }
    dir+=L"\\WebView2";
    auto cb=makeHandler<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler,HRESULT,ICoreWebView2Environment*>([](auto hr,auto* env)->HRESULT { return host.envReady(hr,env); });
    HRESULT hr=create(nullptr,dir.c_str(),nullptr,cb); cb->Release();
    if (FAILED(hr)) { host.fail(L"WebView2 environment request failed"); return false; }
    return true;
}
void resizeLauncherWebView() { host.resize(); }
void updateLauncherWebViewStatus(const char* status) {
    if (!host.ready || host.closed || !status || host.lastStatus==status) return;
    host.lastStatus=status; host.post(std::string("{\"event\":\"status\",\"status\":\"")+status+"\"}");
}
void tickLauncherWebView() { if(!host.closed && !host.failed && !host.ready && GetTickCount64()-host.started>30000) host.fail(L"WebView2 ready timed out"); }
void closeLauncherWebView() { if(!host.closed) host.stop(); }
