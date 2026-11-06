#include "../src/launcher_layout.h"
#include <cassert>

static bool inside(const UiRect& r, int w, int h) {
    return r.x>=0 && r.y>=0 && r.w>0 && r.h>0 && r.x+r.w<=w && r.y+r.h<=h;
}
static bool overlaps(const UiRect& a,const UiRect& b) {
    return a.x<b.x+b.w && b.x<a.x+a.w && a.y<b.y+b.h && b.y<a.y+a.h;
}
static void verify(int w,int h) {
    const auto l=makeLauncherLayout(w,h);
    for (auto r : {l.hero,l.play,l.status,l.log,l.profileList,l.profileNew,l.profileDelete,
                   l.labelEdit,l.nickEdit,l.appEdit,l.detect,l.save}) assert(inside(r,w,h));
    assert(l.play.x>=l.hero.x && l.play.y>=l.hero.y &&
           l.play.x+l.play.w<=l.hero.x+l.hero.w && l.play.y+l.play.h<=l.hero.y+l.hero.h);
    assert(!overlaps(l.hero,l.status) && !overlaps(l.status,l.log));
    assert(!overlaps(l.profileList,l.labelEdit) && !overlaps(l.profileList,l.nickEdit) && !overlaps(l.profileList,l.appEdit));
    assert(!overlaps(l.profileNew,l.profileDelete) && !overlaps(l.detect,l.save));
    assert(l.sideCardY>=217 && l.sideCardY+172<=l.height-72);
}
int main() {
    verify(760,480); verify(900,530); verify(1024,768); verify(1366,768);
    verify(1920,1080); verify(2560,1440); verify(3840,2160);
    for (int width=760; width<=2300; width+=77)
        for (int height=480; height<=1300; height+=41) verify(width,height);
    const auto normal=makeLauncherLayout(900,530), large=makeLauncherLayout(1920,1080);
    assert(large.hero.w>normal.hero.w && large.hero.h>normal.hero.h);
    assert(large.contentX>normal.contentX && large.top>normal.top);
    assert(large.contentWidth<=1050);
}
