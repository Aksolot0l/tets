#include "../src/mouse_diagnostic_patch.h"
#include <cassert>
#include <vector>
#include <cstring>
int main() {
    using namespace mouse_diagnostic_patch;
    std::vector<unsigned char> image(formatRva + sizeof(format) + 10, 0);
    std::memcpy(image.data()+codeRva, code, sizeof(code));
    std::memcpy(image.data()+formatRva, format, sizeof(format));
    assert(matches(image.data(),image.size()));
    assert(!matches(image.data(), codeRva));
    image[formatRva] ^= 1; assert(!matches(image.data(),image.size())); image[formatRva] ^= 1;
    image[codeRva+1] ^= 1; assert(!matches(image.data(),image.size())); image[codeRva+1] ^= 1;
    auto before = image;
    apply(image.data());
    for (std::size_t i = 0; i < image.size(); ++i)
        assert(image[i] == (i >= branchRva && i < branchRva+6 ? 0x90 : before[i]));
    assert(!matches(image.data(),image.size()));
}
