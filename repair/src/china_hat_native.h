#pragma once
#include <windows.h>

using HatCreateHook = int (WINAPI*)(void*, void*, void**);
using HatToggleHook = int (WINAPI*)(void*);
// Verifies the exact user-observed game binary. Hooks and rendering are only
// installed on opt-in. Failure leaves the normal client working.
bool InitializeChinaHat(HatCreateHook create, HatToggleHook enable,
                        HatToggleHook disable, HatToggleHook remove);
bool ChinaHatAvailable();
bool ChinaHatEnabled();
void SetChinaHatEnabled(bool enabled);
