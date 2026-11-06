#include "../src/china_hat_hash.h"
#include <cassert>
#include <cstdint>
#include <stdexcept>
#include <vector>

struct FakeApi {
    using Algorithm=void*; using Hash=void*;
    std::uint8_t* retained=nullptr;
    bool closed=false, destroyed=false, failData=false, failFinish=false, throwData=false;
    int chunks=0;
    bool openAlgorithm(Algorithm& a) { a=this; return true; }
    bool objectLength(Algorithm,std::uint32_t& n) { n=64; return true; }
    bool hashLength(Algorithm,std::uint32_t& n) { n=32; return true; }
    bool createHash(Algorithm,Hash& h,std::uint8_t* b,std::uint32_t n) {
        assert(n==64); retained=b; retained[0]=42; h=this; return true;
    }
    bool hashData(Hash,const std::uint8_t*,std::uint32_t n) {
        assert(n>0 && n<=1024*1024); ++chunks;
        if (throwData) throw std::runtime_error("mock");
        return !failData;
    }
    bool finishHash(Hash,std::uint8_t*,std::uint32_t n) { assert(n==32); return !failFinish; }
    void destroyHash(Hash) {
        // Read and write caller-owned storage in the equivalent of
        // BCryptDestroyHash. ASan detects if it has already been freed.
        assert(!closed && retained && retained[0]==42);
        retained[0]=43; destroyed=true;
    }
    void closeAlgorithm(Algorithm) { assert(destroyed); closed=true; }
};
int main() {
    std::vector<std::uint8_t> bytes(2*1024*1024+1,0); std::uint8_t digest[32]{};
    FakeApi good; assert(hashHatExecutable(good,bytes.data(),bytes.size(),digest));
    assert(good.chunks==3 && good.destroyed && good.closed);
    FakeApi bad; bad.failData=true;
    assert(!hashHatExecutable(bad,bytes.data(),bytes.size(),digest));
    assert(bad.destroyed && bad.closed);
    FakeApi finish; finish.failFinish=true;
    assert(!hashHatExecutable(finish,bytes.data(),bytes.size(),digest));
    assert(finish.destroyed && finish.closed);
    FakeApi throwing; throwing.throwData=true;
    try { hashHatExecutable(throwing,bytes.data(),bytes.size(),digest); assert(false); }
    catch (const std::runtime_error&) {}
    assert(throwing.destroyed && throwing.closed);
}
