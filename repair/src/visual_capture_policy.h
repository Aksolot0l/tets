#pragma once
#include <cstdint>
namespace visual_capture_policy {
// Bound GPU memory usage and reject array/mip layouts that cannot be safely
// copied into the single-sample preview resource.
inline bool supported(std::uint32_t width,std::uint32_t height,std::uint32_t arraySize,std::uint32_t mips) {
 return width && height && static_cast<std::uint64_t>(width)*height<=16000000ULL && arraySize==1 && mips==1;
}
}
