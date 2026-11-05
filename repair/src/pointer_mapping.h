#pragma once

// Windows cursor and client rectangle must be expressed in the same coordinate
// system. ImGui's legacy bridge later scales these values to the back buffer.
// Do not clamp: an outside cursor must remain outside the overlay as well.
inline bool client_pointer(float cursorX, float cursorY, float clientWidth, float clientHeight,
                           float& x, float& y, float& w, float& h) {
    if (clientWidth <= 0.0f || clientHeight <= 0.0f) return false;
    x = cursorX; y = cursorY; w = clientWidth; h = clientHeight;
    return true;
}
