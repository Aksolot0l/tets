#pragma once
#include <cstdint>

// Simple key-repeat fallback when UWP doesn't deliver mouse-wheel events.
// Returns an ImGui vertical wheel value, never queues scroll with no menu.
struct PageScrollRepeat {
    int direction = 0;
    std::uint64_t next = 0;
    float step(bool enabled, bool pageUp, bool pageDown, std::uint64_t nowMs) {
        const int requested = enabled ? (pageUp ? 1 : 0) - (pageDown ? 1 : 0) : 0;
        if (!requested) { direction = 0; next = 0; return 0.0f; }
        if (requested != direction) {
            direction = requested; next = nowMs + 400;
            return static_cast<float>(requested);
        }
        if (nowMs >= next) {
            next = nowMs + 110;
            return static_cast<float>(requested);
        }
        return 0.0f;
    }
};
