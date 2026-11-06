// Experimental rendering adapter for the user's exact 1.1.5 executable.
// The design was researched independently from public version-specific
// entry-point descriptions; it does not embed or redistribute their code.
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include "china_hat_native.h"
#include "china_hat_validation.h"
#include "china_hat_lines.h"
#include "china_hat_hash.h"
#include "visual_trail.h"
#include "china_hat_player.h"
#include "visual_hooks.h"
#include <bcrypt.h>
#include <atomic>
#include <cstdint>
#include <cstring>
#include <cmath>
#include <algorithm>

namespace {
using ClientTick = void (*)(void*,int,int);
using PlayerRender = std::intptr_t (*)(void*,void*,void*,void*,float);
using ScreenRender = void (*)(void*,void*);
using TessBegin = void (*)(void*,std::uint8_t,int);
using TessColor = void (*)(void*,std::uint8_t,std::uint8_t,std::uint8_t,std::uint8_t);
using TessVertex = void (*)(void*,float,float,float);
using TessDraw = void (*)(void*,void*,std::int64_t);
using TessEnd = void (*)(void*,void*,std::int64_t,std::uint8_t);
constexpr std::uint32_t playerRva=0x581910, screenRva=0x3528c0, clientTickRva=0x11b2d0, beginRva=0x5d0660,
 colorRva=0x5d0890, vertexRva=0x5d0a90, drawRva=0x5d1840, endRva=0x5d14f0;
constexpr std::uint32_t originRva=0x19436e8, tessRva=0x1925550, contextRva=0x1913b40,
 shaderColorRva=0x192ae08, shaderFlagRva=0x192ae18;
std::atomic<bool> available{false}, enabled{false};
std::atomic<bool> trailEnabled{false};
std::atomic<float> heightOffset{0.0f};
std::atomic<unsigned> trailGeneration{0};
thread_local visual_trail::History trail;
thread_local unsigned seenTrailGeneration=0;
// 0 waiting for a player, 1 submitted to GPU, 2 game renderer busy/unavailable.
std::atomic<int> hatStatus{0}, trailStatus{0};
std::atomic<ULONGLONG> lastHatSubmit{0}, lastTrailSubmit{0};
bool anyVisual() { return enabled.load(std::memory_order_acquire) || trailEnabled.load(std::memory_order_acquire); }
// 0 = not attempted, 1 = installing, 2 = installed, 3 = failed.
std::atomic<int> hookState{0};
HatCreateHook createHook=nullptr; HatToggleHook enableHook=nullptr, disableHook=nullptr, removeHook=nullptr;
std::uintptr_t base=0;
ClientTick originalTick=nullptr;
std::atomic<std::uintptr_t> clientInstance{0};
std::atomic<ULONGLONG> lastClientTick{0};
PlayerRender originalPlayer=nullptr;
ScreenRender originalScreen=nullptr;
TessBegin tessBegin=nullptr; TessColor tessColor=nullptr; TessVertex tessVertex=nullptr;
TessDraw tessDraw=nullptr; TessEnd tessEnd=nullptr;
struct PlayerFrame { china_hat::Point world{}; float height=0; std::uintptr_t entity=0; ULONGLONG at=0; bool valid=false; };
thread_local PlayerFrame frame;
thread_local bool inScreen=false;
void log(const char* s) { OutputDebugStringA("GAYClient China Hat: "); OutputDebugStringA(s); OutputDebugStringA("\n"); }

bool read(std::uintptr_t p, void* out, std::size_t n) {
 SIZE_T got=0;
 return p && ReadProcessMemory(GetCurrentProcess(),reinterpret_cast<const void*>(p),out,n,&got) && got==n;
}
template<class T> bool fetch(std::uintptr_t p, T& out) { return read(p,&out,sizeof(out)); }
// Only a recent ClientInstance's local-player slot is accepted. The old
// 0x1921df8 chain is deliberately not a fallback: on a server it can identify
// a different player while still returning plausible coordinates.
bool localPlayer(std::uintptr_t& result) {
 auto observed=clientInstance.load(std::memory_order_acquire);
 auto tick=lastClientTick.load(std::memory_order_acquire);
 if (!hat_player::fromClient(observed,tick,GetTickCount64(),
     [](std::uintptr_t address,std::uintptr_t& value) { return fetch(address,value); },result)) return false;
 if (clientInstance.load(std::memory_order_acquire)!=observed) { result=0; return false; }
 return true;
}
bool writable(std::uintptr_t p, std::size_t n) {
 MEMORY_BASIC_INFORMATION m{};
 if (!VirtualQuery(reinterpret_cast<void*>(p),&m,sizeof(m)) || m.State!=MEM_COMMIT || (m.Protect&(PAGE_GUARD|PAGE_NOACCESS))) return false;
 DWORD prot=m.Protect & 0xff;
 if (prot!=PAGE_READWRITE && prot!=PAGE_WRITECOPY && prot!=PAGE_EXECUTE_READWRITE && prot!=PAGE_EXECUTE_WRITECOPY) return false;
 std::uintptr_t end=reinterpret_cast<std::uintptr_t>(m.BaseAddress)+m.RegionSize;
 return p<=end && n<=end-p;
}
struct BCryptApi {
 using Algorithm=BCRYPT_ALG_HANDLE;
 using Hash=BCRYPT_HASH_HANDLE;
 bool openAlgorithm(Algorithm& out) { return BCryptOpenAlgorithmProvider(&out,BCRYPT_SHA256_ALGORITHM,nullptr,0)>=0; }
 bool property(Algorithm h,LPCWSTR name,std::uint32_t& value) {
  DWORD got=0, data=0;
  bool ok=BCryptGetProperty(h,name,reinterpret_cast<PUCHAR>(&data),sizeof(data),&got,0)>=0 && got==sizeof(data);
  value=data; return ok;
 }
 bool objectLength(Algorithm h,std::uint32_t& n) { return property(h,BCRYPT_OBJECT_LENGTH,n); }
 bool hashLength(Algorithm h,std::uint32_t& n) { return property(h,BCRYPT_HASH_LENGTH,n); }
 bool createHash(Algorithm a,Hash& h,std::uint8_t* object,std::uint32_t n) {
  return BCryptCreateHash(a,&h,object,n,nullptr,0,0)>=0;
 }
 bool hashData(Hash h,const std::uint8_t* bytes,std::uint32_t n) {
  return BCryptHashData(h,const_cast<PUCHAR>(bytes),n,0)>=0;
 }
 bool finishHash(Hash h,std::uint8_t* result,std::uint32_t n) { return BCryptFinishHash(h,result,n,0)>=0; }
 void destroyHash(Hash h) { BCryptDestroyHash(h); }
 void closeAlgorithm(Algorithm a) { BCryptCloseAlgorithmProvider(a,0); }
};
bool hashFile(const BYTE* bytes,std::size_t size,std::uint8_t result[32]) {
 BCryptApi api;
 return hashHatExecutable(api,bytes,size,result);
}
// Reads a specific executable RVA only if it is wholly file-backed and in an
// executable PE section. This also verifies candidate screen/end entry points,
// for which the user did not provide a separate prologue in the diagnostic.
bool fileCodeAt(const BYTE* file,std::size_t size,const IMAGE_NT_HEADERS64& nt,std::size_t table,
                std::uint32_t rva,const BYTE*& out) {
 for (unsigned i=0;i<nt.FileHeader.NumberOfSections;i++) {
  const std::size_t off=table+i*sizeof(IMAGE_SECTION_HEADER);
  if (off>size || sizeof(IMAGE_SECTION_HEADER)>size-off) return false;
  IMAGE_SECTION_HEADER sec{}; std::memcpy(&sec,file+off,sizeof(sec));
  if (!(sec.Characteristics & IMAGE_SCN_MEM_EXECUTE) || rva<sec.VirtualAddress) continue;
  const std::uint64_t delta=static_cast<std::uint64_t>(rva)-sec.VirtualAddress;
  if (delta+16>sec.SizeOfRawData || delta+16>sec.Misc.VirtualSize ||
      static_cast<std::uint64_t>(sec.PointerToRawData)+delta+16>size) continue;
  out=file+sec.PointerToRawData+delta; return true;
 }
 return false;
}
bool validateGame() {
 HMODULE module=GetModuleHandleW(nullptr); if (!module) return false;
 wchar_t path[32768]{}; DWORD n=GetModuleFileNameW(module,path,32768);
 if (!n || n>=32768) return false;
 const wchar_t* name=wcsrchr(path,L'\\');
 if (_wcsicmp(name?name+1:path,L"Minecraft.Windows.exe")!=0) return false;
 HANDLE file=CreateFileW(path,GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
 if (file==INVALID_HANDLE_VALUE) return false;
 LARGE_INTEGER length{}; HANDLE mapping=nullptr; const BYTE* bytes=nullptr; bool valid=false;
 if (GetFileSizeEx(file,&length) && length.QuadPart>0 && length.QuadPart<=128LL*1024*1024) {
  mapping=CreateFileMappingW(file,nullptr,PAGE_READONLY,0,0,nullptr);
  if (mapping) bytes=static_cast<const BYTE*>(MapViewOfFile(mapping,FILE_MAP_READ,0,0,0));
 }
 if (bytes) {
  std::size_t size=static_cast<std::size_t>(length.QuadPart);
  IMAGE_DOS_HEADER dos{}; IMAGE_NT_HEADERS64 nt{}; std::uint8_t digest[32]{};
  if (size>=sizeof(dos)) std::memcpy(&dos,bytes,sizeof(dos));
  const bool dosOk=dos.e_magic==IMAGE_DOS_SIGNATURE && dos.e_lfanew>0 && dos.e_lfanew<=0x10000 &&
                   static_cast<std::size_t>(dos.e_lfanew)+sizeof(nt)<=size;
  if (dosOk) std::memcpy(&nt,bytes+dos.e_lfanew,sizeof(nt));
  if (dosOk && nt.Signature==IMAGE_NT_SIGNATURE && nt.FileHeader.Machine==IMAGE_FILE_MACHINE_AMD64 &&
      nt.OptionalHeader.Magic==IMAGE_NT_OPTIONAL_HDR64_MAGIC && nt.FileHeader.NumberOfSections>0 && nt.FileHeader.NumberOfSections<=96 &&
      nt.FileHeader.SizeOfOptionalHeader>=sizeof(IMAGE_OPTIONAL_HEADER64) && hashFile(bytes,size,digest) &&
      hat_validation::identity(digest,nt.FileHeader.TimeDateStamp,nt.OptionalHeader.SizeOfImage)) {
   const std::size_t table=static_cast<std::size_t>(dos.e_lfanew)+4+sizeof(IMAGE_FILE_HEADER)+nt.FileHeader.SizeOfOptionalHeader;
   const auto image=reinterpret_cast<std::uintptr_t>(module);
   valid=true;
   for (const auto& sig:hat_validation::signatures) {
    const BYTE* fromFile=nullptr; std::uint8_t inMemory[16]{};
    if (!fileCodeAt(bytes,size,nt,table,sig.rva,fromFile) || std::memcmp(fromFile,sig.bytes,16)!=0 ||
        !read(image+sig.rva,inMemory,16) || std::memcmp(inMemory,fromFile,16)!=0) { valid=false; break; }
   }
   for (auto rva : {screenRva,endRva,clientTickRva}) {
    const BYTE* fromFile=nullptr; std::uint8_t inMemory[16]{};
    if (!fileCodeAt(bytes,size,nt,table,rva,fromFile) || !read(image+rva,inMemory,16) || std::memcmp(inMemory,fromFile,16)!=0) valid=false;
   }
   if (valid) base=image;
  }
  UnmapViewOfFile(bytes);
 }
 if (mapping) CloseHandle(mapping); CloseHandle(file); return valid;
}

// 1.1.5 candidate tick signature, called only after exact EXE identity and
// loaded/file code comparison. This observes the live ClientInstance; it never
// writes to the instance or changes the original tick arguments.
void clientTickHook(void* self,int a2,int a3) {
 auto instance=reinterpret_cast<std::uintptr_t>(self);
 if (hat_player::alignedPointer(instance)) {
  clientInstance.store(instance,std::memory_order_release);
  lastClientTick.store(GetTickCount64(),std::memory_order_release);
 }
 originalTick(self,a2,a3);
}
void rememberPlayer(void* entity,float partial) {
 frame.valid=false;
 if (!anyVisual() || !entity || !std::isfinite(partial)) return;
 std::uintptr_t actual=0, actor=reinterpret_cast<std::uintptr_t>(entity);
 if (!localPlayer(actual) || actual!=actor) return;
 std::int32_t type=0; float current[3]{}, old[3]{}, dims[2]{};
 if (!fetch(actor+0x14c,type) || type!=30 || !read(actor+0x88,current,sizeof(current)) ||
     !read(actor+0x94,old,sizeof(old)) || !read(actor+0x198,dims,sizeof(dims)) ||
     !std::isfinite(dims[0]) || !std::isfinite(dims[1]) || dims[0]<0.2f || dims[0]>1.5f || dims[1]<0.8f || dims[1]>2.5f) return;
 partial=std::clamp(partial,0.0f,1.0f); float value[3]{};
 for (int i=0;i<3;i++) {
  if (!std::isfinite(current[i]) || !std::isfinite(old[i]) || std::fabs(current[i]-old[i])>64.0f) return;
  value[i]=old[i]+(current[i]-old[i])*partial;
 }
 if (std::fabs(value[0])>30000000 || std::fabs(value[1])>4096 || std::fabs(value[2])>30000000) return;
 std::uintptr_t again=0; if (!localPlayer(again) || again!=actor) return;
 frame={{value[0],value[1],value[2]},dims[1],actor,GetTickCount64(),true};
}
std::intptr_t playerHook(void* self,void* entity,void* pos,void* extra,float partial) {
 const auto result=originalPlayer(self,entity,pos,extra,partial);
 // Do not clear a local-player frame when another player's renderer runs.
 if (anyVisual()) {
  std::uintptr_t local=0;
  if (localPlayer(local) && reinterpret_cast<std::uintptr_t>(entity)==local) rememberPlayer(entity,partial);
 }
 return result;
}
// Keep the previously user-verified wireframe draw path isolated from the
// experimental trail. In particular do not change depth, tint, topology or
// tessellator sequencing for the hat when the trail is disabled.
void renderHat(const PlayerFrame& player) {
 if (!enabled.load(std::memory_order_acquire)) return;
 if (!player.valid || GetTickCount64()-player.at>250) { hatStatus.store(0,std::memory_order_relaxed); return; }
 hatStatus.store(2,std::memory_order_relaxed);
 std::uintptr_t actor=0; if (!localPlayer(actor) || actor!=player.entity) return;
 float origin[3]{}, saved[4]{}; std::uint8_t oldFlag=0, state[0x178]{};
 auto tess=base+tessRva, color=base+shaderColorRva, flag=base+shaderFlagRva;
 if (!read(base+originRva,origin,sizeof(origin)) || !read(tess,state,sizeof(state)) ||
     !read(color,saved,sizeof(saved)) || !fetch(flag,oldFlag) || !writable(color,sizeof(saved)) || !writable(flag,1) ||
     !writable(tess,sizeof(state))) return;
 // Never reset or flush a tessellator another game operation is using.
 std::int32_t pending=0; std::memcpy(&pending,state+0x168,sizeof(pending));
 if (state[0x125] || state[0x170] || pending) return;
 for (float v:origin) if (!std::isfinite(v)) return;
 china_hat::Point head{player.world.x-origin[0],player.world.y-origin[1]-1.62f+player.height+0.03f+heightOffset.load(std::memory_order_relaxed),player.world.z-origin[2]};
 if (std::fabs(head.x)>16 || std::fabs(head.y)>16 || std::fabs(head.z)>16) return;
 china_hat::Mesh mesh{};
 if (!china_hat::build(mesh,head,.58f,.34f,48,static_cast<float>(GetTickCount64()%8000)/8000.0f,255)) return;
 float white[4]={1,1,1,1}; std::memcpy(reinterpret_cast<void*>(color),white,sizeof(white)); *reinterpret_cast<std::uint8_t*>(flag)=1;
 auto* t=reinterpret_cast<void*>(tess);
 // The documented 1.1.5 renderer uses topology 4 for line segments. The first
 // experimental version is a wireframe hat, not a filled/textured cone.
 constexpr int segments=48, spokes=12;
 tessBegin(t,4,2*(segments+spokes));
 auto vertex=[&](const china_hat::Vertex& v) { tessColor(t,v.color.r,v.color.g,v.color.b,v.color.a); tessVertex(t,v.position.x,v.position.y,v.position.z); };
 china_hat::emitWireframe(mesh,spokes,vertex);
 tessDraw(t,reinterpret_cast<void*>(base+contextRva),0);
 lastHatSubmit.store(GetTickCount64(),std::memory_order_relaxed);
 hatStatus.store(1,std::memory_order_relaxed);
 // The researched renderer releases its temporary tessellation at this point.
 // A zeroed, aligned output slot is provided as required by the native ABI.
 alignas(16) std::uint8_t scratch[0x200]{};
 tessEnd(t,scratch,0,0);
 std::memcpy(reinterpret_cast<void*>(color),saved,sizeof(saved)); *reinterpret_cast<std::uint8_t*>(flag)=oldFlag;
}

void renderTrail(const PlayerFrame& player) {
 unsigned gen=trailGeneration.load(std::memory_order_acquire);
 if (gen!=seenTrailGeneration) { trail.clear(); seenTrailGeneration=gen; }
 if (!trailEnabled.load(std::memory_order_acquire)) { trail.clear(); trailStatus.store(0,std::memory_order_relaxed); return; }
 const ULONGLONG now=GetTickCount64();
 if (!player.valid || now-player.at>250) { trail.clear(); trailStatus.store(0,std::memory_order_relaxed); return; }
 std::uintptr_t actor=0;
 if (!localPlayer(actor) || actor!=player.entity) { trail.clear(); trailStatus.store(0,std::memory_order_relaxed); return; }
 china_hat::Point feet{player.world.x,player.world.y-1.62f+.10f,player.world.z};
 trail.update(actor,feet,now);
 if (trail.size()<2) { trailStatus.store(0,std::memory_order_relaxed); return; }
 trailStatus.store(2,std::memory_order_relaxed);
 float origin[3]{}, saved[4]{}; std::uint8_t oldFlag=0, state[0x178]{};
 auto tess=base+tessRva, color=base+shaderColorRva, flag=base+shaderFlagRva;
 if (!read(base+originRva,origin,sizeof(origin)) || !read(tess,state,sizeof(state)) ||
     !read(color,saved,sizeof(saved)) || !fetch(flag,oldFlag) || !writable(color,sizeof(saved)) || !writable(flag,1) ||
     !writable(tess,sizeof(state))) return;
 std::int32_t pending=0; std::memcpy(&pending,state+0x168,sizeof(pending));
 if (state[0x125] || state[0x170] || pending) return;
 for (float v:origin) if (!std::isfinite(v)) return;
 china_hat::Vertex points[visual_trail::History::capacity]{}; bool valid[visual_trail::History::capacity]{};
 int segments=0;
 for (std::size_t i=0;i<trail.size();++i) {
  const auto& p=trail[i]; auto& v=points[i];
  v.position={p.point.x-origin[0],p.point.y-origin[1],p.point.z-origin[2]};
  valid[i]=std::fabs(v.position.x)<32 && std::fabs(v.position.y)<32 && std::fabs(v.position.z)<32;
  float age=static_cast<float>(now-p.time)/visual_trail::History::lifetime;
  v.color=china_hat::rainbow(.38f + static_cast<float>(i)*.002f,static_cast<std::uint8_t>(255.f*(1.f-std::clamp(age,0.f,1.f))));
  if (i && valid[i] && valid[i-1]) ++segments;
 }
 if (!segments) return;
 float white[4]={1,1,1,1}; std::memcpy(reinterpret_cast<void*>(color),white,sizeof(white)); *reinterpret_cast<std::uint8_t*>(flag)=1;
 auto* t=reinterpret_cast<void*>(tess);
 tessBegin(t,4,segments*2);
 auto vertex=[&](const china_hat::Vertex& v) { tessColor(t,v.color.r,v.color.g,v.color.b,v.color.a); tessVertex(t,v.position.x,v.position.y,v.position.z); };
 for (std::size_t i=1;i<trail.size();++i) if (valid[i] && valid[i-1]) { vertex(points[i-1]); vertex(points[i]); }
 tessDraw(t,reinterpret_cast<void*>(base+contextRva),0);
 lastTrailSubmit.store(GetTickCount64(),std::memory_order_relaxed);
 trailStatus.store(1,std::memory_order_relaxed);
 alignas(16) std::uint8_t scratch[0x200]{}; tessEnd(t,scratch,0,0);
 std::memcpy(reinterpret_cast<void*>(color),saved,sizeof(saved)); *reinterpret_cast<std::uint8_t*>(flag)=oldFlag;
}
void screenHook(void* self,void* ctx) {
 if (inScreen) { originalScreen(self,ctx); return; }
 inScreen=true;
 originalScreen(self,ctx);
 PlayerFrame visible=frame; frame.valid=false;
 // Submit the verified hat before collecting/drawing the experimental trail.
 renderHat(visible);
 PlayerFrame local=visible;
 if (trailEnabled.load(std::memory_order_acquire) && !local.valid) {
  std::uintptr_t actor=0;
  if (localPlayer(actor)) { rememberPlayer(reinterpret_cast<void*>(actor),1.0f); local=frame; frame.valid=false; }
 }
 renderTrail(local);
 inScreen=false;
}
}
bool ChinaHatAvailable() { return available.load(std::memory_order_acquire); }
bool ChinaHatEnabled() { return enabled.load(std::memory_order_acquire); }
namespace {
bool installHooks() {
 int state=0;
 if (!hookState.compare_exchange_strong(state,1,std::memory_order_acq_rel)) return state==2;
 // The user can wait a long time before checking the box. Revalidate loaded
 // instructions immediately before patching, in case another component has
 // changed the game code since bootstrap.
 if (!validateGame()) {
  available.store(false,std::memory_order_release);
  hookState.store(3,std::memory_order_release);
  log("game changed or could not be verified before enabling"); return false;
 }
 auto p=reinterpret_cast<void*>(base+playerRva), s=reinterpret_cast<void*>(base+screenRva);
 auto tick=reinterpret_cast<void*>(base+clientTickRva);
 // All hooks are created before enabling; rollback never removes active hooks.
 visual_hooks::Hook hooks[]={{tick,reinterpret_cast<void*>(&clientTickHook),reinterpret_cast<void**>(&originalTick)},
                             {p,reinterpret_cast<void*>(&playerHook),reinterpret_cast<void**>(&originalPlayer)},
                             {s,reinterpret_cast<void*>(&screenHook),reinterpret_cast<void**>(&originalScreen)}};
 bool success=visual_hooks::install(hooks,3,createHook,enableHook,disableHook,removeHook);
 if (!success) {
  clientInstance.store(0,std::memory_order_release); lastClientTick.store(0,std::memory_order_release);
  log("visual hook installation failed");
 }
 if (!success) available.store(false,std::memory_order_release);
 hookState.store(success?2:3,std::memory_order_release);
 return success;
}
}
namespace {
bool enableVisual() {
 if (!ChinaHatAvailable()) return false;
 try { return installHooks(); }
 catch (...) {
  enabled.store(false,std::memory_order_release); trailEnabled.store(false,std::memory_order_release);
  available.store(false,std::memory_order_release); hookState.store(3,std::memory_order_release);
  log("failed to enable; overlay remains available"); return false;
 }
}
}
void SetChinaHatEnabled(bool value) {
 if (!value) { enabled.store(false,std::memory_order_release); hatStatus.store(0,std::memory_order_relaxed); return; }
 hatStatus.store(0,std::memory_order_relaxed);
 if (enableVisual()) enabled.store(true,std::memory_order_release);
}
bool TrailEnabled() { return trailEnabled.load(std::memory_order_acquire); }
void SetTrailEnabled(bool value) {
 if (!value) { trailEnabled.store(false,std::memory_order_release); trailGeneration.fetch_add(1,std::memory_order_acq_rel); trailStatus.store(0,std::memory_order_relaxed); return; }
 trailStatus.store(0,std::memory_order_relaxed);
 if (enableVisual()) trailEnabled.store(true,std::memory_order_release);
}
float ChinaHatHeight() { return heightOffset.load(std::memory_order_acquire); }
void SetChinaHatHeight(float value) { if (std::isfinite(value)) heightOffset.store(std::clamp(value,-0.2f,0.4f),std::memory_order_release); }
int ChinaHatStatus() {
 int status=hatStatus.load(std::memory_order_relaxed);
 auto then=lastHatSubmit.load(std::memory_order_relaxed), now=GetTickCount64();
 return status==1 && (!then || now<then || now-then>1000) ? 0 : status;
}
int TrailStatus() {
 int status=trailStatus.load(std::memory_order_relaxed);
 auto then=lastTrailSubmit.load(std::memory_order_relaxed), now=GetTickCount64();
 return status==1 && (!then || now<then || now-then>1000) ? 0 : status;
}
bool InitializeChinaHat(HatCreateHook create,HatToggleHook enable,HatToggleHook disable,HatToggleHook remove) {
 if (!create || !enable || !disable || !remove || !validateGame()) { log("unsupported game or unable to verify executable; disabled"); return false; }
 tessBegin=reinterpret_cast<TessBegin>(base+beginRva); tessColor=reinterpret_cast<TessColor>(base+colorRva);
 tessVertex=reinterpret_cast<TessVertex>(base+vertexRva); tessDraw=reinterpret_cast<TessDraw>(base+drawRva); tessEnd=reinterpret_cast<TessEnd>(base+endRva);
 createHook=create; enableHook=enable; disableHook=disable; removeHook=remove;
 available.store(true,std::memory_order_release);
 log("validated; hooks and drawing stay off until enabled in menu"); return true;
}
