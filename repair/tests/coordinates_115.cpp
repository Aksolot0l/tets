#include "../src/coordinates_115.h"
#include <cassert>
#include <cstring>
#include <unordered_map>

struct Memory {
    std::unordered_map<std::uint64_t, unsigned char> bytes;
    template<class T> void put(std::uint64_t address, const T& value) {
        const auto* p = reinterpret_cast<const unsigned char*>(&value);
        for (size_t i = 0; i < sizeof(value); ++i) bytes[address + i] = p[i];
    }
    bool read(std::uint64_t address, void* out, size_t length) const {
        auto* p = static_cast<unsigned char*>(out);
        for (size_t i = 0; i < length; ++i) {
            auto at = bytes.find(address + i);
            if (at == bytes.end()) return false;
            p[i] = at->second;
        }
        return true;
    }
};
int main() {
    constexpr std::uint64_t base = 0x140000000ULL, start = 0x300000, first = 0x301000;
    constexpr std::uint64_t aRoot = 0x200000;
    Position115 expected{-123.25f, 64.0f, 101.5f}, unchanged{1,2,3};
    Memory original;
    original.put(base + 0x19431e0, std::uint64_t{0x200003}); // A is unaligned, as in the diagnostic
    original.put(base + 0x1921df8, start);
    original.put(start + 0x30, first);
    std::uint64_t yRoot = first;
    for (auto offset : {0x328ULL,0x0ULL,0x68ULL,0x90ULL,0x18ULL}) {
        auto next = yRoot + 0x1000;
        original.put(yRoot + offset, next); yRoot = next;
    }
    original.put(yRoot + 0x164, 65.6f);
    std::uint64_t player = first, lastPlayerPointerAddress = 0;
    for (auto offset : {0x2c0ULL,0x0ULL,0x78ULL,0x28ULL,0x58ULL}) {
        auto next = player + 0x10000;
        lastPlayerPointerAddress = player + offset;
        original.put(lastPlayerPointerAddress, next); player = next;
    }
    original.put(player + 0x94, expected);
    auto check = [&](Memory& m, bool success) {
        auto read = [&](std::uint64_t a, void* p, size_t n) { return m.read(a,p,n); };
        auto result = unchanged;
        assert(readPosition115(base, read, result) == success);
        if (success) assert(result.x == expected.x && result.y == expected.y && result.z == expected.z);
        else assert(result.x == unchanged.x && result.y == unchanged.y && result.z == unchanged.z);
    };
    check(original, true);
    { auto m = original; m.put(yRoot + 0x164, 100.0f); check(m, false); }
    { auto m = original; m.put(yRoot + 0x164, NAN); check(m, false); }
    { auto m = original; m.put(player + 0x94, Position115{NAN, 64.0f, 10.0f}); check(m, false); }
    { auto m = original; m.put(player + 0x94, Position115{30000002.0f, 64.0f, 10.0f}); check(m, false); }
    { auto m = original; m.put(lastPlayerPointerAddress, std::uint64_t{0}); check(m, false); }
    { auto m = original; m.bytes.erase(player + 0x94); check(m, false); }
    { auto m = original; m.put(base + 0x1921df8, std::uint64_t{0}); check(m, false); }
    // Legacy A still works only if its independent height check works.
    { auto m = original; m.put(lastPlayerPointerAddress, std::uint64_t{0});
      m.put(base + 0x19431e0, aRoot); m.put(aRoot + 0x524, expected); check(m, true);
      m.put(yRoot + 0x164, 100.0f); check(m, false); }
    // The object changing while sampling must not be published.
    int lastReads = 0;
    auto changing = [&](std::uint64_t a, void* p, size_t n) {
        if (a == lastPlayerPointerAddress && ++lastReads == 2) {
            std::uint64_t different = player + 0x1000; std::memcpy(p, &different, sizeof(different)); return true;
        }
        return original.read(a,p,n);
    };
    auto result = unchanged;
    assert(!readPosition115(base, changing, result));
    assert(result.x == unchanged.x && result.y == unchanged.y && result.z == unchanged.z);
    assert(!readPosition115(UINT64_MAX - 2, changing, result));
}
