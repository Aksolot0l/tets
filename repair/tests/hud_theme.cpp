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
 assert(hud_theme::border(0).a==0); assert(hud_theme::background(std::numeric_limits<float>::quiet_NaN()).a==.72f);
}
