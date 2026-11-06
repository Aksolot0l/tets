#pragma once
#include "launcher_profiles.h"
#include <stdexcept>
#include <string>
#include <utility>

// Pure validation/planning stage; callers commit the returned copy only after
// saveLauncherProfiles succeeds. No browser input can supply file paths.
inline bool webProfileIndex(const LauncherProfiles& profiles, const nlohmann::json& args, size_t& index) {
    if (!args.is_object() || !args.contains("id") || !args["id"].is_string()) return false;
    const auto value=args["id"].get<std::string>();
    if (value.empty() || value.size()>3 || (value.size()>1 && value[0]=='0')) return false;
    index=0;
    for (char c:value) { if (c<'0'||c>'9') return false; index=index*10+static_cast<size_t>(c-'0'); }
    return index<profiles.entries.size();
}
inline bool validWebAppId(const std::string& s) {
    if (s.empty() || s.size()>512) return false;
    unsigned bangs=0;
    for (unsigned char c:s) {
        if (c=='!') { ++bangs; continue; }
        if (!((c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9')||c=='.'||c=='_'||c=='-')) return false;
    }
    return bangs==1 && s.front()!='!' && s.back()!='!';
}
inline LauncherProfiles planWebProfileChange(const LauncherProfiles& original, const std::string& method,
                                               const nlohmann::json& args, bool writable) {
    if (!writable) throw std::runtime_error("Сохранение отключено: файл профилей недоступен или повреждён.");
    if (!args.is_object() || original.entries.empty() || original.selected>=original.entries.size()) throw std::runtime_error("Некорректные данные профиля.");
    auto next=original;
    if (method=="selectProfile") {
        size_t index=0; if (!webProfileIndex(original,args,index)) throw std::runtime_error("Профиль не найден. Обновите список.");
        next.selected=index;
    } else if (method=="deleteProfile") {
        size_t index=0;
        if (!webProfileIndex(original,args,index)) throw std::runtime_error("Профиль не найден. Обновите список.");
        if (next.entries.size()<=1) throw std::runtime_error("Нельзя удалить последний профиль.");
        next.entries.erase(next.entries.begin()+index);
        if (next.selected==index) next.selected=0; else if (next.selected>index) --next.selected;
    } else if (method=="saveProfile") {
        if (!args.contains("profile") || !args["profile"].is_object() || !args.contains("isNew") || !args["isNew"].is_boolean()) throw std::runtime_error("Некорректный профиль.");
        const auto& profile=args["profile"];
        LauncherProfile p{stringField(profile,"label"),stringField(profile,"nickname"),stringField(profile,"version"),stringField(profile,"aumid")};
        if (!validProfile(p) || (!p.aumid.empty() && !validWebAppId(p.aumid))) throw std::runtime_error("Проверьте название, длину полей и App ID профиля.");
        if (args["isNew"].get<bool>()) {
            if (next.entries.size()>=200) throw std::runtime_error("Максимум 200 профилей.");
            next.entries.push_back(std::move(p)); next.selected=next.entries.size()-1;
        } else {
            size_t index=0; if (!webProfileIndex(original,profile,index)) throw std::runtime_error("Профиль не найден. Обновите список.");
            p.version=next.entries[index].version; next.entries[index]=std::move(p); next.selected=index;
        }
    } else throw std::runtime_error("Неизвестная команда профиля.");
    return next;
}
