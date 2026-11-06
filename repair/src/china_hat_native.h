#pragma once
#include <windows.h>
#include "visual_settings.h"
struct ID3D11DeviceContext;
// Opt-in read-only GPU skin candidate discovery for the effect editor.
void SetPlayerSkinPreviewActive(bool enabled);
// Experimental GPU draw replay, never enabled unless the user selects it.
void SetPlayerModelPreviewActive(bool enabled);
void SetPlayerPreviewHatActive(bool enabled);
void ObserveVisualContext(ID3D11DeviceContext* context);
int VisualDepthBefore();
int VisualDepthAfter();
int VisualDepthWorld();
int VisualOcclusionStatus();
// Optional player-pass timing test; never bypasses depth or blends.
bool HatPlayerPass();
void SetHatPlayerPass(bool enabled);
int HatPlayerPassStatus();
int HatPlayerDepthCompare();
int WorldDepthCompare();
bool VisualLegacyMode();
void SetVisualLegacyMode(bool enabled);

using HatCreateHook = int (WINAPI*)(void*, void*, void**);
using HatToggleHook = int (WINAPI*)(void*);
// Verifies the exact user-observed game binary. Hooks and rendering are only
// installed on opt-in. Failure leaves the normal client working.
bool InitializeChinaHat(HatCreateHook create, HatToggleHook enable,
                        HatToggleHook disable, HatToggleHook remove);
bool ChinaHatAvailable();
bool ChinaHatEnabled();
void SetChinaHatEnabled(bool enabled);

// Visuals are independent switches and use the same validated game hooks.
bool HatFillEnabled();
void SetHatFillEnabled(bool enabled);
bool TrailFillEnabled();
void SetTrailFillEnabled(bool enabled);
bool VisualTintEnabled();
void SetVisualTintEnabled(bool enabled);
// First-person model replacement experiment. Off by default; runtime unverified.
bool KatanaEnabled();
void SetKatanaEnabled(bool enabled);
int KatanaStatus();
// Separate built-in, opt-in test accessory, not an external model loader.
bool AccessoryEnabled();
void SetAccessoryEnabled(bool enabled);
int AccessoryStatus();
bool JumpEnabled();
void SetJumpEnabled(bool enabled);
bool JumpRainbowEnabled();
void SetJumpRainbowEnabled(bool enabled);
int JumpStatus();
bool TrailEnabled();
void SetTrailEnabled(bool enabled);
float ChinaHatHeight();
void SetChinaHatHeight(float value);
int ChinaHatStatus();
int TrailStatus();

float GetVisualParam(VisualParam param);
void SetVisualParam(VisualParam param,float value);
bool HatRainbowEnabled();
void SetHatRainbowEnabled(bool enabled);
bool TrailRainbowEnabled();
void SetTrailRainbowEnabled(bool enabled);
bool VertexGradientEnabled();
void SetVertexGradientEnabled(bool enabled);
int HatFillStatus();
int TrailFillStatus();
