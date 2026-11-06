#pragma once

// The fallback controls are only shown on an actual WebView failure. Late
// asynchronous ready callbacks must not override an already displayed fallback.
enum class LauncherView { Loading, Web, Fallback };
class LauncherStartupView {
    LauncherView value_ = LauncherView::Loading;
public:
    LauncherView value() const { return value_; }
    bool ready() {
        if (value_ != LauncherView::Loading) return false;
        value_ = LauncherView::Web;
        return true;
    }
    void failed() { value_ = LauncherView::Fallback; }
};
