#pragma once
#include <cstddef>
#include <cstdint>
#include <cstring>

// Exact legacy core only. The JNE at 0x183d branches into the mouse test
// background and text. Six NOPs fall through to ImGui::Render at 0x1843.
// The checks deliberately fail closed for a different core or existing patch.
namespace mouse_diagnostic_patch {
constexpr std::size_t codeRva = 0x1836;
constexpr std::size_t branchRva = 0x183d;
constexpr std::size_t formatRva = 0x1bb008;
constexpr unsigned char code[] = {0xe8,0xf5,0x17,0x00,0x00,0x84,0xc0,0x0f,0x85,0x18,0x02,0x00,0x00,0xe8,0xb8,0x67,0x05,0x00};
constexpr char format[] = "UWP mouse test: reg %u/3  events %u  poll %u/3 %s";
inline bool matches(const unsigned char* image, std::size_t size) {
    return image && size >= formatRva + sizeof(format) && size >= codeRva + sizeof(code) &&
        std::memcmp(image + codeRva, code, sizeof(code)) == 0 &&
        std::memcmp(image + formatRva, format, sizeof(format)) == 0;
}
inline void apply(unsigned char* image) { std::memset(image + branchRva, 0x90, 6); }
}
