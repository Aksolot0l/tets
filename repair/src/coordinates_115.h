#pragma once
#include <cstdint>
#include <cmath>
#include <limits>

struct Position115 { float x, y, z; };

// Experimental read-only paths published for Minecraft Windows 10 1.1.5.
// Read is a callable bool(uint64_t address, void* out, size_t bytes). Null,
// torn, unmapped, non-finite, or inconsistent paths are rejected. In
// particular, no fallback to zero or the last known value is permitted.
template<class Read> bool readPosition115(std::uint64_t base, Read read, Position115& out) {
    auto pointer = [&](std::uint64_t address, std::uint64_t& value) {
        return address && read(address, &value, sizeof(value)) && value >= 0x10000 &&
               value < 0x0000800000000000ULL && (value & 7) == 0;
    };
    auto add = [](std::uint64_t a, std::uint64_t b, std::uint64_t& value) {
        if (a > std::numeric_limits<std::uint64_t>::max() - b) return false;
        value = a + b; return true;
    };
    std::uint64_t a = 0, root = 0, rootAgain = 0, yRoot = 0;
    if (!add(base, 0x019431E0, a) || !pointer(a, root)) return false;
    Position115 candidate{};
    if (!add(root, 0x524, a) || !read(a, &candidate, sizeof(candidate))) return false;
    // Independent pointer chain to height, used only as a consistency check.
    if (!add(base, 0x01921DF8, a) || !pointer(a, yRoot)) return false;
    constexpr std::uint64_t path[] = {0x30, 0x328, 0x0, 0x68, 0x90, 0x18};
    for (auto offset : path) {
        if (!add(yRoot, offset, a) || !pointer(a, yRoot)) return false;
    }
    float separateY = 0;
    if (!add(yRoot, 0x164, a) || !read(a, &separateY, sizeof(separateY))) return false;
    if (!add(base, 0x019431E0, a) || !pointer(a, rootAgain) || rootAgain != root) return false;
    if (!std::isfinite(candidate.x) || !std::isfinite(candidate.y) || !std::isfinite(candidate.z) ||
        !std::isfinite(separateY) || std::fabs(candidate.x) > 30000000.0f ||
        std::fabs(candidate.z) > 30000000.0f || std::fabs(candidate.y) > 4096.0f ||
        std::fabs(candidate.y - separateY) > 2.5f) return false;
    out = candidate;
    return true;
}
