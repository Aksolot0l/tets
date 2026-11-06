#include "../src/page_scroll.h"
#include <cassert>
int main() {
    PageScrollRepeat state;
    assert(state.step(false, true, false, 0) == 0);
    assert(state.step(true, false, true, 10) == -1);
    assert(state.step(true, false, true, 409) == 0);
    assert(state.step(true, false, true, 410) == -1);
    assert(state.step(true, false, true, 519) == 0);
    assert(state.step(true, false, true, 520) == -1);
    assert(state.step(true, true, false, 521) == 1);
    assert(state.step(true, true, true, 522) == 0);
    assert(state.step(true, true, false, 523) == 1);
    assert(state.step(false, true, false, 524) == 0);
    assert(state.step(true, true, false, 525) == 1);
}
