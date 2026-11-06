#pragma once
#include <cstdint>
#include <cstddef>
#include <cmath>
#include <limits>

struct Position115 { float x, y, z; };
struct BlockPosition115 { int x, y, z; };

// The reader validates ranges and finiteness before this is called. Block
// coordinates floor toward negative infinity, not toward zero. The one-block
// vertical correction is a display calibration based on player feedback; it
// does not change the raw values used for the independent Y consistency test.
inline BlockPosition115 displayPosition115(const Position115& position) {
    return {static_cast<int>(std::floor(position.x)),
            static_cast<int>(std::floor(position.y)) - 1,
            static_cast<int>(std::floor(position.z))};
}

// Experimental, read-only 1.1.5 paths. A separate height path is required
// for either candidate. No failed read may replace out or reuse old values.
template<class Read> bool readPosition115(std::uint64_t base, Read read, Position115& out) {
    auto pointer = [&](std::uint64_t address, std::uint64_t& value) {
        return address && read(address, &value, sizeof(value)) && value >= 0x10000 &&
               value < 0x0000800000000000ULL && (value & 7) == 0;
    };
    auto add = [](std::uint64_t a, std::uint64_t b, std::uint64_t& value) {
        if (a > std::numeric_limits<std::uint64_t>::max() - b) return false;
        value = a + b; return true;
    };
    auto chain = [&](std::uint64_t start, const std::uint64_t* offsets, std::size_t count, std::uint64_t& result) {
        std::uint64_t address = 0;
        if (!pointer(start, result)) return false;
        for (std::size_t i = 0; i < count; ++i)
            if (!add(result, offsets[i], address) || !pointer(address, result)) return false;
        return true;
    };
    constexpr std::uint64_t yPath[] = {0x30, 0x328, 0x0, 0x68, 0x90, 0x18};
    constexpr std::uint64_t playerPath[] = {0x30, 0x2c0, 0x0, 0x78, 0x28, 0x58};
    std::uint64_t sharedStart = 0, yRoot = 0, address = 0;
    if (!add(base, 0x01921DF8, sharedStart) ||
        !chain(sharedStart, yPath, sizeof(yPath)/sizeof(yPath[0]), yRoot)) return false;
    float separateY = 0;
    if (!add(yRoot, 0x164, address) || !read(address, &separateY, sizeof(separateY)) ||
        !std::isfinite(separateY) || std::fabs(separateY) > 4096.0f) return false;
    auto plausible = [&](const Position115& p) {
        return std::isfinite(p.x) && std::isfinite(p.y) && std::isfinite(p.z) &&
               std::fabs(p.x) <= 30000000.0f && std::fabs(p.z) <= 30000000.0f &&
               std::fabs(p.y) <= 4096.0f && std::fabs(p.y - separateY) <= 2.5f;
    };
    auto yStillValid = [&]() {
        std::uint64_t again = 0;
        return chain(sharedStart, yPath, sizeof(yPath)/sizeof(yPath[0]), again) && again == yRoot;
    };
    // C: alternative local player chain observed by the user's diagnostic.
    std::uint64_t player = 0, again = 0;
    Position115 candidate{};
    if (chain(sharedStart, playerPath, sizeof(playerPath)/sizeof(playerPath[0]), player) &&
        add(player, 0x94, address) && read(address, &candidate, sizeof(candidate)) && plausible(candidate) &&
        chain(sharedStart, playerPath, sizeof(playerPath)/sizeof(playerPath[0]), again) && again == player &&
        yStillValid()) {
        out = candidate;
        return true;
    }
    // A: retain the earlier candidate as a fallback for other matching builds.
    std::uint64_t rootAddress = 0, root = 0;
    if (!add(base, 0x019431E0, rootAddress) || !pointer(rootAddress, root) ||
        !add(root, 0x524, address) || !read(address, &candidate, sizeof(candidate)) || !plausible(candidate) ||
        !pointer(rootAddress, again) || again != root || !yStillValid()) return false;
    out = candidate;
    return true;
}
