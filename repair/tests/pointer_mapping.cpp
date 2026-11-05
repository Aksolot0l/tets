#include "../src/pointer_mapping.h"
#include <cassert>
#include <cmath>

int main() {
    float x = -1, y = -1, w = -1, h = -1;
    // A window positioned anywhere on the desktop is first converted by
    // ScreenToClient. This maps logical client pixels to the DXGI back buffer.
    assert(client_pointer(400, 300, 800, 600, x, y, w, h));
    assert(x * 1600 / w == 800 && y * 1200 / h == 600);
    assert(client_pointer(-5, 610, 800, 600, x, y, w, h));
    assert(x < 0 && y > h); // no false hover when outside the window
    assert(!client_pointer(0, 0, 0, 600, x, y, w, h));
    assert(!client_pointer(0, 0, 800, 0, x, y, w, h));
    assert(client_pointer(1920, 1080, 3840, 2160, x, y, w, h));
    assert(std::abs(x / w - 0.5f) < 0.0001f);
    assert(std::abs(y / h - 0.5f) < 0.0001f);
}
