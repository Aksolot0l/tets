#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define UNICODE
#define _UNICODE
#include "launcher_ui.h"
#include "launcher_webview.h"
#include <shellapi.h>
#include <objbase.h>
#include <gdiplus.h>
#include "launcher_profiles_win.h"
#include "launcher_web_profiles.h"
#include "launcher_layout.h"
#include <atomic>
#include <cmath>
#include <cstring>
#include <cwchar>
#include <initializer_list>

namespace {
std::atomic<LauncherStatus> state{LauncherStatus::Waiting};
HWND mainWindow = nullptr, classicWindow = nullptr, playButton = nullptr, logButton = nullptr;
HWND homeNav=nullptr, profileNav=nullptr, profileList=nullptr, labelEdit=nullptr, nickEdit=nullptr, appEdit=nullptr;
HWND saveButton=nullptr, newButton=nullptr, deleteButton=nullptr, detectButton=nullptr;
bool hiddenClassicOnce = false, profilesPage = false, newProfile = false, storeWritable = true, webActive = false;
LauncherProfiles profiles = defaultLauncherProfiles();
HBRUSH inputBrush = nullptr, listBrush = nullptr;
ULONGLONG launchPendingUntil = 0;
int dpi = 96;
HFONT fontSmall = nullptr, fontBody = nullptr, fontBold = nullptr, fontTitle = nullptr, fontBrand = nullptr;
ULONG_PTR gdiplusToken = 0;
Gdiplus::Image* heroImage = nullptr;
Gdiplus::Image* mascotImage = nullptr;
IStream* heroStream = nullptr;
IStream* mascotStream = nullptr;
constexpr int PLAY=101, LOG=103, HOME=104, PROFILES=105, LIST=106, LABEL=107, NICK=108, APPID=109,
              SAVE=110, NEW_PROFILE=111, DELETE_PROFILE=112, DETECT=113;
int S(int n) { return MulDiv(n, dpi, 96); }
LauncherLayout layout() {
    RECT r{}; if (mainWindow) GetClientRect(mainWindow,&r);
    return makeLauncherLayout(MulDiv(r.right-r.left,96,dpi), MulDiv(r.bottom-r.top,96,dpi));
}
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
HMODULE launcherModule() {
    HMODULE module=nullptr;
    GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                       reinterpret_cast<LPCWSTR>(&launcherStartUi),&module);
    return module;
}
void loadImageResource(int id, Gdiplus::Image*& image, IStream*& stream) {
    HMODULE module=launcherModule(); if (!module) return;
    HRSRC resource=FindResourceW(module,MAKEINTRESOURCEW(id),RT_RCDATA);
    if (!resource) return;
    DWORD size=SizeofResource(module,resource); HGLOBAL found=LoadResource(module,resource);
    const void* bytes=found?LockResource(found):nullptr;
    if (!bytes || !size) return;
    HGLOBAL memory=GlobalAlloc(GMEM_MOVEABLE,size);
    if (!memory) return;
    void* target=GlobalLock(memory);
    if (!target) { GlobalFree(memory); return; }
    std::memcpy(target,bytes,size); GlobalUnlock(memory);
    if (FAILED(CreateStreamOnHGlobal(memory,TRUE,&stream))) { GlobalFree(memory); return; }
    image=Gdiplus::Image::FromStream(stream);
    if (!image || image->GetLastStatus()!=Gdiplus::Ok) { delete image; image=nullptr; stream->Release(); stream=nullptr; }
}
void loadHero() {
    Gdiplus::GdiplusStartupInput input;
    if (Gdiplus::GdiplusStartup(&gdiplusToken, &input, nullptr) != Gdiplus::Ok) { gdiplusToken=0; return; }
    loadImageResource(101,heroImage,heroStream);
    loadImageResource(102,mascotImage,mascotStream);
}
void unloadHero() {
    delete heroImage; heroImage=nullptr;
    delete mascotImage; mascotImage=nullptr;
    if (heroStream) { heroStream->Release(); heroStream=nullptr; }
    if (mascotStream) { mascotStream->Release(); mascotStream=nullptr; }
    if (gdiplusToken) { Gdiplus::GdiplusShutdown(gdiplusToken); gdiplusToken=0; }
}
void drawMascot(HDC dc, int x, int y, int size) {
    if (!mascotImage) return;
    Gdiplus::Graphics graphics(dc);
    graphics.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);
    graphics.DrawImage(mascotImage,Gdiplus::Rect(S(x),S(y),S(size),S(size)),0,0,
                       static_cast<int>(mascotImage->GetWidth()),static_cast<int>(mascotImage->GetHeight()),Gdiplus::UnitPixel);
}
void drawHero(HDC dc, const UiRect& box) {
    rounded(dc,box.x,box.y,box.w,box.h,18,rgb(20,43,33));
    if (!heroImage) return;
    Gdiplus::Graphics g(dc);
    g.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);
    Gdiplus::GraphicsPath clip;
    const int x=S(box.x), y=S(box.y), w=S(box.w), h=S(box.h), r=S(18);
    clip.AddArc(x,y,r,r,180,90); clip.AddArc(x+w-r,y,r,r,270,90);
    clip.AddArc(x+w-r,y+h-r,r,r,0,90); clip.AddArc(x,y+h-r,r,r,90,90); clip.CloseFigure();
    g.SetClip(&clip);
    const int imageW=std::clamp(box.w*60/100,320,700);
    const int drift=static_cast<int>(3.0*std::sin(static_cast<double>(GetTickCount64()%12000)/1900.0));
    const int imageX=box.x+box.w-imageW-5+drift;
    const float sw=static_cast<float>(heroImage->GetWidth()), sh=static_cast<float>(heroImage->GetHeight());
    const float aspect=static_cast<float>(imageW+10)/static_cast<float>(box.h);
    float cropW=sw, cropH=sh;
    if (sw/sh>aspect) cropW=sh*aspect; else cropH=sw/aspect;
    g.DrawImage(heroImage,Gdiplus::Rect(S(imageX),y,S(imageW+10),h),
                (sw-cropW)*0.5f,(sh-cropH)*0.5f,cropW,cropH,Gdiplus::UnitPixel);
    Gdiplus::LinearGradientBrush shade(Gdiplus::Point(S(imageX-16),y),Gdiplus::Point(S(imageX+160),y),
                                       Gdiplus::Color(255,20,43,33),Gdiplus::Color(0,20,43,33));
    g.FillRectangle(&shade,S(imageX-16),y,S(176),h);
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
    if (classicWindow && IsWindowVisible(classicWindow)) {
        bool foreground=GetForegroundWindow()==classicWindow;
        ShowWindow(classicWindow,SW_HIDE);
        if (foreground && !hiddenClassicOnce) SetForegroundWindow(mainWindow);
        hiddenClassicOnce=true;
    }
}
std::wstring readText(HWND control) {
    int n=GetWindowTextLengthW(control); if (n<0 || n>1024) return {};
    std::wstring value(static_cast<size_t>(n)+1,L'\0');
    int copied=GetWindowTextW(control,&value[0],n+1); value.resize(static_cast<size_t>(copied)); return value;
}
void putProfile(const LauncherProfile& p) {
    SetWindowTextW(labelEdit,toWide(p.label).c_str()); SetWindowTextW(nickEdit,toWide(p.nickname).c_str());
    SetWindowTextW(appEdit,toWide(p.aumid).c_str());
}
void refillProfiles() {
    SendMessageW(profileList,LB_RESETCONTENT,0,0);
    for (const auto& p:profiles.entries) { auto name=toWide(p.label); SendMessageW(profileList,LB_ADDSTRING,0,reinterpret_cast<LPARAM>(name.c_str())); }
    SendMessageW(profileList,LB_SETCURSEL,profiles.selected,0);
    newProfile=false; putProfile(profiles.entries[profiles.selected]);
}
bool persist(const LauncherProfiles& next) {
    if (!storeWritable || !saveLauncherProfiles(next)) {
        MessageBoxW(mainWindow,L"Не удалось сохранить профили. Существующий файл не изменён; проверьте доступ к папке LocalAppData.",L"Профили",MB_OK|MB_ICONERROR); return false;
    }
    profiles=next; return true;
}
void switchPage(bool page) {
    profilesPage=page;
    for (HWND h : {profileList,labelEdit,nickEdit,appEdit,saveButton,newButton,deleteButton,detectButton}) ShowWindow(h,page?SW_SHOW:SW_HIDE);
    for (HWND h : {playButton,logButton}) ShowWindow(h,page?SW_HIDE:SW_SHOW);
    if (page) refillProfiles();
    InvalidateRect(homeNav,nullptr,TRUE); InvalidateRect(profileNav,nullptr,TRUE); InvalidateRect(mainWindow,nullptr,FALSE);
}
void saveEditedProfile() {
    LauncherProfile p{toUtf8(readText(labelEdit)),toUtf8(readText(nickEdit)),"1.1.5",toUtf8(readText(appEdit))};
    if (!validProfile(p)) { MessageBoxW(mainWindow,L"Укажите название профиля. Проверьте длину полей.",L"Профили",MB_OK|MB_ICONWARNING); return; }
    if (!p.aumid.empty() && !validAppId(toWide(p.aumid))) { MessageBoxW(mainWindow,L"Некорректный App ID. Запустите игру и нажмите «Найти игру».",L"Профили",MB_OK|MB_ICONWARNING); return; }
    auto next=profiles;
    if (newProfile) {
        if(next.entries.size()>=200) { MessageBoxW(mainWindow,L"Максимум 200 профилей.",L"Профили",MB_OK|MB_ICONWARNING); return; }
        next.entries.push_back(p); next.selected=next.entries.size()-1;
    } else { p.version=next.entries[next.selected].version; next.entries[next.selected]=p; }
    if (persist(next)) refillProfiles();
}
void deleteSelectedProfile() {
    if (newProfile) { refillProfiles(); return; }
    if (profiles.entries.size()<=1) { MessageBoxW(mainWindow,L"Оставьте хотя бы один профиль.",L"Профили",MB_OK|MB_ICONINFORMATION); return; }
    if (MessageBoxW(mainWindow,L"Удалить локальный профиль? Аккаунт и файлы игры не изменятся.",L"Удаление",MB_YESNO|MB_ICONQUESTION)!=IDYES) return;
    auto next=profiles; next.entries.erase(next.entries.begin()+next.selected);
    if(next.selected>=next.entries.size()) next.selected=next.entries.size()-1;
    if(persist(next)) refillProfiles();
}
void detectApp() {
    auto id=detectMinecraftAppId();
    if (id.empty()) { MessageBoxW(mainWindow,L"Запустите одно окно Minecraft через меню «Пуск», затем снова нажмите «Найти игру». Если окон несколько, закройте лишние.",L"Поиск игры",MB_OK|MB_ICONINFORMATION); return; }
    SetWindowTextW(appEdit,id.c_str());
    MessageBoxW(mainWindow,L"Игра найдена. Нажмите «Сохранить», чтобы использовать её для этого профиля.",L"Поиск игры",MB_OK|MB_ICONINFORMATION);
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
    if (!homeNav || !profileList || !appEdit || !saveButton) return;
    const auto l=layout();
    auto move = [](HWND window,const UiRect& r) { MoveWindow(window,S(r.x),S(r.y),S(r.w),S(r.h),TRUE); };
    move(homeNav,{14,126,162,42}); move(profileNav,{14,175,162,42});
    move(playButton,l.play); move(logButton,l.log); move(profileList,l.profileList);
    move(newButton,l.profileNew); move(deleteButton,l.profileDelete);
    move(labelEdit,l.labelEdit); move(nickEdit,l.nickEdit); move(appEdit,l.appEdit);
    move(detectButton,l.detect); move(saveButton,l.save);
    for (HWND h : {profileList,labelEdit,nickEdit,appEdit}) SendMessageW(h,WM_SETFONT,reinterpret_cast<WPARAM>(fontBody),TRUE);
    SendMessageW(profileList,LB_SETITEMHEIGHT,0,S(40));
    for (HWND h : {labelEdit,nickEdit,appEdit})
        SendMessageW(h,EM_SETMARGINS,EC_LEFTMARGIN|EC_RIGHTMARGIN,MAKELPARAM(S(10),S(10)));
}
void drawProfileItem(const DRAWITEMSTRUCT* item) {
    if (item->itemID==static_cast<UINT>(-1)) return;
    HDC dc=item->hDC; RECT box=item->rcItem;
    bool selected=(item->itemState&ODS_SELECTED)!=0;
    fill(dc,box,rgb(23,40,32));
    if (selected) {
        RECT selection=box; InflateRect(&selection,-S(5),-S(3));
        HBRUSH b=CreateSolidBrush(rgb(32,76,53)); HGDIOBJ oldBrush=SelectObject(dc,b);
        HGDIOBJ oldPen=SelectObject(dc,GetStockObject(NULL_PEN));
        RoundRect(dc,selection.left,selection.top,selection.right,selection.bottom,S(8),S(8));
        SelectObject(dc,oldPen); SelectObject(dc,oldBrush); DeleteObject(b);
    }
    wchar_t label[512]{};
    auto count=SendMessageW(item->hwndItem,LB_GETTEXTLEN,item->itemID,0);
    if (count>=0 && count<static_cast<LRESULT>(sizeof(label)/sizeof(label[0])))
        SendMessageW(item->hwndItem,LB_GETTEXT,item->itemID,reinterpret_cast<LPARAM>(label));
    RECT textBox=box; textBox.left+=S(14); textBox.right-=S(8);
    HGDIOBJ old=SelectObject(dc,selected?fontBold:fontBody);
    SetBkMode(dc,TRANSPARENT); SetTextColor(dc,selected?rgb(230,248,234):rgb(177,202,184));
    DrawTextW(dc,label,-1,&textBox,DT_LEFT|DT_VCENTER|DT_SINGLELINE|DT_NOPREFIX|DT_END_ELLIPSIS);
    SelectObject(dc,old);
    if (item->itemState&ODS_FOCUS) { InflateRect(&box,-S(3),-S(3)); DrawFocusRect(dc,&box); }
}
void drawButton(const DRAWITEMSTRUCT* d) {
    int id=static_cast<int>(d->CtlID); bool pressed=(d->itemState&ODS_SELECTED)!=0;
    bool disabled=(d->itemState&ODS_DISABLED)!=0; HDC dc=d->hDC; RECT r=d->rcItem;
    POINT cursor{}; GetCursorPos(&cursor); bool hovered=WindowFromPoint(cursor)==d->hwndItem;
    bool nav=id==HOME || id==PROFILES;
    bool activeNav=(id==HOME&&!profilesPage)||(id==PROFILES&&profilesPage);
    COLORREF bg = (id==PLAY || id==SAVE) ? (pressed?rgb(35,168,115):hovered?rgb(69,226,157):rgb(50,205,142))
                  : nav ? (activeNav?rgb(23,52,40):hovered?rgb(17,37,28):rgb(9,18,15))
                  : (pressed?rgb(37,65,57):hovered?rgb(39,67,54):rgb(29,48,43));
    if (disabled) bg=rgb(34,43,40);
    fill(dc,r,nav?rgb(9,18,15):id==PLAY?rgb(20,43,33):rgb(14,24,21));
    HBRUSH b=CreateSolidBrush(bg); HGDIOBJ ob=SelectObject(dc,b); HGDIOBJ op=SelectObject(dc,GetStockObject(NULL_PEN));
    RoundRect(dc,r.left,r.top,r.right,r.bottom,S(10),S(10)); SelectObject(dc,op); SelectObject(dc,ob); DeleteObject(b);
    wchar_t label[128]{}; GetWindowTextW(d->hwndItem,label,128);
    HGDIOBJ old=SelectObject(dc,fontBold); SetBkMode(dc,TRANSPARENT);
    SetTextColor(dc,disabled?rgb(107,120,114):((id==PLAY || id==SAVE)?rgb(8,37,27):rgb(208,225,216)));
    DrawTextW(dc,label,-1,&r,DT_CENTER|DT_VCENTER|DT_SINGLELINE|DT_NOPREFIX); SelectObject(dc,old);
    if (d->itemState & ODS_FOCUS) { InflateRect(&r,-S(5),-S(5)); DrawFocusRect(dc,&r); }
}
void paint(HWND window) {
    PAINTSTRUCT ps{}; HDC screen=BeginPaint(window,&ps); RECT client{}; GetClientRect(window,&client);
    if (client.right<=0 || client.bottom<=0) { EndPaint(window,&ps); return; }
    HDC dc=CreateCompatibleDC(screen); HBITMAP bitmap=CreateCompatibleBitmap(screen,client.right,client.bottom);
    if (!dc || !bitmap) { if(bitmap) DeleteObject(bitmap); if(dc) DeleteDC(dc); EndPaint(window,&ps); return; }
    HGDIOBJ old=SelectObject(dc,bitmap); fill(dc,client,rgb(14,24,21));
    const auto l=layout(); const int x=l.contentX, y=l.top, width=l.contentWidth;
    RECT sidebar{0,0,S(190),client.bottom}; fill(dc,sidebar,rgb(9,18,15));
    if (mascotImage) drawMascot(dc,20,25,46);
    else {
        rounded(dc,23,28,40,40,11,rgb(52,208,143));
        text(dc,L"G",23,28,40,40,fontBrand,rgb(8,38,26),DT_CENTER|DT_VCENTER|DT_SINGLELINE);
    }
    text(dc,L"GAYCLIENT",74,28,105,25,fontBrand,rgb(236,247,240));
    text(dc,L"GAME LAUNCHER",75,52,105,17,fontSmall,rgb(111,143,125));
    text(dc,L"НАВИГАЦИЯ",25,99,145,19,fontSmall,rgb(91,123,105));
    rounded(dc,16,l.sideCardY,158,172,16,rgb(16,33,26));
    int mascotFloat=static_cast<int>(2.0*std::sin(static_cast<double>(GetTickCount64()%5000)/800.0));
    if (mascotImage) drawMascot(dc,25,l.sideCardY+7+mascotFloat,140);
    else text(dc,L"G",25,l.sideCardY+7,140,140,fontTitle,rgb(64,217,151),DT_CENTER|DT_VCENTER|DT_SINGLELINE);
    rounded(dc,70,l.sideCardY+155,50,3,3,rgb(47,136,87));
    text(dc,L"MINECRAFT",25,l.height-72,145,19,fontSmall,rgb(91,123,105));
    text(dc,L"WINDOWS 10  /  1.1.5",25,l.height-48,145,19,fontSmall,rgb(135,171,145),DT_LEFT|DT_VCENTER|DT_SINGLELINE|DT_END_ELLIPSIS);
    text(dc,L"ВАШ ЛАУНЧЕР",x,y+28,160,22,fontSmall,rgb(112,158,130));
    auto currentName=toWide(profiles.entries[profiles.selected].label);
    auto currentCaption=std::wstring(L"Профиль: ")+currentName;
    text(dc,currentCaption.c_str(),x+170,y+28,width-170,22,fontSmall,rgb(151,185,161),DT_RIGHT|DT_VCENTER|DT_SINGLELINE|DT_END_ELLIPSIS);

    if (!profilesPage) {
        drawHero(dc,l.hero);
        const int hx=l.hero.x, hy=l.hero.y;
        rounded(dc,hx+width-133,hy+l.hero.h-39,116,26,12,rgb(11,29,20));
        drawMascot(dc,hx+width-129,hy+l.hero.h-37,22);
        text(dc,L"GAYCLIENT",hx+width-104,hy+l.hero.h-39,81,26,fontSmall,rgb(171,222,184));
        rounded(dc,hx+26,hy+25,124,24,12,rgb(29,75,52));
        text(dc,L"ВАШ МИР ЖДЁТ",hx+35,hy+25,110,24,fontSmall,rgb(115,232,165));
        text(dc,L"Готов к новым\nприключениям?",hx+26,hy+66,310,74,fontTitle,rgb(245,251,246),DT_LEFT|DT_WORDBREAK);
        text(dc,L"Запускай игру — клиент\nподключится автоматически.",hx+26,hy+140,270,37,fontSmall,rgb(186,211,193),DT_LEFT|DT_WORDBREAK);

        rounded(dc,l.status.x,l.status.y,l.status.w,l.status.h,14,rgb(22,37,31));
        auto s=state.load(std::memory_order_acquire);
        int pulse=static_cast<int>(18.0*(1.0+std::sin(static_cast<double>(GetTickCount64()%5000)/800.0)));
        rounded(dc,l.status.x+20,l.status.y+23,10,10,10,(s==LauncherStatus::Failed || s==LauncherStatus::MultipleGames)?rgb(239,168,90):rgb(45,180+pulse,125));
        bool pending = s==LauncherStatus::Waiting && GetTickCount64()<launchPendingUntil;
        text(dc,L"СТАТУС ПОДКЛЮЧЕНИЯ",l.status.x+39,l.status.y+11,width-59,18,fontSmall,rgb(108,147,122));
        text(dc,pending?L"Запрос на запуск отправлен":statusTitle(s),l.status.x+39,l.status.y+33,width-59,23,fontBold,rgb(232,244,235),DT_LEFT|DT_VCENTER|DT_SINGLELINE|DT_END_ELLIPSIS);
        text(dc,pending?L"Ожидаем появления Minecraft.":statusDetail(s),l.status.x+39,l.status.y+61,width-59,28,fontSmall,rgb(160,185,169),DT_LEFT|DT_WORDBREAK);
        text(dc,L"Не закрывайте лаунчер во время игры",x+180,l.log.y,width-180,40,fontSmall,rgb(125,153,136),DT_RIGHT|DT_VCENTER|DT_SINGLELINE|DT_END_ELLIPSIS);
    } else {
        text(dc,L"Профили",x,y+75,width,42,fontTitle,rgb(241,248,243));
        text(dc,L"Выберите профиль или создайте новый.",x,y+123,width,24,fontBody,rgb(159,183,167));
        auto field=[&](const UiRect& r,const wchar_t* title) {
            text(dc,title,r.x,r.y-30,r.w,20,fontSmall,rgb(125,158,137),DT_LEFT|DT_VCENTER|DT_SINGLELINE|DT_END_ELLIPSIS);
            rounded(dc,r.x-2,r.y-7,r.w+4,r.h+14,8,rgb(29,48,40));
        };
        field(l.labelEdit,L"НАЗВАНИЕ ПРОФИЛЯ");
        field(l.nickEdit,L"НИК — ЛОКАЛЬНАЯ ЗАМЕТКА");
        field(l.appEdit,L"APP ID ИГРЫ");
        text(dc,L"Ник в профиле не меняет имя в игре или учётную запись.",l.labelEdit.x,l.detect.y+54,l.labelEdit.w,40,fontSmall,rgb(144,177,154),DT_LEFT|DT_WORDBREAK);
    }
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
    auto appId=toWide(profiles.entries[profiles.selected].aumid);
    if (!validAppId(appId)) {
        MessageBoxW(window,L"Игра ещё не выбрана для этого профиля. Откройте Minecraft через меню «Пуск», затем в разделе «Профили» нажмите «Найти игру» и «Сохранить».",L"Выберите игру",MB_OK|MB_ICONINFORMATION);
        switchPage(true); return;
    }
    auto target=std::wstring(L"shell:AppsFolder\\")+appId;
    auto result=reinterpret_cast<INT_PTR>(ShellExecuteW(window,L"open",target.c_str(),nullptr,nullptr,SW_SHOWNORMAL));
    if (result<=32) MessageBoxW(window,L"Windows не запустила выбранную игру. Запустите её через меню «Пуск», затем в разделе «Профили» нажмите «Найти игру» и сохраните профиль.",L"Ошибка запуска",MB_OK|MB_ICONERROR);
    else { launchPendingUntil=GetTickCount64()+10000; InvalidateRect(window,nullptr,FALSE); }
}

