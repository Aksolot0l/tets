#pragma once
// Disable only on a fresh Delete press, in the active game, never while an
// ImGui text editor is accepting input. Ignore holds and background presses.
class DisableHotkey {
    bool wasDown = false;
public:
    bool update(bool down, bool gameForeground, bool editingText) {
        bool pressed = down && !wasDown && gameForeground && !editingText;
        wasDown = down;
        return pressed;
    }
};
