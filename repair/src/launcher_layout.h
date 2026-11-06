#pragma once
#include <algorithm>

struct UiRect { int x, y, w, h; };
struct LauncherLayout {
    int width, height, contentX, contentWidth, top, extra, sideCardY;
    UiRect hero, play, status, log, profileList, profileNew, profileDelete;
    UiRect labelEdit, nickEdit, appEdit, detect, save;
};
// Logical pixels at 96 DPI. Keep the sidebar fixed, expand the content to a
// comfortable maximum, and center it on larger monitors without stretching
// text or bitmaps. The minimum 760x480 client is enforced by the window.
inline LauncherLayout makeLauncherLayout(int width, int height) {
    width=std::max(width,760); height=std::max(height,480);
    LauncherLayout l{}; l.width=width; l.height=height;
    constexpr int sidebar=190, margin=28;
    const int available=width-sidebar-2*margin;
    l.contentWidth=std::min(1050,available);
    l.contentX=sidebar+margin+(available-l.contentWidth)/2;
    l.extra=std::min(110,std::max(0,(height-530)/2));
    l.top=std::max(-25,(height-(530+l.extra))/2);
    l.sideCardY=std::max(224,height-268);
    const int x=l.contentX, y=l.top, w=l.contentWidth;
    l.hero={x,y+75,w,252+l.extra};
    l.play={x+26,l.hero.y+l.hero.h-72,205,48};
    l.status={x,y+343+l.extra,w,97};
    l.log={x,y+462+l.extra,160,40};
    int listWidth=std::clamp(w*31/100,160,240);
    int formX=x+listWidth+25, formWidth=w-listWidth-25;
    int split=(listWidth-8)/2, button=(formWidth-12)/2;
    l.profileList={x,y+164,listWidth,236+l.extra};
    l.profileNew={x,y+415+l.extra,split,40};
    l.profileDelete={x+split+8,y+415+l.extra,listWidth-split-8,40};
    l.labelEdit={formX,y+194,formWidth,24};
    l.nickEdit={formX,y+268+l.extra/4,formWidth,24};
    l.appEdit={formX,y+342+l.extra/2,formWidth,24};
    l.detect={formX,y+394+l.extra,button,40};
    l.save={formX+button+12,y+394+l.extra,formWidth-button-12,40};
    return l;
}
