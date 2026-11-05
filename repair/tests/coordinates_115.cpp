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
    constexpr std::uint64_t base = 0x140000000ULL, root = 0x200000, start = 0x300000;
    Memory m;
    m.put(base + 0x19431e0, root);
    m.put(base + 0x1921df8, start);
    Position115 expected{-123.25f, 64.0f, 101.5f};
    m.put(root + 0x524, expected);
    std::uint64_t current = start;
    for (auto offset : {0x30ULL,0x328ULL,0x0ULL,0x68ULL,0x90ULL,0x18ULL}) {
        auto next = current + 0x1000;
        m.put(current + offset, next);
        current = next;
    }
    m.put(current + 0x164, 65.6f);
    auto read = [&](std::uint64_t a, void* p, size_t n) { return m.read(a,p,n); };
    Position115 got{1,2,3};
    assert(readPosition115(base, read, got));
    assert(got.x == expected.x && got.y == expected.y && got.z == expected.z);
    m.put(current + 0x164, 100.0f);
    got = {1,2,3}; assert(!readPosition115(base, read, got));
    assert(got.x == 1 && got.y == 2 && got.z == 3); // no fabricated value
    m.put(current + 0x164, 64.0f);
    m.put(root + 0x524, Position115{NAN, 64.0f, 10.0f});
    assert(!readPosition115(base, read, got));
    m.put(base + 0x19431e0, std::uint64_t{0});
    assert(!readPosition115(base, read, got));
}
