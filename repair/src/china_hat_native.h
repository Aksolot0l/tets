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
std::uint32_t KatanaEvidence();
// Read-only diagnostics snapshot for the katana test page. All fields are
// informational; none of them feed rendering or suppression decisions.
struct KatanaDiagnostics {
 bool valid=false, agree=false, wrapped=false;
 float yaw=0.f, pitchUp=0.f;
 unsigned sources=0;                       // 1 player, 2 global A, 4 global B, 8 client +0x58
 unsigned long long clientInstance=0, player=0, rotationHolder=0;
 unsigned long long rootA=0, interA=0, holderA=0, rootB=0, interB=0, holderB=0;
 float playerAngles[2]={}, holderAngles[2]={}, holderAAngles[2]={}, holderBAngles[2]={};
 float playerHead=0;
 bool playerAnglesRead=false, holderAnglesRead=false, holderAAnglesRead=false,
      holderBAnglesRead=false, playerHeadRead=false;
 int tpCallsInPass=0, tpCallsOutOfPass=0;
 unsigned long long tpLastTexture=0, learnedTexture=0;
};
KatanaDiagnostics KatanaDebugSnapshot();
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
