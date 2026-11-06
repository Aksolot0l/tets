#include "../src/launcher_profiles.h"
#include <cassert>
int main() {
    LauncherProfiles result=defaultLauncherProfiles();
    const std::string legacy=R"({"schema":1,"selected_profile":7,"aumid":"Package_abc!App","profiles":[{"id":3,"label":"Основной","nickname":"Один","version":"1.1.5"},{"id":7,"label":"Второй","nickname":"Два","version":"1.1.5"}],"registry_backup":{"arbitrary":"unchanged"}})";
    assert(readProfiles(legacy,true,result));
    assert(result.entries.size()==2 && result.selected==1);
    assert(result.entries[1].nickname=="Два" && result.entries[1].aumid=="Package_abc!App");
    auto bytes=writeProfiles(result); LauncherProfiles again;
    assert(readProfiles(bytes,false,again));
    assert(again.entries.size()==2 && again.selected==1 && again.entries[0].label=="Основной");
    auto before=again.entries[0].label;
    assert(!readProfiles("{corrupted",false,again) && again.entries[0].label==before);
    assert(!readProfiles(R"({"schema":42,"profiles":[]})",false,again));
    assert(!readProfiles(R"({"schema":1,"selected":10,"profiles":[{"label":"a"}]})",false,again));
    assert(!readProfiles(R"({"schema":1,"profiles":[{"label":""}]})",false,again));
    assert(readProfiles(R"({"profiles":[],"aumid":null})",true,again));
    assert(again.entries.size()==1 && again.selected==0);
    assert(readProfiles(R"({"profiles":[],"aumid":"Package_abc!App"})",true,again));
    assert(again.entries[0].aumid=="Package_abc!App");
    LauncherProfile tooLong{std::string(257,'a'),"","",""};
    assert(!validProfile(tooLong));
}
