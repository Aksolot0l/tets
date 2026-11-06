#include "../src/launcher_startup_view.h"
#include <cassert>
int main() {
    LauncherStartupView success;
    assert(success.value() == LauncherView::Loading);
    assert(success.ready() && success.value() == LauncherView::Web);
    assert(!success.ready());
    success.failed(); // WebView process may fail after it was ready.
    assert(success.value() == LauncherView::Fallback);
    assert(!success.ready() && success.value() == LauncherView::Fallback);
    LauncherStartupView failure;
    failure.failed(); // missing loader or timeout before readiness
    assert(failure.value() == LauncherView::Fallback);
    assert(!failure.ready() && failure.value() == LauncherView::Fallback);
}
