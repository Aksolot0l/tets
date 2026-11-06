#include "../src/hud_theme.h"
#include <cassert>
#include <limits>
int main() {
 assert(hud_theme::card("##hud_fps")); assert(hud_theme::card("##hud_keystrokes"));
 assert(hud_theme::card("##hud_coordinates")); assert(!hud_theme::card("HUD"));
 assert(!hud_theme::card("##hud_editor")); assert(!hud_theme::card("##hud_fps_extra"));
 assert(!hud_theme::card(nullptr)); assert(hud_theme::card("##notification_12"));
 assert(!hud_theme::card("##notification_")); assert(!hud_theme::card("##notification_else"));
 assert(hud_theme::background(0).a==0); assert(hud_theme::background(2).a==1);
 assert(hud_theme::palette(-1).bg.r==hud_theme::palette(0).bg.r);
 assert(hud_theme::palette(1).accent.g!=hud_theme::palette(2).accent.g);
 assert(hud_theme::background(.5f,1).a==.5f);
 assert(hud_theme::rounding(-1)==0 && hud_theme::rounding(21)==20);
 assert(hud_theme::rounding(std::numeric_limits<float>::quiet_NaN())==8);
 assert(hud_theme::border(0).a==0); assert(hud_theme::background(std::numeric_limits<float>::quiet_NaN()).a==.72f);
}
