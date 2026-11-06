#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define UNICODE
#define _UNICODE
#include "launcher_ui.h"
#include <shellapi.h>
#include <objbase.h>
#include <gdiplus.h>
#include <atomic>
#include <cstring>
#include <cwchar>
#include <initializer_list>

namespace {
std::atomic<LauncherStatus> state{LauncherStatus::Waiting};
HWND mainWindow = nullptr, classicWindow = nullptr, playButton = nullptr, classicButton = nullptr, logButton = nullptr;
bool classicVisible = false, hiddenClassicOnce = false;
ULONGLONG launchPendingUntil = 0;
int dpi = 96;
HFONT fontSmall = nullptr, fontBody = nullptr, fontBold = nullptr, fontTitle = nullptr, fontBrand = nullptr;
ULONG_PTR gdiplusToken = 0;
Gdiplus::Image* heroImage = nullptr;
IStream* heroStream = nullptr;
constexpr int PLAY = 101, CLASSIC = 102, LOG = 103;
int S(int n) { return MulDiv(n, dpi, 96); }
COLORREF rgb(int r,int g,int b) { return RGB(r,g,b); }
void disposeFonts() {
    for (HFONT* f : {&fontSmall, &fontBody, &fontBold, &fontTitle, &fontBrand}) {
        if (*f) DeleteObject(*f);
        *f = nullptr;
    }
}
HFONT font(int size, int weight) {
    return CreateFontW(-S(size),0,0,0,weight,FALSE,FALSE,FALSE,DEFAULT_CHARSET,
                       OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH,L"Segoe UI");
}
void rounded(HDC dc, int x,int y,int w,int h,int r,COLORREF color);
void makeFonts() { disposeFonts(); fontSmall=font(12,FW_NORMAL); fontBody=font(14,FW_NORMAL); fontBold=font(14,FW_SEMIBOLD); fontTitle=font(27,FW_BOLD); fontBrand=font(17,FW_BOLD); }
void loadHero() {
    Gdiplus::GdiplusStartupInput input;
    if (Gdiplus::GdiplusStartup(&gdiplusToken, &input, nullptr) != Gdiplus::Ok) { gdiplusToken=0; return; }
    HMODULE module=nullptr;
    if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                            reinterpret_cast<LPCWSTR>(&launcherStartUi),&module)) return;
    HRSRC resource=FindResourceW(module,MAKEINTRESOURCEW(101),RT_RCDATA);
    if (!resource) return;
    DWORD size=SizeofResource(module,resource); HGLOBAL found=LoadResource(module,resource);
    const void* bytes=found?LockResource(found):nullptr;
    if (!bytes || !size) return;
    HGLOBAL memory=GlobalAlloc(GMEM_MOVEABLE,size);
    if (!memory) return;
    void* target=GlobalLock(memory);
    if (!target) { GlobalFree(memory); return; }
    std::memcpy(target,bytes,size); GlobalUnlock(memory);
    if (FAILED(CreateStreamOnHGlobal(memory,TRUE,&heroStream))) { GlobalFree(memory); return; }
    heroImage=Gdiplus::Image::FromStream(heroStream);
    if (!heroImage || heroImage->GetLastStatus()!=Gdiplus::Ok) { delete heroImage; heroImage=nullptr; heroStream->Release(); heroStream=nullptr; }
}
void unloadHero() {
    delete heroImage; heroImage=nullptr;
    if (heroStream) { heroStream->Release(); heroStream=nullptr; }
    if (gdiplusToken) { Gdiplus::GdiplusShutdown(gdiplusToken); gdiplusToken=0; }
}
void drawHero(HDC dc) {
    rounded(dc,226,75,650,252,18,rgb(20,43,33));
    if (!heroImage) return; // All controls and the original GUI still work without the image.
    Gdiplus::Graphics g(dc);
    g.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);
    Gdiplus::GraphicsPath clip;
    const int x=S(226), y=S(75), w=S(650), h=S(252), r=S(18);
    clip.AddArc(x,y,r,r,180,90); clip.AddArc(x+w-r,y,r,r,270,90);
    clip.AddArc(x+w-r,y+h-r,r,r,0,90); clip.AddArc(x,y+h-r,r,r,90,90); clip.CloseFigure();
    g.SetClip(&clip);
    g.DrawImage(heroImage,Gdiplus::Rect(S(501),y,S(375),h),0,0,
                static_cast<int>(heroImage->GetWidth()),static_cast<int>(heroImage->GetHeight()),Gdiplus::UnitPixel);
    Gdiplus::LinearGradientBrush shade(Gdiplus::Point(S(485),y),Gdiplus::Point(S(655),y),
                                       Gdiplus::Color(255,20,43,33),Gdiplus::Color(0,20,43,33));
    g.FillRectangle(&shade,S(485),y,S(170),h);
}

