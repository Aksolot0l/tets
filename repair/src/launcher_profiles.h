#pragma once
#include "../vendor/json/json.hpp"
#include <algorithm>
#include <string>
#include <vector>

// Profile names and nicknames are local notes. They do not alter authentication
// or the player's identity. App IDs are the only field used for activation.
struct LauncherProfile { std::string label, nickname, version, aumid; };
struct LauncherProfiles { std::vector<LauncherProfile> entries; std::size_t selected = 0; };
inline LauncherProfiles defaultLauncherProfiles() { return {{{"Основной", "", "1.1.5", ""}}, 0}; }
inline std::string stringField(const nlohmann::json& v, const char* key) {
    auto it=v.find(key); return it!=v.end() && it->is_string() ? it->get<std::string>() : std::string();
}
inline bool validProfile(const LauncherProfile& p) {
    return !p.label.empty() && p.label.size()<=256 && p.nickname.size()<=256 && p.version.size()<=128 && p.aumid.size()<=512;
}
inline bool readProfiles(const std::string& data, bool legacy, LauncherProfiles& out) {
    try {
        auto root=nlohmann::json::parse(data);
        if (!root.is_object()) return false;
        if (!legacy && (!root.contains("schema") || root["schema"]!=1)) return false;
        auto entries=root.find("profiles");
        if (entries==root.end() || !entries->is_array() || entries->size()>200) return false;
        LauncherProfiles result; auto globalId=stringField(root,"aumid");
        std::string selectedId;
        if (legacy && root.contains("selected_profile") && !root["selected_profile"].is_null()) selectedId=root["selected_profile"].dump();
        for (const auto& item:*entries) {
            if (!item.is_object()) return false;
            LauncherProfile p{stringField(item,"label"),stringField(item,"nickname"),stringField(item,"version"),stringField(item,"aumid")};
            if (legacy && p.aumid.empty()) p.aumid=globalId;
            if (!validProfile(p)) return false;
            if (legacy && item.contains("id") && item["id"].dump()==selectedId) result.selected=result.entries.size();
            result.entries.push_back(std::move(p));
        }
        if (result.entries.empty()) {
            result=defaultLauncherProfiles();
            if (legacy && globalId.size()<=512) result.entries[0].aumid=globalId;
        }
        if (!legacy && root.contains("selected")) {
            if (!root["selected"].is_number_unsigned()) return false;
            auto index=root["selected"].get<std::size_t>(); if (index>=result.entries.size()) return false;
            result.selected=index;
        }
        out=std::move(result); return true;
    } catch (...) { return false; }
}
inline std::string writeProfiles(const LauncherProfiles& profiles) {
    nlohmann::json root={{"schema",1},{"selected",profiles.selected},{"profiles",nlohmann::json::array()}};
    for (const auto& p:profiles.entries) root["profiles"].push_back({{"label",p.label},{"nickname",p.nickname},{"version",p.version},{"aumid",p.aumid}});
    return root.dump(2);
}
