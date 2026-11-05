#pragma once
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <cstdint>

// Pagefile-backed mapping shared by duplicating a read-only handle into the
// game. Never a named object, never a disk file. Aligned 32-bit counter reads
// are atomic on the x64 Windows targets supported by this project.
struct WheelBridge {
    std::uint32_t magic;
    std::uint32_t version;
    volatile LONG horizontal;
    volatile LONG vertical;
};
constexpr std::uint32_t kWheelMagic = 0x47574831;
static_assert(sizeof(WheelBridge) == 16, "wheel bridge ABI");
