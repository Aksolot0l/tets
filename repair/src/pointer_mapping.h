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

// CoreWindow bounds are in the same DIPs as PointerPosition. Bounds origin
// describes the client origin inside the system window (e.g. its title bar).
inline bool uwp_client_pointer(float pointerX, float pointerY, float left, float top,
                               float width, float height, float& x, float& y, float& w, float& h) {
    return client_pointer(pointerX - left, pointerY - top, width, height, x, y, w, h);
}
