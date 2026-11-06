#include "../src/disable_hotkey.h"
#include <cassert>
int main() {
    DisableHotkey k;
    assert(!k.update(false, true, false));
    assert(k.update(true, true, false));
    assert(!k.update(true, true, false)); // held key
    assert(!k.update(false, true, false));
    assert(!k.update(true, false, false)); // background
    assert(!k.update(true, true, false)); // held across focus change
    assert(!k.update(false, true, false));
    assert(!k.update(true, true, true)); // text editor
    assert(!k.update(true, true, false)); // held after editing
    assert(!k.update(false, true, false));
    assert(k.update(true, true, false));
}