// The web interface sends only typed, whitelisted requests. Profiles remain in
// the same validated, atomic native store used by the existing launcher.
const char* webStatus() {
    switch (state.load(std::memory_order_acquire)) {
    case LauncherStatus::GameDetected: return "detected";
    case LauncherStatus::Loading: return "loading";
    case LauncherStatus::Loaded: return "loaded";
    case LauncherStatus::Failed: case LauncherStatus::MultipleGames: return "error";
    default: return "idle";
    }
}
nlohmann::json webState() {
    nlohmann::json result={{"profiles",nlohmann::json::array()},{"selectedId",std::to_string(profiles.selected)},
                           {"writable",storeWritable},{"status",webStatus()}};
    for (size_t i=0;i<profiles.entries.size();++i) {
        const auto& p=profiles.entries[i];
        result["profiles"].push_back({{"id",std::to_string(i)},{"label",p.label},{"nickname",p.nickname},
                                       {"version",p.version},{"aumid",p.aumid},{"createdAt",""}});
    }
    return result;
}
std::string webDispatch(const std::string& message) {
    using json=nlohmann::json;
    json response={{"id",0},{"ok",false}};
    try {
        auto request=json::parse(message);
        if (!request.is_object() || !request.contains("id") || !request["id"].is_number_unsigned() ||
            !request.contains("method") || !request["method"].is_string()) throw std::runtime_error("Некорректный запрос.");
        response["id"]=request["id"];
        const std::string method=request["method"].get<std::string>();
        const auto args=request.value("args",json::object());
        if (!args.is_object()) throw std::runtime_error("Некорректные параметры.");
        json result=json::object();
        if (method=="ready") { result=webState(); response["readyMarker"]=true; }
        else if (method=="selectProfile" || method=="saveProfile" || method=="deleteProfile") {
            auto next=planWebProfileChange(profiles,method,args,storeWritable);
            if (!saveLauncherProfiles(next)) throw std::runtime_error("Не удалось сохранить профили. Существующий файл не изменён.");
            profiles=std::move(next); result=webState();
        } else if (method=="detectGame") {
            const auto id=detectMinecraftAppId();
            if (id.empty()) throw std::runtime_error("Запустите одно окно Minecraft через меню Пуск и повторите поиск.");
            result={{"aumid",toUtf8(id)}};
        } else if (method=="launch") {
            auto status=state.load(std::memory_order_acquire);
            if (status==LauncherStatus::GameDetected || status==LauncherStatus::Loading || status==LauncherStatus::Loaded)
                throw std::runtime_error("Minecraft уже обнаружен. Переключитесь в игру.");
            if (status==LauncherStatus::MultipleGames) throw std::runtime_error("Найдено несколько игр. Закройте лишние и перезапустите клиент.");
            if (GetTickCount64()<launchPendingUntil) throw std::runtime_error("Запрос уже отправлен. Ожидаем появления игры.");
            const auto id=toWide(profiles.entries[profiles.selected].aumid);
            if (!validAppId(id)) throw std::runtime_error("Выберите игру: запустите Minecraft, в Профилях нажмите Найти игру и Сохранить.");
            const auto target=std::wstring(L"shell:AppsFolder\\")+id;
            if (reinterpret_cast<INT_PTR>(ShellExecuteW(mainWindow,L"open",target.c_str(),nullptr,nullptr,SW_SHOWNORMAL))<=32)
                throw std::runtime_error("Windows не запустила игру. Запустите её через Пуск, найдите App ID и сохраните профиль.");
            launchPendingUntil=GetTickCount64()+10000;
        } else if (method=="openLog") openLog(mainWindow);
        else if (method=="minimize") ShowWindow(mainWindow,SW_MINIMIZE);
        else if (method=="maximize") ShowWindow(mainWindow,IsZoomed(mainWindow)?SW_RESTORE:SW_MAXIMIZE);
        else if (method=="close") PostMessageW(mainWindow,WM_CLOSE,0,0);
        else throw std::runtime_error("Неизвестная команда.");
        response["ok"]=true; response["result"]=std::move(result);
    } catch (const std::exception& e) { response["error"]=e.what(); }
    return response.dump(-1,' ',false,json::error_handler_t::replace);
}
void webReady() {
    webActive=true;
    for (HWND h : {homeNav,profileNav,profileList,labelEdit,nickEdit,appEdit,saveButton,newButton,deleteButton,detectButton,playButton,logButton}) ShowWindow(h,SW_HIDE);
}
void webFailure() { webActive=false; if (mainWindow && IsWindow(mainWindow)) switchPage(false); }
LRESULT CALLBACK windowProc(HWND window,UINT msg,WPARAM w,LPARAM l) {
    switch(msg) {
    case WM_CREATE:
        mainWindow=window; makeFonts(); inputBrush=CreateSolidBrush(rgb(29,48,40)); listBrush=CreateSolidBrush(rgb(23,40,32));
        homeNav=CreateWindowExW(0,L"BUTTON",L"Главная",WS_CHILD|WS_VISIBLE|WS_TABSTOP|BS_OWNERDRAW,0,0,0,0,window,reinterpret_cast<HMENU>(HOME),nullptr,nullptr);
        profileNav=CreateWindowExW(0,L"BUTTON",L"Профили",WS_CHILD|WS_VISIBLE|WS_TABSTOP|BS_OWNERDRAW,0,0,0,0,window,reinterpret_cast<HMENU>(PROFILES),nullptr,nullptr);
        playButton=CreateWindowExW(0,L"BUTTON",L"▶   Начать игру",WS_CHILD|WS_VISIBLE|WS_TABSTOP|BS_OWNERDRAW,0,0,0,0,window,reinterpret_cast<HMENU>(PLAY),nullptr,nullptr);
        logButton=CreateWindowExW(0,L"BUTTON",L"Открыть журнал",WS_CHILD|WS_VISIBLE|WS_TABSTOP|BS_OWNERDRAW,0,0,0,0,window,reinterpret_cast<HMENU>(LOG),nullptr,nullptr);
        profileList=CreateWindowExW(0,L"LISTBOX",L"",WS_CHILD|WS_TABSTOP|WS_VSCROLL|LBS_NOTIFY|LBS_NOINTEGRALHEIGHT|LBS_OWNERDRAWFIXED|LBS_HASSTRINGS,0,0,0,0,window,reinterpret_cast<HMENU>(LIST),nullptr,nullptr);
        labelEdit=CreateWindowExW(0,L"EDIT",L"",WS_CHILD|WS_TABSTOP|ES_AUTOHSCROLL,0,0,0,0,window,reinterpret_cast<HMENU>(LABEL),nullptr,nullptr);
        nickEdit=CreateWindowExW(0,L"EDIT",L"",WS_CHILD|WS_TABSTOP|ES_AUTOHSCROLL,0,0,0,0,window,reinterpret_cast<HMENU>(NICK),nullptr,nullptr);
        appEdit=CreateWindowExW(0,L"EDIT",L"",WS_CHILD|WS_TABSTOP|ES_AUTOHSCROLL,0,0,0,0,window,reinterpret_cast<HMENU>(APPID),nullptr,nullptr);
        saveButton=CreateWindowExW(0,L"BUTTON",L"Сохранить",WS_CHILD|WS_TABSTOP|BS_OWNERDRAW,0,0,0,0,window,reinterpret_cast<HMENU>(SAVE),nullptr,nullptr);
        newButton=CreateWindowExW(0,L"BUTTON",L"+ Новый",WS_CHILD|WS_TABSTOP|BS_OWNERDRAW,0,0,0,0,window,reinterpret_cast<HMENU>(NEW_PROFILE),nullptr,nullptr);
        deleteButton=CreateWindowExW(0,L"BUTTON",L"Удалить",WS_CHILD|WS_TABSTOP|BS_OWNERDRAW,0,0,0,0,window,reinterpret_cast<HMENU>(DELETE_PROFILE),nullptr,nullptr);
        detectButton=CreateWindowExW(0,L"BUTTON",L"Найти игру",WS_CHILD|WS_TABSTOP|BS_OWNERDRAW,0,0,0,0,window,reinterpret_cast<HMENU>(DETECT),nullptr,nullptr);
        if (!homeNav || !profileNav || !playButton || !logButton || !profileList || !labelEdit || !nickEdit || !appEdit || !saveButton || !newButton || !deleteButton || !detectButton || !inputBrush || !listBrush) return -1;
        SendMessageW(labelEdit,EM_SETLIMITTEXT,128,0); SendMessageW(nickEdit,EM_SETLIMITTEXT,128,0); SendMessageW(appEdit,EM_SETLIMITTEXT,512,0);
        placeControls(); SetTimer(window,1,125,nullptr); startLauncherWebView(window,webDispatch,webReady,webFailure); return 0;
    case WM_TIMER: {
        static ULONGLONG lastClassic=0; auto now=GetTickCount64();
        if (now-lastClassic>=500) { updateClassic(); lastClassic=now; }
        tickLauncherWebView(); updateLauncherWebViewStatus(webStatus());
        if (IsIconic(window)) return 0;
        InvalidateRect(window,nullptr,FALSE);
        for (HWND h : {homeNav,profileNav,playButton,logButton,saveButton,newButton,deleteButton,detectButton}) if (IsWindowVisible(h)) InvalidateRect(h,nullptr,FALSE);
        return 0;
    }
    case WM_PAINT: paint(window); return 0;
    case WM_ERASEBKGND: return 1;
    case WM_MEASUREITEM: {
        auto* item=reinterpret_cast<MEASUREITEMSTRUCT*>(l);
        if (item && item->CtlID==LIST) { item->itemHeight=S(40); return TRUE; }
        break;
    }
    case WM_DRAWITEM: {
        auto* item=reinterpret_cast<const DRAWITEMSTRUCT*>(l);
        if (item->CtlID==LIST) drawProfileItem(item); else drawButton(item);
        return TRUE;
    }
    case WM_SIZE:
        if (w!=SIZE_MINIMIZED) { placeControls(); resizeLauncherWebView(); InvalidateRect(window,nullptr,TRUE); }
        return 0;
    case WM_GETMINMAXINFO: {
        RECT r{0,0,S(760),S(480)};
        DWORD style=static_cast<DWORD>(GetWindowLongPtrW(window,GWL_STYLE));
        DWORD exStyle=static_cast<DWORD>(GetWindowLongPtrW(window,GWL_EXSTYLE));
        using AdjustForDpi=BOOL (WINAPI*)(LPRECT,DWORD,BOOL,DWORD,UINT);
        auto fn=reinterpret_cast<AdjustForDpi>(GetProcAddress(GetModuleHandleW(L"user32.dll"),"AdjustWindowRectExForDpi"));
        if (fn) fn(&r,style,FALSE,exStyle,static_cast<UINT>(dpi)); else AdjustWindowRectEx(&r,style,FALSE,exStyle);
        auto* limits=reinterpret_cast<MINMAXINFO*>(l);
        limits->ptMinTrackSize.x=r.right-r.left; limits->ptMinTrackSize.y=r.bottom-r.top;
        return 0;
    }
    case WM_CTLCOLOREDIT:
        SetTextColor(reinterpret_cast<HDC>(w),rgb(222,240,227)); SetBkColor(reinterpret_cast<HDC>(w),rgb(29,48,40)); return reinterpret_cast<LRESULT>(inputBrush);
    case WM_CTLCOLORLISTBOX:
        SetTextColor(reinterpret_cast<HDC>(w),rgb(222,240,227)); SetBkColor(reinterpret_cast<HDC>(w),rgb(23,40,32)); return reinterpret_cast<LRESULT>(listBrush);
    case WM_COMMAND:
        if (LOWORD(w)==LIST && HIWORD(w)==LBN_SELCHANGE) {
            auto index=SendMessageW(profileList,LB_GETCURSEL,0,0);
            if (index!=LB_ERR && static_cast<size_t>(index)<profiles.entries.size()) {
                auto next=profiles; next.selected=static_cast<size_t>(index);
                if (next.selected!=profiles.selected) persist(next);
                refillProfiles();
                InvalidateRect(window,nullptr,FALSE);
            }
            return 0;
        }
        if (HIWORD(w)==BN_CLICKED) { switch(LOWORD(w)) {
        case HOME: switchPage(false); break;
        case PROFILES: switchPage(true); break;
        case PLAY: launchGame(window); break;
        case LOG: openLog(window); break;
        case SAVE: saveEditedProfile(); break;
        case DETECT: detectApp(); break;
        case DELETE_PROFILE: deleteSelectedProfile(); break;
        case NEW_PROFILE:
            newProfile=true; SendMessageW(profileList,LB_SETCURSEL,static_cast<WPARAM>(-1),0);
            SetWindowTextW(labelEdit,L""); SetWindowTextW(nickEdit,L"");
            SetWindowTextW(appEdit,toWide(profiles.entries[profiles.selected].aumid).c_str()); SetFocus(labelEdit); break;
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
    case WM_DESTROY:
        closeLauncherWebView();
        disposeFonts();
        if(inputBrush) { DeleteObject(inputBrush); inputBrush=nullptr; }
        if(listBrush) { DeleteObject(listBrush); listBrush=nullptr; }
        PostQuitMessage(0); return 0;
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
    storeWritable=loadLauncherProfiles(profiles);
    HINSTANCE instance=GetModuleHandleW(nullptr);
    HMODULE module=launcherModule();
    HICON bigIcon=module?static_cast<HICON>(LoadImageW(module,MAKEINTRESOURCEW(103),IMAGE_ICON,S(32),S(32),0)):nullptr;
    HICON smallIcon=module?static_cast<HICON>(LoadImageW(module,MAKEINTRESOURCEW(103),IMAGE_ICON,S(16),S(16),0)):nullptr;
    auto cleanupIcons=[&]() { if (bigIcon) DestroyIcon(bigIcon); if (smallIcon) DestroyIcon(smallIcon); };
    WNDCLASSEXW wc{}; wc.cbSize=sizeof(wc); wc.hInstance=instance; wc.lpfnWndProc=windowProc;
    wc.lpszClassName=L"GAYClientModernLauncherV1"; wc.hCursor=LoadCursorW(nullptr,IDC_ARROW);
    wc.hIcon=bigIcon?bigIcon:LoadIconW(nullptr,IDI_APPLICATION); wc.hIconSm=smallIcon;
    if (!RegisterClassExW(&wc)) { cleanupIcons(); unloadHero(); if (SUCCEEDED(com)) CoUninitialize(); return 1; }
    DWORD style=WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX|WS_MAXIMIZEBOX|WS_THICKFRAME|WS_CLIPCHILDREN;
    RECT rect{0,0,S(900),S(530)}; AdjustWindowRectEx(&rect,style,FALSE,0);
    HWND window=CreateWindowExW(0,wc.lpszClassName,L"GAYClient",style,CW_USEDEFAULT,CW_USEDEFAULT,
                                rect.right-rect.left,rect.bottom-rect.top,nullptr,nullptr,instance,nullptr);
    if (!window) { UnregisterClassW(wc.lpszClassName,instance); cleanupIcons(); unloadHero(); if (SUCCEEDED(com)) CoUninitialize(); return 1; }
    ShowWindow(window,SW_SHOW); UpdateWindow(window);
    MSG message{};
    while (GetMessageW(&message,nullptr,0,0)>0) {
        if (webActive || !IsDialogMessageW(window,&message)) { TranslateMessage(&message); DispatchMessageW(&message); }
    }
    UnregisterClassW(wc.lpszClassName,instance); cleanupIcons(); unloadHero();
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
