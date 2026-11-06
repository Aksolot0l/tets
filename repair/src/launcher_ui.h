#pragma once
#include <windows.h>

enum class LauncherStatus : int { Waiting, GameDetected, Loading, Loaded, Failed, MultipleGames };
void launcherSetStatus(LauncherStatus status);
// The classic launcher remains in the same process and is only hidden once
// the new window is ready. On failure it is left visible.
bool launcherStartUi();

// Request a graceful window close, then terminate only this launcher process if it remains alive.
void launcherRequestShutdown();
