#pragma once
#include "settings.h"
#include <array>
namespace module_hud {
struct Entry { ag::ModuleId id; const char* name; };
inline constexpr Entry entries[] = {{ag::ModuleId::Fps,"FPS"},{ag::ModuleId::Cps,"CPS"},
 {ag::ModuleId::Clock,"Clock"},{ag::ModuleId::Cpu,"CPU"},{ag::ModuleId::Ram,"RAM"},
 {ag::ModuleId::Resolution,"Resolution"},{ag::ModuleId::Keystrokes,"Keystrokes"},
 {ag::ModuleId::Watermark,"Watermark"},{ag::ModuleId::Information,"Information"},
 {ag::ModuleId::Notifications,"Notifications"},{ag::ModuleId::ActiveModules,"Active Modules"},
 {ag::ModuleId::Hotkeys,"Hotkeys"},{ag::ModuleId::Coordinates,"Coordinates"}};
inline bool editing(bool menuOpen,bool editorOpen) { return menuOpen && editorOpen; }
template<class Emit> void labels(const std::array<bool,ag::kModuleCount>& enabled, bool hat, bool trail, Emit emit, bool jump=false) {
 bool any=false;
 for (const auto& e:entries) if (ag::ModuleAvailable(e.id) && enabled[static_cast<std::size_t>(e.id)]) { emit(e.name); any=true; }
 if (hat) { emit("China Hat"); any=true; }
 if (trail) { emit("Trail"); any=true; }
 if (jump) { emit("Jump Rings"); any=true; }
 if (!any) emit("No active modules");
}
}
