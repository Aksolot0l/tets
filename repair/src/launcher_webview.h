#pragma once
#include <windows.h>
#include <string>

// All methods run on the launcher UI's STA thread. The legacy Win32 controls
// remain available if the WebView runtime or initialization fails.
using WebDispatch = std::string (*)(const std::string&);
using WebReady = void (*)();
bool startLauncherWebView(HWND parent, WebDispatch dispatch, WebReady ready, WebReady failure);
void resizeLauncherWebView();
void updateLauncherWebViewStatus(const char* status);
void tickLauncherWebView();
void closeLauncherWebView();
