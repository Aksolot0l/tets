#include "../src/hud_module_list.h"
#include <cassert>
#include <string>
#include <vector>
int main() {
 using namespace module_hud;
 assert(!editing(false,false) && !editing(false,true) && !editing(true,false) && editing(true,true));
 std::array<bool,ag::kModuleCount> enabled{}; std::vector<std::string> out;
 auto add=[&](const char* s){out.emplace_back(s);};
 labels(enabled,false,false,add); assert(out.size()==1 && out[0]=="No active modules"); out.clear();
 enabled[static_cast<std::size_t>(ag::ModuleId::Fps)]=true;
 enabled[static_cast<std::size_t>(ag::ModuleId::Session)]=true; // unsupported is not shown
 labels(enabled,true,true,add,true); assert((out==std::vector<std::string>{"FPS","China Hat","Trail","Jump Rings"}));
}
