#pragma once
#include <cstdint>
#include <cstring>

// Windows wheel deltas can accumulate through 32-bit signed wraparound.
// Read the cumulative value without modifying the read-only mapping.
struct WheelCounter {
    std::uint32_t lastX = 0, lastY = 0;
    bool initialized = false;
    static int delta(std::uint32_t current, std::uint32_t last) {
        std::uint32_t modular = current - last;
        std::int32_t value = 0;
        std::memcpy(&value, &modular, sizeof(value));
        return value > 12000 ? 12000 : value < -12000 ? -12000 : value;
    }
    void reset() { initialized = false; }
    void take(std::uint32_t currentX, std::uint32_t currentY, int& x, int& y) {
        if (!initialized) { lastX = currentX; lastY = currentY; initialized = true; }
        x = delta(currentX, lastX); y = delta(currentY, lastY);
        lastX = currentX; lastY = currentY;
    }
};
