#include "../src/launcher_web_profiles.h"
#include <cassert>
#include <iostream>
using nlohmann::json;
template<class Fn> void fails(Fn fn) { bool failed=false; try { fn(); } catch (const std::exception&) { failed=true; } assert(failed); }
int main() {
    auto original=defaultLauncherProfiles(); original.entries[0].aumid="Real.App!Game";
    json edited={{"id","0"},{"label","Пример"},{"nickname","Кот"},{"version","bad"},{"aumid","Real.App!Game"}};
    auto update=planWebProfileChange(original,"saveProfile",{{"profile",edited},{"isNew",false}},true);
    assert(original.entries[0].label=="Основной" && update.entries[0].label=="Пример");
    assert(update.entries[0].version=="1.1.5");
    auto added=planWebProfileChange(update,"saveProfile",{{"profile",edited},{"isNew",true}},true);
    assert(added.entries.size()==2 && added.selected==1);
    auto selected=planWebProfileChange(added,"selectProfile",{{"id","0"}},true); assert(selected.selected==0);
    auto deleted=planWebProfileChange(added,"deleteProfile",{{"id","0"}},true); assert(deleted.entries.size()==1 && deleted.selected==0);
    fails([&]{planWebProfileChange(original,"deleteProfile",{{"id","0"}},true);});
    fails([&]{planWebProfileChange(original,"selectProfile",{{"id","00"}},true);});
    fails([&]{planWebProfileChange(original,"selectProfile",{{"id","-1"}},true);});
    fails([&]{planWebProfileChange(original,"selectProfile",{{"id","0"}},false);});
    edited["aumid"]="Bad!Id!Oops"; fails([&]{planWebProfileChange(original,"saveProfile",{{"profile",edited},{"isNew",false}},true);});
    edited["aumid"]=""; edited["label"]=""; fails([&]{planWebProfileChange(original,"saveProfile",{{"profile",edited},{"isNew",true}},true);});
    assert(!validWebAppId("!foo") && !validWebAppId("foo!") && !validWebAppId("foo!bar!baz"));
    assert(validWebAppId("Microsoft.MinecraftUWP_8wekyb3d8bbwe!App"));
    assert(original.entries.size()==1 && original.entries[0].aumid=="Real.App!Game");
    std::cout << "Web profile planning and preservation: OK\n";
}
