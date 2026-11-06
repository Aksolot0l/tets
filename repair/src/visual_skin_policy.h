#pragma once
#include <cstdint>
// A filter, NOT proof that a texture is a skin. Candidate views are gathered
// only during the local player's own renderer and require user verification.
namespace visual_skin_policy {
inline bool dimensions(std::uint32_t width,std::uint32_t height,std::uint32_t samples,std::uint32_t arraySize) {
 if (samples!=1 || arraySize!=1 || width<64 || width>512 || (width&(width-1))!=0) return false;
 return height==width || height==width/2u;
}
inline float vScale(std::uint32_t width,std::uint32_t height) { return height ? static_cast<float>(width)/static_cast<float>(height) : 1.f; }
}
