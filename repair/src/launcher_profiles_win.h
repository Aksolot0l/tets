#pragma once
#include "launcher_profiles.h"
#include <string>

std::wstring toWide(const std::string& value);
std::string toUtf8(const std::wstring& value);
// Imports the old store only when the new store is absent; never writes it.
// False means an existing store is unreadable/corrupt; saving is disabled.
bool loadLauncherProfiles(LauncherProfiles& profiles);
bool saveLauncherProfiles(const LauncherProfiles& profiles);
// Returns the AUMID only if precisely one Minecraft.Windows.exe is running.
std::wstring detectMinecraftAppId();
bool validAppId(const std::wstring& id);
