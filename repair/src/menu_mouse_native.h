#pragma once
#include "china_hat_native.h"
// Optional, exact-build-only input test. Failure preserves the old behavior.
bool InitializeMenuMouse(HatCreateHook, HatToggleHook, HatToggleHook, HatToggleHook);
void SetMenuMouseOpen(bool);
int MenuMouseStatus();
