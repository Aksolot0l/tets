// x64 1.1.5 input research adapter. Observed from the user's exact executable,
// not an imported x86 layout. Game hooks stay dormant if identity or byte
// signatures differ. The actual game behavior still needs Windows testing.
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <atomic>
#include <cstdint>
#include <cstring>
#include "menu_mouse_native.h"
#include "menu_mouse_policy.h"
#include "visual_hooks.h"

namespace {
constexpr std::uintptr_t queueRva=0x6f0f30, tickRva=0x7d37f0, factoryRva=0x7d6370;
constexpr std::uintptr_t mouseVtableRva=0x1738e28;
struct Signature { std::uintptr_t rva; unsigned char bytes[16]; };
constexpr Signature signatures[]={
 {queueRva,{0x48,0x89,0x54,0x24,0x10,0x57,0x48,0x83,0xec,0x30,0x48,0xc7,0x44,0x24,0x20,0xfe}},
 {tickRva,{0x48,0x8b,0xc4,0x55,0x57,0x41,0x54,0x41,0x56,0x41,0x57,0x48,0x8b,0xec,0x48,0x83}},
 {factoryRva,{0x48,0x89,0x5c,0x24,0x10,0x48,0x89,0x74,0x24,0x18,0x48,0x89,0x7c,0x24,0x20,0x41}},
 {0x7d3b72,{0x8b,0x43,0x74,0x83,0xf8,0x01,0x75,0x10,0x80,0x7b,0x72,0x00,0x75,0x0a,0x48,0x8b}},
 {0x7d4590,{0x40,0x53,0x56,0x57,0x41,0x56,0x48,0x83,0xec,0x38,0x48,0xc7,0x44,0x24,0x20,0xfe}},
 {0x7d4850,{0x40,0x53,0x56,0x57,0x41,0x56,0x48,0x83,0xec,0x38,0x48,0xc7,0x44,0x24,0x20,0xfe}}
};
using QueueFn=void (*)(void*,void**);
using TickFn=std::uintptr_t (*)(void*);
using FactoryFn=void** (*)(void**,const unsigned char*,const unsigned char*,const std::uint32_t*,const std::int16_t*,const std::int16_t*);
using DeleteFn=void (*)(void*,unsigned);
QueueFn realQueue=nullptr; TickFn realTick=nullptr; FactoryFn factory=nullptr;
std::uintptr_t image=0;
std::atomic<bool> menuOpen{false}, ready{false};
std::atomic<unsigned> held{0};
// The cursor state is used only on the game's live input-tick thread. No
// handler or CoreWindow pointer is retained between ticks.
thread_local menu_mouse::CursorPolicy cursor;

bool read(const void* address,void* dst,std::size_t n) {
 SIZE_T got=0; return address && ReadProcessMemory(GetCurrentProcess(),address,dst,n,&got) && got==n;
}
template<class T> bool readAt(const void* base,std::size_t offset,T& out) {
 return base && read(static_cast<const unsigned char*>(base)+offset,&out,sizeof(out));
}
bool writable(void* p,std::size_t n) {
 MEMORY_BASIC_INFORMATION m{};
 if (!p || !VirtualQuery(p,&m,sizeof(m)) || m.State!=MEM_COMMIT || (m.Protect&(PAGE_GUARD|PAGE_NOACCESS))) return false;
 DWORD prot=m.Protect&0xff;
 if (prot!=PAGE_READWRITE && prot!=PAGE_WRITECOPY && prot!=PAGE_EXECUTE_READWRITE && prot!=PAGE_EXECUTE_WRITECOPY) return false;
 auto x=reinterpret_cast<std::uintptr_t>(p), end=reinterpret_cast<std::uintptr_t>(m.BaseAddress)+m.RegionSize;
 return x<=end && n<=end-x;
}
bool executable(std::uintptr_t p) {
 MEMORY_BASIC_INFORMATION m{};
 if (!p || !VirtualQuery(reinterpret_cast<void*>(p),&m,sizeof(m)) || m.State!=MEM_COMMIT || (m.Protect&(PAGE_GUARD|PAGE_NOACCESS))) return false;
 DWORD prot=m.Protect&0xff;
 return prot==PAGE_EXECUTE || prot==PAGE_EXECUTE_READ || prot==PAGE_EXECUTE_READWRITE || prot==PAGE_EXECUTE_WRITECOPY;
}
void eventHook(void* queue,void** slot) {
 if (!ready.load(std::memory_order_acquire)) { realQueue(queue,slot); return; }
 void* event=nullptr; std::uintptr_t vt=0, destroy=0;
 unsigned char kind=255, action=0, pressed=0;
 // The vtable and tag must both match the mouse factory. Never suppress or
 // interpret any keyboard, touch, text, or unknown event as a mouse event.
 bool mouse=slot && read(slot,&event,sizeof(event)) && event && readAt(event,0,vt) && vt==image+mouseVtableRva &&
   readAt(event,8,kind) && kind==0 && readAt(event,0x18,action) && readAt(event,0x19,pressed);
 if (!mouse) { realQueue(queue,slot); return; }
 const bool menu=menuOpen.load(std::memory_order_acquire);
 unsigned bit=pressed<=1 ? menu_mouse::buttonBit(action) : 0, old=0;
 if (bit) old=pressed ? held.fetch_or(menu?0:bit,std::memory_order_acq_rel)
                      : held.fetch_and(~bit,std::memory_order_acq_rel);
 if (!menu || (pressed<=1 && menu_mouse::allowMouse(true,action,pressed!=0,old))) { realQueue(queue,slot); return; }
 // Queue takes a unique pointer and invokes the first virtual deleting
 // destructor if it still owns it. Do the same when discarding an event.
 if (!writable(slot,sizeof(*slot)) || !read(reinterpret_cast<void*>(vt),&destroy,sizeof(destroy)) ||
     destroy<image || destroy-image>=0x1a91000 || !executable(destroy)) {
  // Do not send an untrusted click into the world. In this abnormal case we
  // deliberately leak the event rather than execute an unverified destructor.
  return;
 }
 *slot=nullptr;
 reinterpret_cast<DeleteFn>(destroy)(event,1);
}
// If a button was already held when the menu opened, enqueue its UP edge so
// the world does not continue mining/attacking. The object and queue are taken
// from the live handler on its own tick; nothing is cached or fabricated as a
// press. Race-safe with a physical up edge reaching eventHook at the same time.
void releaseHeld(void* handler) {
 std::uintptr_t queue=0; std::int16_t x=0,y=0;
 if (!readAt(handler,0x20,queue) || !queue || !readAt(handler,0x12a,x) || !readAt(handler,0x12c,y)) return;
 unsigned bits=held.exchange(0,std::memory_order_acq_rel);
 for (unsigned action=1;action<=3;++action) if (bits&menu_mouse::buttonBit(action)) {
  unsigned char button=static_cast<unsigned char>(action), up=0;
  std::uint32_t relative=1; void* event=nullptr;
  factory(&event,&button,&up,&relative,&x,&y);
  if (event) realQueue(reinterpret_cast<void*>(queue),&event);
  else held.fetch_or(menu_mouse::buttonBit(action),std::memory_order_acq_rel);
 }
}
std::uintptr_t tickHook(void* handler) {
 if (!ready.load(std::memory_order_acquire) || !handler) return realTick(handler);
 unsigned char grabbed=0; std::uint32_t command=0;
 auto* commandPtr=static_cast<unsigned char*>(handler)+0x74;
 if (!readAt(handler,0x72,grabbed) || grabbed>1 || !readAt(handler,0x74,command) || command>2 || !writable(commandPtr,sizeof(command))) {
  return realTick(handler);
 }
 const bool menu=menuOpen.load(std::memory_order_acquire);
 auto d=cursor.step(menu,grabbed!=0,command,GetTickCount64());
 if (menu && held.load(std::memory_order_acquire)) releaseHeld(handler);
 if (d.command!=command) std::memcpy(commandPtr,&d.command,sizeof(d.command));
 return realTick(handler);
}
}
void SetMenuMouseOpen(bool value) { menuOpen.store(value,std::memory_order_release); }
int MenuMouseStatus() { return ready.load(std::memory_order_acquire)?1:0; }
bool InitializeMenuMouse(HatCreateHook create,HatToggleHook enable,HatToggleHook disable,HatToggleHook remove) {
 // ChinaHatAvailable means the shared verifier has checked the complete disk
 // SHA256, PE identity and its own loaded signatures. Verify our sites too.
 if (!ChinaHatAvailable() || !create || !enable || !disable || !remove) return false;
 image=reinterpret_cast<std::uintptr_t>(GetModuleHandleW(nullptr));
 if (!image) return false;
 for (const auto& sig:signatures) {
  unsigned char bytes[16]{};
  if (!read(reinterpret_cast<void*>(image+sig.rva),bytes,sizeof(bytes)) || std::memcmp(bytes,sig.bytes,sizeof(bytes))!=0) return false;
 }
 factory=reinterpret_cast<FactoryFn>(image+factoryRva);
 visual_hooks::Hook hooks[]={
  {reinterpret_cast<void*>(image+queueRva),reinterpret_cast<void*>(&eventHook),reinterpret_cast<void**>(&realQueue)},
  {reinterpret_cast<void*>(image+tickRva),reinterpret_cast<void*>(&tickHook),reinterpret_cast<void**>(&realTick)}
 };
 if (!visual_hooks::install(hooks,2,create,enable,disable,remove)) return false;
 ready.store(true,std::memory_order_release); return true;
}
