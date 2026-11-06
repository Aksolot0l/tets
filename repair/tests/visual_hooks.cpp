#include "../src/visual_hooks.h"
#include <cassert>
#include <cstdint>
using namespace visual_hooks;
struct Fake {
 int failCreate=0,failEnable=0,failDisable=0,created=0,enabled=0,disabled=0,removed=0,active=0;
 static int bit(void* p) { return 1<<static_cast<int>(reinterpret_cast<std::uintptr_t>(p)-1); }
 int create(void* target,void*,void**) { int b=bit(target); if (failCreate&b) return 1; created|=b; return 0; }
 int enable(void* target) { int b=bit(target); if (failEnable&b) return 1; enabled|=b; active|=b; return 0; }
 int disable(void* target) { int b=bit(target); disabled|=b; if (failDisable&b) return 1; active&=~b; return 0; }
 int remove(void* target) { int b=bit(target); assert(!(active&b)); removed|=b; return 0; }
};
bool run(Fake& f) {
 void* tramp[3]{}; Hook hooks[3]{};
 for (int i=0;i<3;i++) hooks[i]={reinterpret_cast<void*>(static_cast<std::uintptr_t>(i+1)),reinterpret_cast<void*>(10),&tramp[i]};
 return install(hooks,3,[&](auto a,auto b,auto c){return f.create(a,b,c);},
   [&](auto a){return f.enable(a);},[&](auto a){return f.disable(a);},[&](auto a){return f.remove(a);});
}
int main() {
 Fake ok; assert(run(ok)); assert(ok.created==7 && ok.enabled==7 && ok.disabled==0 && ok.removed==0 && ok.active==7);
 Fake create; create.failCreate=4; assert(!run(create)); assert(create.created==3 && create.enabled==0 && create.removed==3);
 Fake partial; partial.failEnable=4; assert(!run(partial)); assert(partial.enabled==3 && partial.disabled==3 && partial.removed==7 && partial.active==0);
 Fake failure; failure.failEnable=4; failure.failDisable=2; assert(!run(failure));
 assert(failure.enabled==3 && failure.disabled==3 && failure.removed==5 && failure.active==2);
 // Seven D3D draw targets: a partial enable failure must disable/remove every
 // earlier active hook and remove every created but inactive hook.
 Fake gpu; gpu.failEnable=1<<5; void* tramp[7]{}; Hook hooks[7]{};
 for (int i=0;i<7;i++) hooks[i]={reinterpret_cast<void*>(static_cast<std::uintptr_t>(i+1)),reinterpret_cast<void*>(10),&tramp[i]};
 assert(!install(hooks,7,[&](auto a,auto b,auto c){return gpu.create(a,b,c);},
   [&](auto a){return gpu.enable(a);},[&](auto a){return gpu.disable(a);},[&](auto a){return gpu.remove(a);}));
 assert(gpu.created==127 && gpu.enabled==31 && gpu.disabled==31 && gpu.removed==127 && gpu.active==0);
}