void fill(HDC dc, RECT rect, COLORREF color) { HBRUSH brush=CreateSolidBrush(color); FillRect(dc,&rect,brush); DeleteObject(brush); }
void rounded(HDC dc, int x,int y,int w,int h,int r,COLORREF color) {
    HBRUSH brush=CreateSolidBrush(color); HGDIOBJ oldBrush=SelectObject(dc,brush);
    HGDIOBJ oldPen=SelectObject(dc,GetStockObject(NULL_PEN));
    RoundRect(dc,S(x),S(y),S(x+w),S(y+h),S(r),S(r));
    SelectObject(dc,oldPen); SelectObject(dc,oldBrush); DeleteObject(brush);
}
void text(HDC dc, const wchar_t* value, int x,int y,int w,int h, HFONT f,COLORREF color,UINT flags=DT_LEFT|DT_VCENTER|DT_SINGLELINE) {
    RECT r{S(x),S(y),S(x+w),S(y+h)}; HGDIOBJ old=SelectObject(dc,f);
    SetBkMode(dc,TRANSPARENT); SetTextColor(dc,color); DrawTextW(dc,value,-1,&r,flags|DT_NOPREFIX); SelectObject(dc,old);
}
BOOL CALLBACK findClassic(HWND window, LPARAM data) {
    DWORD process=0; GetWindowThreadProcessId(window,&process);
    if (process != GetCurrentProcessId() || window == mainWindow || GetWindow(window,GW_OWNER)) return TRUE;
    wchar_t title[128]{}; GetWindowTextW(window,title,128);
    if (wcscmp(title,L"Aogiri Green Launcher") != 0) return TRUE;
    *reinterpret_cast<HWND*>(data)=window; return FALSE;
}
void updateClassic() {
    if (!classicWindow || !IsWindow(classicWindow)) {
        classicWindow=nullptr; EnumWindows(findClassic,reinterpret_cast<LPARAM>(&classicWindow));
    }
    if (classicWindow && !classicVisible && IsWindowVisible(classicWindow)) {
        bool foreground=GetForegroundWindow()==classicWindow;
        ShowWindow(classicWindow,SW_HIDE);
        if (foreground && !hiddenClassicOnce) SetForegroundWindow(mainWindow);
        hiddenClassicOnce=true;
    }
    EnableWindow(classicButton,classicWindow != nullptr);
}
const wchar_t* statusTitle(LauncherStatus s) {
    switch(s) {
    case LauncherStatus::GameDetected: return L"Minecraft найден";
    case LauncherStatus::Loading: return L"Подключаем клиент";
    case LauncherStatus::Loaded: return L"DLL загружена";
    case LauncherStatus::Failed: return L"Не удалось подключить клиент";
    case LauncherStatus::MultipleGames: return L"Найдено несколько игр";
    default: return L"Ожидание игры";
    }
}
const wchar_t* statusDetail(LauncherStatus s) {
    switch(s) {
    case LauncherStatus::GameDetected: return L"Ждём завершения запуска игры.";
    case LauncherStatus::Loading: return L"Это может занять несколько секунд.";
    case LauncherStatus::Loaded: return L"Проверьте оверлей в игре. Не закрывайте лаунчер.";
    case LauncherStatus::Failed: return L"Проверьте подробности в журнале ниже.";
    case LauncherStatus::MultipleGames: return L"Закройте лишние процессы и перезапустите лаунчер.";
    default: return L"Нажмите «Начать игру» — клиент подключится автоматически.";
    }
}
void placeControls() {
    MoveWindow(playButton,S(252),S(255),S(205),S(48),TRUE);
    MoveWindow(classicButton,S(226),S(462),S(205),S(40),TRUE);
    MoveWindow(logButton,S(440),S(462),S(160),S(40),TRUE);
}
void drawButton(const DRAWITEMSTRUCT* d) {
    int id=static_cast<int>(d->CtlID); bool pressed=(d->itemState&ODS_SELECTED)!=0;
    bool disabled=(d->itemState&ODS_DISABLED)!=0; HDC dc=d->hDC; RECT r=d->rcItem;
    COLORREF bg = id==PLAY ? (pressed?rgb(35,168,115):rgb(50,205,142)) : (pressed?rgb(37,65,57):rgb(29,48,43));
    if (disabled) bg=rgb(34,43,40);
    fill(dc,r,id==PLAY?rgb(20,43,33):rgb(14,24,21));
    HBRUSH b=CreateSolidBrush(bg); HGDIOBJ ob=SelectObject(dc,b); HGDIOBJ op=SelectObject(dc,GetStockObject(NULL_PEN));
    RoundRect(dc,r.left,r.top,r.right,r.bottom,S(10),S(10)); SelectObject(dc,op); SelectObject(dc,ob); DeleteObject(b);
    wchar_t label[128]{}; GetWindowTextW(d->hwndItem,label,128);
    HGDIOBJ old=SelectObject(dc,fontBold); SetBkMode(dc,TRANSPARENT);
    SetTextColor(dc,disabled?rgb(107,120,114):(id==PLAY?rgb(8,37,27):rgb(208,225,216)));
    DrawTextW(dc,label,-1,&r,DT_CENTER|DT_VCENTER|DT_SINGLELINE|DT_NOPREFIX); SelectObject(dc,old);
    if (d->itemState & ODS_FOCUS) { InflateRect(&r,-S(5),-S(5)); DrawFocusRect(dc,&r); }
}
void paint(HWND window) {
    PAINTSTRUCT ps{}; HDC screen=BeginPaint(window,&ps); RECT client{}; GetClientRect(window,&client);
    HDC dc=CreateCompatibleDC(screen); HBITMAP bitmap=CreateCompatibleBitmap(screen,client.right,client.bottom);
    if (!dc || !bitmap) { if(bitmap) DeleteObject(bitmap); if(dc) DeleteDC(dc); EndPaint(window,&ps); return; }
    HGDIOBJ old=SelectObject(dc,bitmap); fill(dc,client,rgb(14,24,21));
    RECT sidebar{0,0,S(190),client.bottom}; fill(dc,sidebar,rgb(9,18,15));
    rounded(dc,23,28,40,40,11,rgb(52,208,143));
    text(dc,L"G",23,28,40,40,fontBrand,rgb(8,38,26),DT_CENTER|DT_VCENTER|DT_SINGLELINE);
    text(dc,L"GAYCLIENT",74,28,105,25,fontBrand,rgb(236,247,240));
    text(dc,L"GAME LAUNCHER",75,52,105,17,fontSmall,rgb(111,143,125));
    text(dc,L"НАВИГАЦИЯ",25,99,145,19,fontSmall,rgb(91,123,105));
    rounded(dc,14,126,162,42,10,rgb(23,52,40));
    text(dc,L"●",28,126,18,42,fontSmall,rgb(64,217,151));
    text(dc,L"Главная",51,126,115,42,fontBold,rgb(230,247,237));
    text(dc,L"MINECRAFT",25,430,145,19,fontSmall,rgb(91,123,105));
    text(dc,L"Windows 10",25,454,145,22,fontBody,rgb(197,218,203));
    text(dc,L"Версия 1.1.5",25,479,145,18,fontSmall,rgb(111,143,125));
    text(dc,L"ВАШ ЛАУНЧЕР",226,28,230,22,fontSmall,rgb(112,158,130));
    text(dc,L"ИГРА  /  КЛИЕНТ",697,28,179,22,fontSmall,rgb(112,143,124),DT_RIGHT|DT_VCENTER|DT_SINGLELINE);

    drawHero(dc);
    rounded(dc,252,100,124,24,12,rgb(29,75,52));
    text(dc,L"ВАШ МИР ЖДЁТ",261,100,110,24,fontSmall,rgb(115,232,165));
    text(dc,L"Готов к новым\nприключениям?",252,141,310,74,fontTitle,rgb(245,251,246),DT_LEFT|DT_WORDBREAK);
    text(dc,L"Запускай игру — клиент\nподключится автоматически.",252,215,270,37,fontSmall,rgb(186,211,193),DT_LEFT|DT_WORDBREAK);

    rounded(dc,226,343,650,97,14,rgb(22,37,31));
    auto s=state.load(std::memory_order_acquire);
    rounded(dc,246,366,10,10,10,(s==LauncherStatus::Failed || s==LauncherStatus::MultipleGames)?rgb(239,168,90):rgb(59,215,148));
    bool pending = s==LauncherStatus::Waiting && GetTickCount64()<launchPendingUntil;
    text(dc,L"СТАТУС ПОДКЛЮЧЕНИЯ",265,354,300,18,fontSmall,rgb(108,147,122));
    text(dc,pending?L"Запрос на запуск отправлен":statusTitle(s),265,376,560,23,fontBold,rgb(232,244,235));
    text(dc,pending?L"Ожидаем появления Minecraft.":statusDetail(s),265,404,580,28,fontSmall,rgb(160,185,169),DT_LEFT|DT_WORDBREAK);
    text(dc,L"Не закрывайте лаунчер во время игры",616,464,260,38,fontSmall,rgb(125,153,136),DT_RIGHT|DT_VCENTER|DT_SINGLELINE);
    BitBlt(screen,0,0,client.right,client.bottom,dc,0,0,SRCCOPY);
    SelectObject(dc,old); DeleteObject(bitmap); DeleteDC(dc); EndPaint(window,&ps);
}

