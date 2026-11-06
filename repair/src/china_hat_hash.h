#pragma once
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <vector>

// BCrypt retains the caller-owned hash object buffer until the hash handle is
// destroyed. Keep storage alive longer than both handles, including on errors
// and exceptions. Api is injectable so this ordering can be tested portably.
template <class Api>
bool hashHatExecutable(Api& api, const std::uint8_t* bytes, std::size_t size, std::uint8_t result[32]) {
    std::vector<std::uint8_t> storage;
    typename Api::Algorithm algorithm{};
    typename Api::Hash hash{};
    struct Cleanup {
        Api& api;
        typename Api::Algorithm& algorithm;
        typename Api::Hash& hash;
        ~Cleanup() {
            if (hash) api.destroyHash(hash);
            if (algorithm) api.closeAlgorithm(algorithm);
        }
    } cleanup{api, algorithm, hash};
    if (!api.openAlgorithm(algorithm)) return false;
    std::uint32_t objectSize=0, hashSize=0;
    if (!api.objectLength(algorithm,objectSize) || objectSize==0 || objectSize>=65536 ||
        !api.hashLength(algorithm,hashSize) || hashSize!=32) return false;
    storage.resize(objectSize);
    if (!api.createHash(algorithm,hash,storage.data(),objectSize)) return false;
    for (std::size_t pos=0;pos<size;) {
        auto chunk=static_cast<std::uint32_t>(std::min<std::size_t>(size-pos,1024*1024));
        if (!api.hashData(hash,bytes+pos,chunk)) return false;
        pos+=chunk;
    }
    return api.finishHash(hash,result,32);
}
