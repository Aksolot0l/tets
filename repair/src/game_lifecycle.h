#pragma once
#include <cstdint>

// The game-exit decision deliberately requires a signalled handle AND a
// successful process snapshot. A missing PID alone may mean a snapshot error,
// a start-up race, a second game or a reused PID, and must not close the app.
enum class GameExitAction { Wait, CloseLauncher, WatchReplacement };
inline GameExitAction gameExitAction(bool snapshotOk, bool multiple,
                                     std::uint32_t foundPid, std::uint32_t watchedPid,
                                     bool watchedProcessExited) {
    if (!snapshotOk || !watchedPid || !watchedProcessExited || multiple) return GameExitAction::Wait;
    if (!foundPid) return GameExitAction::CloseLauncher;
    if (foundPid != watchedPid) return GameExitAction::WatchReplacement;
    return GameExitAction::Wait;
}

// Fallback only when opening the observed process for SYNCHRONIZE failed.
// Require three consecutive complete, empty snapshots; failures and other
// Minecraft processes reset the streak. No game was ever observed => no exit.
struct UnwatchedGameExit {
    bool observed = false;
    unsigned emptySnapshots = 0;
    void markObserved() { observed = true; emptySnapshots = 0; }
    void reset() { observed = false; emptySnapshots = 0; }
    bool update(bool snapshotOk, bool multiple, std::uint32_t foundPid) {
        if (!observed) return false;
        if (!snapshotOk || multiple || foundPid) { emptySnapshots = 0; return false; }
        return ++emptySnapshots >= 3;
    }
};