void openLog(HWND window) {
    wchar_t path[32768]{}; DWORD n=GetModuleFileNameW(nullptr,path,32768);
    if (!n || n>=32768) { MessageBoxW(window,L"Не удалось определить папку клиента.",L"Журнал",MB_OK|MB_ICONWARNING); return; }
    wchar_t* slash=wcsrchr(path,L'\\');
    if (!slash || (slash-path)+20>=32768) return;
    wcscpy(slash+1,L"client-autoload.log");
    if (GetFileAttributesW(path)==INVALID_FILE_ATTRIBUTES) {
        MessageBoxW(window,L"Журнал пока не создан. Запустите игру и повторите попытку.",L"Журнал",MB_OK|MB_ICONINFORMATION); return;
    }
    if (reinterpret_cast<INT_PTR>(ShellExecuteW(window,L"open",path,nullptr,nullptr,SW_SHOWNORMAL))<=32)
        MessageBoxW(window,L"Не удалось открыть журнал рядом с клиентом.",L"Журнал",MB_OK|MB_ICONWARNING);
}
void launchGame(HWND window) {
    if (GetTickCount64()<launchPendingUntil) return;
    auto s=state.load(std::memory_order_acquire);
    if (s==LauncherStatus::GameDetected || s==LauncherStatus::Loading || s==LauncherStatus::Loaded) {
        MessageBoxW(window,L"Minecraft уже обнаружен. Переключитесь в игру; повторный запуск не требуется.",L"GAYClient",MB_OK|MB_ICONINFORMATION); return;
    }
    if (s==LauncherStatus::MultipleGames) {
        MessageBoxW(window,L"Найдено несколько процессов Minecraft. Закройте лишние и перезапустите лаунчер.",L"GAYClient",MB_OK|MB_ICONWARNING); return;
    }
    auto result=reinterpret_cast<INT_PTR>(ShellExecuteW(window,L"open",L"shell:AppsFolder\\Microsoft.MinecraftUWP_8wekyb3d8bbwe!App",nullptr,nullptr,SW_SHOWNORMAL));
    if (result<=32) MessageBoxW(window,L"Windows не смогла запустить Minecraft. Попробуйте кнопку «Старый интерфейс» и выберите игру там.",L"Ошибка запуска",MB_OK|MB_ICONERROR);
    else { launchPendingUntil=GetTickCount64()+10000; InvalidateRect(window,nullptr,FALSE); }
}
LRESULT CALLBACK windowProc(HWND window,UINT msg,WPARAM w,LPARAM l) {
    switch(msg) {
    case WM_CREATE:
        mainWindow=window; makeFonts();
        playButton=CreateWindowExW(0,L"BUTTON",L"▶   Начать игру",WS_CHILD|WS_VISIBLE|WS_TABSTOP|BS_OWNERDRAW,0,0,0,0,window,reinterpret_cast<HMENU>(PLAY),nullptr,nullptr);
        classicButton=CreateWindowExW(0,L"BUTTON",L"Старый интерфейс",WS_CHILD|WS_VISIBLE|WS_TABSTOP|BS_OWNERDRAW,0,0,0,0,window,reinterpret_cast<HMENU>(CLASSIC),nullptr,nullptr);
        logButton=CreateWindowExW(0,L"BUTTON",L"Открыть журнал",WS_CHILD|WS_VISIBLE|WS_TABSTOP|BS_OWNERDRAW,0,0,0,0,window,reinterpret_cast<HMENU>(LOG),nullptr,nullptr);
        if (!playButton || !classicButton || !logButton) return -1;
        placeControls(); EnableWindow(classicButton,FALSE); SetTimer(window,1,500,nullptr); return 0;
    case WM_TIMER: updateClassic(); InvalidateRect(window,nullptr,FALSE); return 0;
    case WM_PAINT: paint(window); return 0;
    case WM_ERASEBKGND: return 1;
    case WM_DRAWITEM: drawButton(reinterpret_cast<const DRAWITEMSTRUCT*>(l)); return TRUE;
    case WM_COMMAND:
        if (HIWORD(w)==BN_CLICKED) { switch(LOWORD(w)) {
        case PLAY: launchGame(window); break;
        case LOG: openLog(window); break;
        case CLASSIC:
            updateClassic(); if (classicWindow && IsWindow(classicWindow)) {
                classicVisible=!classicVisible;
                ShowWindow(classicWindow,classicVisible?SW_RESTORE:SW_HIDE);
                SetWindowTextW(classicButton,classicVisible?L"Скрыть старое окно":L"Старый интерфейс");
                if (classicVisible) SetForegroundWindow(classicWindow);
            } break;
        } }
        return 0;
    case WM_DPICHANGED: {
        dpi=HIWORD(w); makeFonts(); placeControls();
        const RECT* r=reinterpret_cast<const RECT*>(l);
        SetWindowPos(window,nullptr,r->left,r->top,r->right-r->left,r->bottom-r->top,SWP_NOZORDER|SWP_NOACTIVATE); InvalidateRect(window,nullptr,TRUE); return 0;
    }
    case WM_CLOSE:
        KillTimer(window,1);
        // Closing the classic window lets the original application exit cleanly,
        // taking the autoload and wheel threads with it.
        updateClassic();
        if (classicWindow && IsWindow(classicWindow)) PostMessageW(classicWindow,WM_CLOSE,0,0);
        else ExitProcess(0);
        DestroyWindow(window); return 0;
    case WM_DESTROY: disposeFonts(); PostQuitMessage(0); return 0;
    }
    return DefWindowProcW(window,msg,w,l);
}
DWORD WINAPI uiThread(void*) {
    // Set awareness on this thread only: the original GUI keeps its own policy.
    using SetDpiContext = HANDLE (WINAPI*)(HANDLE);
    HMODULE user=GetModuleHandleW(L"user32.dll");
    auto setContext=reinterpret_cast<SetDpiContext>(GetProcAddress(user,"SetThreadDpiAwarenessContext"));
    if (setContext) setContext(reinterpret_cast<HANDLE>(-4));
    HDC dc=GetDC(nullptr); if(dc) { dpi=GetDeviceCaps(dc,LOGPIXELSX); ReleaseDC(nullptr,dc); }
    HRESULT com=CoInitializeEx(nullptr,COINIT_APARTMENTTHREADED|COINIT_DISABLE_OLE1DDE);
    loadHero();
    HINSTANCE instance=GetModuleHandleW(nullptr);
    WNDCLASSEXW wc{}; wc.cbSize=sizeof(wc); wc.hInstance=instance; wc.lpfnWndProc=windowProc;
    wc.lpszClassName=L"GAYClientModernLauncherV1"; wc.hCursor=LoadCursorW(nullptr,IDC_ARROW); wc.hIcon=LoadIconW(nullptr,IDI_APPLICATION);
    if (!RegisterClassExW(&wc)) { unloadHero(); if (SUCCEEDED(com)) CoUninitialize(); return 1; }
    DWORD style=WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX|WS_CLIPCHILDREN;
    RECT rect{0,0,S(900),S(530)}; AdjustWindowRectEx(&rect,style,FALSE,0);
    HWND window=CreateWindowExW(0,wc.lpszClassName,L"GAYClient",style,CW_USEDEFAULT,CW_USEDEFAULT,
                                rect.right-rect.left,rect.bottom-rect.top,nullptr,nullptr,instance,nullptr);
    if (!window) { unloadHero(); if (SUCCEEDED(com)) CoUninitialize(); return 1; }
    ShowWindow(window,SW_SHOW); UpdateWindow(window);
    MSG message{};
    while (GetMessageW(&message,nullptr,0,0)>0) {
        if (!IsDialogMessageW(window,&message)) { TranslateMessage(&message); DispatchMessageW(&message); }
    }
    unloadHero();
    if (SUCCEEDED(com)) CoUninitialize();
    return 0;
}
}
void launcherSetStatus(LauncherStatus value) { state.store(value,std::memory_order_release); }
bool launcherStartUi() {
    HANDLE thread=CreateThread(nullptr,0,uiThread,nullptr,0,nullptr);
    if (!thread) return false;
    CloseHandle(thread); return true;
}
