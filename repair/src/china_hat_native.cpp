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
#include "visual_jump.h"
#include "visual_accessory.h"
#include "katana_asset.h"
#include "katana_geometry.h"
#include "katana_pose.h"
#include "katana_rotation.h"
#include "katana_evidence.h"
#include "diamond_sword_candidate.h"
#include "china_hat_player.h"
#include "visual_hooks.h"
#include "visual_color.h"
#include "visual_ribbon.h"
#include "visual_gradient.h"
#include "visual_depth_policy.h"
#include "visual_gpu_depth.h"
#include "visual_player_skin.h"
#include "visual_player_replay.h"
#include <d3d11.h>
#include "hat_animation.h"
#include <bcrypt.h>
#include <atomic>
#include <cstdint>
#include <cstring>
#include <cmath>
#include <algorithm>
#include <vector>

namespace {
using ClientTick = void (*)(void*,int,int);
// In the recorded x64 dispatcher, the fourth parameter is passed in XMM3;
// preserve it as float even though this wrapper's exact meaning is unknown.
using PlayerRender = std::intptr_t (*)(void*,void*,void*,float,float);
using ScreenRender = void (*)(void*,void*);
using EntitiesRender = std::intptr_t (*)(void*,float);
using TessBegin = void (*)(void*,std::uint8_t,int);
using TessColor = void (*)(void*,std::uint8_t,std::uint8_t,std::uint8_t,std::uint8_t);
using TessVertex = void (*)(void*,float,float,float);
using TessDraw = void (*)(void*,void*,std::int64_t);
using TessEnd = void (*)(void*,void*,std::int64_t,std::uint8_t);
constexpr std::uint32_t playerRva=0x581910, screenRva=0x3528c0, clientTickRva=0x11b2d0, beginRva=0x5d0660,
 colorRva=0x5d0890, vertexRva=0x5d0a90, drawRva=0x5d1840, endRva=0x5d14f0, worldEntitiesRva=0x5ae250;
constexpr std::uint32_t originRva=0x19436e8, tessRva=0x1925550, contextRva=0x1913b40,
 shaderColorRva=0x192ae08, shaderFlagRva=0x192ae18;
std::atomic<bool> available{false}, enabled{false};
// Modules remain off until the user explicitly enables them. Defaults configure
// the requested filled/rainbow appearance, with a wireframe fallback in Visuals.
std::atomic<bool> accessoryEnabled{false};
// Experimental, first-person only, disabled on every startup.
std::atomic<bool> katanaEnabled{false};
std::atomic<int> katanaStatus{0};
std::atomic<std::uint32_t> katanaEvents{0};
std::atomic<ULONGLONG> lastKatanaSubmit{0};
std::atomic<ULONGLONG> lastKatanaVisible{0}, lastKatanaSuppress{0};
std::atomic<ULONGLONG> lastFpSwordSeen{0}, lastTpSwordSeen{0};
// Texture fingerprint learned from a first-person verified diamond sword draw;
// used to recognise the same item on the third-person cached path.
std::atomic<std::uintptr_t> katanaSwordTexture{0};
// Third-person identity diagnostics: how many cached item draws happened
// inside/outside the local player's renderer scope, and the item-context
// texture they carried. Display only; nothing renders from these.
std::atomic<int> katanaTpCallsInPass{0}, katanaTpCallsOutOfPass{0};
std::atomic<std::uintptr_t> katanaTpLastTexture{0};
std::atomic<int> katanaYawSign{1};
std::atomic<int> katanaHookState{0}, katanaHookFailure{8};
using HeldRender=std::uintptr_t (*)(void*,void*,void*,std::uintptr_t,float,std::uint8_t,std::uint8_t);
using HeldDraw=std::uintptr_t (*)(void*,void*,std::uint8_t);
using HeldGeometry=std::uintptr_t (*)(void*,void*,void*,void*,int);
HeldRender originalHeldRender=nullptr; HeldDraw originalHeldDraw=nullptr; HeldGeometry originalHeldGeometry=nullptr;
thread_local bool katanaScope=false, katanaLocalPlayerPass=false;
// Third-person identity observed during the local player's renderer scope.
thread_local bool katanaThirdSwordSeen=false;
// Position samples of the local player, used only to sanity-check the yaw
// convention against the direction the player actually moves.
thread_local struct KatanaTrack {
 static constexpr std::size_t capacity=16;
 float x[capacity]{}, y[capacity]{}, z[capacity]{};
 ULONGLONG t[capacity]{};
 std::size_t head=0, count=0;
 void push(float px,float py,float pz,ULONGLONG now) {
  x[head]=px; y[head]=py; z[head]=pz; t[head]=now;
  head=(head+1)%capacity; if (count<capacity) ++count;
 }
} katanaTrack;
thread_local katana_rotation::YawSignVote katanaYawVotes;
thread_local bool katanaFirstPerson=true;
// --- Katana GPU visibility verification -------------------------------------
// A submitted Draw is not proof of visible pixels. Every katana batch is
// wrapped in a D3D11 occlusion query; only a query that counted rasterised
// samples allows the original sword to be suppressed. Any query failure
// permanently disables verification, which keeps the original item visible.
struct KatanaQueries {
 static constexpr std::size_t capacity=8;
 static constexpr UINT64 minSamples=16;
 ID3D11Query* slots[capacity]{};
 bool inFlight[capacity]{};
 bool disabled=false;
 void idleReset() { for (std::size_t i=0;i<capacity;++i) inFlight[i]=false; }
 void releaseAll() {
  for (std::size_t i=0;i<capacity;++i) { inFlight[i]=false; if (slots[i]) { slots[i]->Release(); slots[i]=nullptr; } }
 }
 ID3D11Query* wrapBegin(ID3D11DeviceContext* ctx) {
  if (disabled || !ctx) return nullptr;
  for (std::size_t n=0;n<capacity;++n) {
   if (inFlight[n]) continue;
   if (!slots[n]) {
    ID3D11Device* device=nullptr; ctx->GetDevice(&device);
    if (!device) { disabled=true; return nullptr; }
    D3D11_QUERY_DESC desc{}; desc.Query=D3D11_QUERY_OCCLUSION;
    HRESULT hr=device->CreateQuery(&desc,&slots[n]);
    device->Release();
    if (FAILED(hr) || !slots[n]) { disabled=true; return nullptr; }
   }
   inFlight[n]=true;
   ctx->Begin(slots[n]);
   return slots[n];
  }
  return nullptr;
 }
 void wrapEnd(ID3D11DeviceContext* ctx, ID3D11Query* query) {
  if (!ctx || !query) return;
  ctx->End(query);
 }
 void poll(ID3D11DeviceContext* ctx, ULONGLONG now) {
  if (disabled || !ctx) return;
  for (std::size_t i=0;i<capacity;++i) {
   if (!inFlight[i] || !slots[i]) continue;
   UINT64 samples=0;
   const HRESULT hr=ctx->GetData(slots[i],&samples,sizeof(samples),1 /*DO_NOT_WAIT*/);
   if (hr==S_FALSE) continue;
   inFlight[i]=false;
   if (FAILED(hr)) { disabled=true; releaseAll(); return; }
   if (samples>=minSamples) {
    lastKatanaVisible.store(now,std::memory_order_relaxed);
    katanaEvents.fetch_or(katana_evidence::occlusionVisible,std::memory_order_relaxed);
   }
  }
 }
};
KatanaQueries katanaQueries;  // touched only on the render thread
bool katanaVisibleRecently(ULONGLONG now) {
 const auto then=lastKatanaVisible.load(std::memory_order_relaxed);
 return then && now>=then && now-then<=1200;
}
std::atomic<int> accessoryStatus{0};
std::atomic<ULONGLONG> lastAccessorySubmit{0};
std::atomic<bool> jumpEnabled{false}, jumpRainbow{false};
std::atomic<bool> trailEnabled{false}, materialTint{true}, hatFill{true}, trailFill{true};
std::atomic<bool> hatRainbow{true}, trailRainbow{true}, vertexGradient{false};
// Default to fail-closed world rendering. Legacy screen rendering is a manual
// fallback only and may show through walls.
std::atomic<bool> strictDepth{true}, worldHookReady{false}, gpuHooksReady{false};
// Off by default: preserve the current world pass until the alternate timing
// is actually tested by the player. Does not change trail or legacy mode.
std::atomic<bool> hatPlayerPass{false};
std::atomic<bool> previewHatCapture{false};
std::atomic<int> hatPlayerPassStatus{0}, hatPlayerCompare{0}, worldDepthCompare{0};
std::atomic<ULONGLONG> lastPlayerPassAttempt{0};
// 0 waiting, 1 guarded GPU draw, 2 no scene depth, 3 hooks unavailable,
// 4 draw blocked, 5 no GPU draw observed (e.g. waiting/busy).
std::atomic<int> occlusionStatus{0};
struct ParamStorage {
 std::atomic<float> values[static_cast<std::size_t>(VisualParam::Count)]{};
 ParamStorage() { for (std::size_t i=0;i<static_cast<std::size_t>(VisualParam::Count);++i) values[i].store(visualParamSpecs[i].value); }
} params;
float param(VisualParam p) { return params.values[static_cast<std::size_t>(p)].load(std::memory_order_acquire); }
// Diagnostics only: record OM bindings before and after the existing screen
// hook, never modify depth/blend. A bound DSV is not proof of valid scene depth.
SRWLOCK contextLock=SRWLOCK_INIT;
ID3D11DeviceContext* observedContext=nullptr;
DWORD observedThread=0;
std::atomic<int> depthBefore{0}, depthAfter{0}, depthWorld{0};
void sampleOutputMerger(std::atomic<int>& target) {
 ID3D11DeviceContext* ctx=nullptr;
 AcquireSRWLockShared(&contextLock);
 if (observedContext && observedThread==GetCurrentThreadId()) { ctx=observedContext; ctx->AddRef(); }
 ReleaseSRWLockShared(&contextLock);
 if (!ctx) { target.store(0,std::memory_order_relaxed); return; }
 ID3D11DepthStencilView* dsv=nullptr; ctx->OMGetRenderTargets(0,nullptr,&dsv);
 ID3D11DepthStencilState* depth=nullptr; UINT ref=0; ctx->OMGetDepthStencilState(&depth,&ref);
 D3D11_DEPTH_STENCIL_DESC desc{};
 if (depth) depth->GetDesc(&desc);
 // D3D11 default null depth state enables LESS depth test and writes.
 int bits=1 | (dsv?2:0) | ((!depth || desc.DepthEnable)?4:0);
 ID3D11BlendState* blend=nullptr; FLOAT factors[4]{}; UINT mask=0;
 ctx->OMGetBlendState(&blend,factors,&mask);
 D3D11_BLEND_DESC blendDesc{}; if (blend) blend->GetDesc(&blendDesc);
 if (blend && blendDesc.RenderTarget[0].BlendEnable) bits |= 8;
 if (blend) blend->Release(); if (depth) depth->Release(); if (dsv) dsv->Release(); ctx->Release();
 target.store(bits,std::memory_order_relaxed);
}
std::atomic<unsigned> trailGeneration{0};
std::atomic<unsigned> jumpGeneration{0};
thread_local visual_jump::History jumps;
thread_local unsigned seenJumpGeneration=0;
std::atomic<int> jumpStatus{0};
std::atomic<ULONGLONG> lastJumpSubmit{0};
thread_local visual_trail::History trail;
thread_local unsigned seenTrailGeneration=0;
// 0 waiting for a player, 1 submitted to GPU, 2 game renderer busy/unavailable.
std::atomic<int> hatStatus{0}, trailStatus{0}, hatFillStatus{0}, trailFillStatus{0};
std::atomic<ULONGLONG> lastHatSubmit{0}, lastTrailSubmit{0};
bool anyVisual() { return enabled.load(std::memory_order_acquire) || trailEnabled.load(std::memory_order_acquire) || jumpEnabled.load(std::memory_order_acquire) || accessoryEnabled.load(std::memory_order_acquire) || katanaEnabled.load(std::memory_order_acquire); }
// 0 = not attempted, 1 = installing, 2 = installed, 3 = failed.
std::atomic<int> hookState{0};
HatCreateHook createHook=nullptr; HatToggleHook enableHook=nullptr, disableHook=nullptr, removeHook=nullptr;
std::uintptr_t base=0;
ClientTick originalTick=nullptr;
std::atomic<std::uintptr_t> clientInstance{0};
std::atomic<ULONGLONG> lastClientTick{0};
PlayerRender originalPlayer=nullptr;
ScreenRender originalScreen=nullptr;
EntitiesRender originalEntities=nullptr;
TessBegin tessBegin=nullptr; TessColor tessColor=nullptr; TessVertex tessVertex=nullptr;
TessDraw tessDraw=nullptr; TessEnd tessEnd=nullptr;
struct PlayerFrame { china_hat::Point world{}; float height=0; std::uintptr_t entity=0; ULONGLONG at=0; bool valid=false; };
thread_local PlayerFrame frame;
thread_local bool inScreen=false;
thread_local bool inEntityPass=false, entityPlayerPass=false, playerPassAttempted=false;
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
// Check again after a draw/end before an optional second tessellator batch.
bool tessIdle(std::uintptr_t tess) {
 std::uint8_t state[0x178]{};
 if (!read(tess,state,sizeof(state)) || !writable(tess,sizeof(state))) return false;
 std::int32_t pending=0; std::memcpy(&pending,state+0x168,sizeof(pending));
 return !state[0x125] && !state[0x170] && !pending;
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
bool validateGame(bool afterVisualHooks=false) {
 // Never excuse memory differences unless the adapter itself completed all
 // required visual hooks. All disk hashes/signatures remain mandatory.
 if (afterVisualHooks && hookState.load(std::memory_order_acquire)!=2) return false;
 const bool worldInstalled=afterVisualHooks && worldHookReady.load(std::memory_order_acquire);
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
    if (!fileCodeAt(bytes,size,nt,table,sig.rva,fromFile) || std::memcmp(fromFile,sig.bytes,16)!=0) { valid=false; break; }
    if (!hat_validation::mayBePatched(sig.rva,afterVisualHooks,worldInstalled) &&
        (!read(image+sig.rva,inMemory,16) || std::memcmp(inMemory,fromFile,16)!=0)) { valid=false; break; }
   }
   for (auto rva : {screenRva,endRva,clientTickRva}) {
    const BYTE* fromFile=nullptr; std::uint8_t inMemory[16]{};
    if (!fileCodeAt(bytes,size,nt,table,rva,fromFile)) { valid=false; continue; }
    if (!hat_validation::mayBePatched(rva,afterVisualHooks,worldInstalled) &&
        (!read(image+rva,inMemory,16) || std::memcmp(inMemory,fromFile,16)!=0)) valid=false;
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
 if (katanaEnabled.load(std::memory_order_acquire)) katanaTrack.push(value[0],value[1],value[2],frame.at);
}
void drawHatAtPlayer(const PlayerFrame& player);
void renderKatana(const PlayerFrame& player);
ID3D11DeviceContext* sameThreadContext();
std::intptr_t playerHook(void* self,void* entity,void* pos,float renderValue,float partial) {
 // Only observe this exact local player's original renderer on the observed
 // immediate-context thread. Optional GPU draw replay is separately opt-in.
 // A nested player renderer (for example another actor) is never captured.
 const bool nested=visual_gpu_depth::inPlayerCapture;
 if (nested) visual_gpu_depth::inPlayerCapture=false;
 struct NestedScope { bool nested; ~NestedScope() { if (nested) visual_gpu_depth::inPlayerCapture=true; } } nestedScope{nested};
 ID3D11DeviceContext* captureContext=nullptr; bool capture=false;
 if (visual_player_skin::isRequested() && !nested) {
  std::uintptr_t local=0;
  if (localPlayer(local) && reinterpret_cast<std::uintptr_t>(entity)==local) {
   captureContext=sameThreadContext();
   if (captureContext && visual_gpu_depth::matches(captureContext)) {
    visual_player_skin::begin(local,captureContext);
    capture=visual_player_skin::scopeContext==captureContext;
    if (capture) visual_player_replay::begin(local,captureContext);
    visual_gpu_depth::inPlayerCapture=capture;
   }
  }
 }
 struct CaptureScope {
  ID3D11DeviceContext* ctx; bool active;
  ~CaptureScope() { if(active) { visual_gpu_depth::inPlayerCapture=false; visual_player_replay::end(); visual_player_skin::end(); } if(ctx)ctx->Release(); }
 } captureScope{captureContext,capture};
 std::uintptr_t katanaLocal=0;
 const bool localKatana=katanaEnabled.load(std::memory_order_acquire) && localPlayer(katanaLocal) && reinterpret_cast<std::uintptr_t>(entity)==katanaLocal;
 struct KatanaPlayerScope { bool previous; explicit KatanaPlayerScope(bool value):previous(katanaLocalPlayerPass) { katanaLocalPlayerPass=value; } ~KatanaPlayerScope() { katanaLocalPlayerPass=previous; katanaThirdSwordSeen=false; } } katanaPlayerScope{localKatana};
 const auto result=originalPlayer(self,entity,pos,renderValue,partial);
 if (capture) {
  visual_gpu_depth::inPlayerCapture=false; visual_player_skin::end();
  // Keep the actor target alive until the entity pass finishes so the existing
  // world-hat draw can be copied into its own depth buffer. No other actor is
  // captured in the meantime. Outside that pass, publish immediately.
  if (!inEntityPass) visual_player_replay::end();
  captureScope.active=false;
 }
 // Do not clear a local-player frame when another player's renderer runs.
 if (anyVisual()) {
  std::uintptr_t local=0;
  const bool isLocal=localPlayer(local) && reinterpret_cast<std::uintptr_t>(entity)==local;
  if (isLocal) {
   rememberPlayer(entity,partial);
   if (visual_depth_policy::playerHat(strictDepth.load(std::memory_order_acquire),entityPlayerPass,
       inEntityPass,isLocal && enabled.load(std::memory_order_acquire),frame.valid,playerPassAttempted)) {
    playerPassAttempted=true; lastPlayerPassAttempt.store(GetTickCount64(),std::memory_order_relaxed);
    drawHatAtPlayer(frame);
   }
  }
 }
 return result;
}
void renderHat(const PlayerFrame& player);
void renderTrail(const PlayerFrame& player);
void renderJump(const PlayerFrame& player);
void renderAccessory(const PlayerFrame& player);
ID3D11DeviceContext* sameThreadContext() {
 ID3D11DeviceContext* ctx=nullptr;
 AcquireSRWLockShared(&contextLock);
 if (observedContext && observedThread==GetCurrentThreadId()) { ctx=observedContext; ctx->AddRef(); }
 ReleaseSRWLockShared(&contextLock);
 return ctx;
}
bool validDepthCompare(D3D11_COMPARISON_FUNC fn) {
 return fn==D3D11_COMPARISON_LESS || fn==D3D11_COMPARISON_LESS_EQUAL ||
        fn==D3D11_COMPARISON_GREATER || fn==D3D11_COMPARISON_GREATER_EQUAL;
}
// Candidate entity pass, verified against this game's SHA/prologue. World
// depth state was observed bound/enabled by the user; actual content, material
// matrices and transparency must still be tested on Windows.
std::intptr_t entitiesDiagnosticHook(void* self,float partial) {
 // Never recursively start another diagnostic pass or draw another hat.
 if (inEntityPass) return originalEntities(self,partial);
 if (!katanaEnabled.load(std::memory_order_acquire)) katanaQueries.idleReset();
 entityPlayerPass=hatPlayerPass.load(std::memory_order_acquire);
 playerPassAttempted=false;
 inEntityPass=true;
 struct EntityScope { ~EntityScope() { visual_player_replay::end(); inEntityPass=false; } } scope;
 auto result=originalEntities(self,partial);
 inEntityPass=false;
 if (!anyVisual()) return result;
 sampleOutputMerger(depthWorld);
 if (!strictDepth.load(std::memory_order_acquire)) return result;
 auto* ctx=sameThreadContext(); ID3D11DepthStencilView* dsv=nullptr;
 ID3D11DepthStencilState* state=nullptr; UINT ref=0;
 D3D11_COMPARISON_FUNC compare=D3D11_COMPARISON_LESS;
 bool enabledDepth=false;
 if (ctx) {
  ctx->OMGetRenderTargets(0,nullptr,&dsv); ctx->OMGetDepthStencilState(&state,&ref);
  if (state) { D3D11_DEPTH_STENCIL_DESC desc{}; state->GetDesc(&desc); enabledDepth=desc.DepthEnable; compare=desc.DepthFunc; }
  else enabledDepth=true; // D3D11 default null state: depth enabled, LESS.
 }
 worldDepthCompare.store(ctx?static_cast<int>(compare):0,std::memory_order_relaxed);
 const bool worldReady=worldHookReady.load(std::memory_order_acquire);
 const bool gpuReady=gpuHooksReady.load(std::memory_order_acquire) && visual_gpu_depth::matches(ctx);
 if (!visual_depth_policy::worldDraw(true,worldReady,gpuReady,ctx!=nullptr,dsv!=nullptr,enabledDepth && validDepthCompare(compare))) {
  occlusionStatus.store(!worldReady || !gpuReady ? 3 : 2,std::memory_order_relaxed);
 } else {
  PlayerFrame visible=frame; frame.valid=false; PlayerFrame local=visible;
  if ((trailEnabled.load(std::memory_order_acquire) || jumpEnabled.load(std::memory_order_acquire) || accessoryEnabled.load(std::memory_order_acquire) || katanaEnabled.load(std::memory_order_acquire)) && !local.valid) {
   std::uintptr_t actor=0;
   if (localPlayer(actor)) { rememberPlayer(reinterpret_cast<void*>(actor),partial); local=frame; frame.valid=false; }
  }
  visual_gpu_depth::expectedContext=ctx; visual_gpu_depth::expectedDepth=dsv; visual_gpu_depth::worldCompare=compare;
  auto rejected=visual_gpu_depth::rejected.load(std::memory_order_relaxed);
  auto submitted=visual_gpu_depth::submitted.load(std::memory_order_relaxed);
  if (visual_depth_policy::entitiesHat(entityPlayerPass)) {
   const bool previous=visual_gpu_depth::inHatCapture;
   visual_gpu_depth::inHatCapture=previewHatCapture.load(std::memory_order_acquire);
   struct RestoreHatCapture { bool previous; ~RestoreHatCapture() { visual_gpu_depth::inHatCapture=previous; } } restore{previous};
   renderHat(visible);
  }
  renderTrail(local);
  renderJump(local);
  renderAccessory(local);
  renderKatana(local);
  visual_gpu_depth::expectedContext=nullptr; visual_gpu_depth::expectedDepth=nullptr;
  const bool blocked=visual_gpu_depth::rejected.load(std::memory_order_relaxed)!=rejected;
  const bool drew=visual_gpu_depth::submitted.load(std::memory_order_relaxed)!=submitted;
  occlusionStatus.store(blocked?4:drew?1:5,std::memory_order_relaxed);
 }
 if (state) state->Release(); if (dsv) dsv->Release(); if (ctx) ctx->Release();
 return result;
}
// Optional timing comparison: immediately after the *local* player renderer
// returns, while still inside the original entity pass. Uses the same exact
// depth/DSV checks, transparency and GPU state restoration as the world path.
// Its camera/depth compatibility is unverified until the user tests it.
void drawHatAtPlayer(const PlayerFrame& player) {
 auto* ctx=sameThreadContext(); ID3D11DepthStencilView* dsv=nullptr;
 ID3D11DepthStencilState* state=nullptr; UINT ref=0;
 D3D11_COMPARISON_FUNC compare=D3D11_COMPARISON_LESS; bool enabledDepth=false;
 if (ctx) {
  ctx->OMGetRenderTargets(0,nullptr,&dsv); ctx->OMGetDepthStencilState(&state,&ref);
  if (state) { D3D11_DEPTH_STENCIL_DESC desc{}; state->GetDesc(&desc); enabledDepth=desc.DepthEnable; compare=desc.DepthFunc; }
  else enabledDepth=true;
 }
 hatPlayerCompare.store(ctx?static_cast<int>(compare):0,std::memory_order_relaxed);
 const bool worldReady=worldHookReady.load(std::memory_order_acquire);
 const bool gpuReady=gpuHooksReady.load(std::memory_order_acquire) && visual_gpu_depth::matches(ctx);
 if (!visual_depth_policy::worldDraw(true,worldReady,gpuReady,ctx!=nullptr,dsv!=nullptr,enabledDepth && validDepthCompare(compare))) {
  hatPlayerPassStatus.store(!worldReady || !gpuReady ? 3 : 2,std::memory_order_relaxed);
 } else {
  visual_gpu_depth::expectedContext=ctx; visual_gpu_depth::expectedDepth=dsv; visual_gpu_depth::worldCompare=compare;
  const auto rejected=visual_gpu_depth::rejected.load(std::memory_order_relaxed);
  const auto submitted=visual_gpu_depth::submitted.load(std::memory_order_relaxed);
  {
   const bool previous=visual_gpu_depth::inHatCapture;
   visual_gpu_depth::inHatCapture=previewHatCapture.load(std::memory_order_acquire);
   struct RestoreHatCapture { bool previous; ~RestoreHatCapture() { visual_gpu_depth::inHatCapture=previous; } } restore{previous};
   renderHat(player);
  }
  visual_gpu_depth::expectedContext=nullptr; visual_gpu_depth::expectedDepth=nullptr;
  const bool blocked=visual_gpu_depth::rejected.load(std::memory_order_relaxed)!=rejected;
  const bool drew=visual_gpu_depth::submitted.load(std::memory_order_relaxed)!=submitted;
  hatPlayerPassStatus.store(blocked?4:drew?1:5,std::memory_order_relaxed);
 }
 if (state) state->Release(); if (dsv) dsv->Release(); if (ctx) ctx->Release();
}
// Activate D3D guards only while this adapter calls the game's tessDraw. Other
// game and ImGui draws pass through untouched. No write to the scene depth.
void submitVisual(void* tess,float alpha) {
 const bool guarded=visual_gpu_depth::expectedContext!=nullptr;
 struct DrawScope {
  bool active;
  explicit DrawScope(bool a,float opacity):active(a) { if (active) { visual_gpu_depth::opacity=opacity; visual_gpu_depth::inVisualDraw=true; } }
  ~DrawScope() { if (active) visual_gpu_depth::inVisualDraw=false; }
 } scope(guarded,alpha);
 tessDraw(tess,reinterpret_cast<void*>(base+contextRva),0);
}
// Keep the previously user-verified wireframe draw path isolated from the
// experimental trail. Preserve the tested depth, topology and draw sequence.
// The separate opt-in material tint may be ignored by the game's material.
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
 const auto now=GetTickCount64();
 china_hat::Point head{player.world.x-origin[0],player.world.y-origin[1]-1.62f+player.height+0.03f+
   param(VisualParam::HatOffset)+hat_animation::bob(now,param(VisualParam::HatBobAmplitude),param(VisualParam::HatBobSpeed)),player.world.z-origin[2]};
 if (std::fabs(head.x)>16 || std::fabs(head.y)>16 || std::fabs(head.z)>16) return;
 china_hat::Mesh mesh{};
 const bool gradient=materialTint.load(std::memory_order_acquire) && hatRainbow.load(std::memory_order_acquire) && vertexGradient.load(std::memory_order_acquire);
 const float phase=gradient ? visual_gradient::phase(now,param(VisualParam::HatRainbowSpeed)) : static_cast<float>(now%8000)/8000.0f;
 if (!china_hat::build(mesh,head,param(VisualParam::HatRadius),param(VisualParam::HatConeHeight),48,phase,255)) return;
 auto tint=visual_color::animated(materialTint.load(std::memory_order_acquire),hatRainbow.load(std::memory_order_acquire),
    {{param(VisualParam::HatRed),param(VisualParam::HatGreen),param(VisualParam::HatBlue)}},param(VisualParam::HatRainbowSpeed),now);
 if (gradient) tint={{1.f,1.f,1.f,1.f}};
 tint[3]=param(VisualParam::HatOpacity); // The game's material must support alpha/blending.
 std::memcpy(reinterpret_cast<void*>(color),tint.data(),sizeof(float)*tint.size()); *reinterpret_cast<std::uint8_t*>(flag)=1;
 auto* t=reinterpret_cast<void*>(tess);
 // The documented 1.1.5 renderer uses topology 4 for line segments. The first
 // experimental version is a wireframe hat, not a filled/textured cone.
 constexpr int segments=48, spokes=12;
 tessBegin(t,4,2*(segments+spokes));
 auto vertex=[&](const china_hat::Vertex& v) { tessColor(t,v.color.r,v.color.g,v.color.b,v.color.a); tessVertex(t,v.position.x,v.position.y,v.position.z); };
 china_hat::emitWireframe(mesh,spokes,vertex);
 submitVisual(t,param(VisualParam::HatOpacity));
 lastHatSubmit.store(GetTickCount64(),std::memory_order_relaxed);
 hatStatus.store(1,std::memory_order_relaxed);
 // The researched renderer releases its temporary tessellation at this point.
 // A zeroed, aligned output slot is provided as required by the native ABI.
 alignas(16) std::uint8_t scratch[0x200]{};
 tessEnd(t,scratch,0,0);
 if (hatFill.load(std::memory_order_acquire)) {
  if (!tessIdle(tess)) { hatFillStatus.store(2,std::memory_order_relaxed); }
  else {
  // The prior batch may have consumed the game's shader-color dirty flag.
  std::memcpy(reinterpret_cast<void*>(color),tint.data(),sizeof(float)*tint.size()); *reinterpret_cast<std::uint8_t*>(flag)=1;
  // Candidate triangle-list mode, not yet confirmed for the user's build.
  // Keep the proven wireframe as the first, separate batch.
  tessBegin(t,2,static_cast<int>(mesh.triangleVertices*2));
  china_hat::emitDoubleSidedSurface(mesh,vertex);
  submitVisual(t,param(VisualParam::HatOpacity));
  hatFillStatus.store(1,std::memory_order_relaxed);
  alignas(16) std::uint8_t surfaceScratch[0x200]{}; tessEnd(t,surfaceScratch,0,0);
  }
 } else hatFillStatus.store(0,std::memory_order_relaxed);
 std::memcpy(reinterpret_cast<void*>(color),saved,sizeof(saved)); *reinterpret_cast<std::uint8_t*>(flag)=oldFlag;
}

void renderAccessory(const PlayerFrame& player) {
 if (!accessoryEnabled.load(std::memory_order_acquire)) return;
 const ULONGLONG now=GetTickCount64(); std::uintptr_t actor=0;
 if (!player.valid || now<player.at || now-player.at>250 || !localPlayer(actor) || actor!=player.entity) {
  accessoryStatus.store(0,std::memory_order_relaxed); return;
 }
 accessoryStatus.store(2,std::memory_order_relaxed);
 float origin[3]{},saved[4]{}; std::uint8_t oldFlag=0;
 auto tess=base+tessRva,color=base+shaderColorRva,flag=base+shaderFlagRva;
 if (!read(base+originRva,origin,sizeof(origin)) || !read(color,saved,sizeof(saved)) || !fetch(flag,oldFlag) ||
     !writable(color,sizeof(saved)) || !writable(flag,1) || !tessIdle(tess)) return;
 for(float v:origin) if(!std::isfinite(v)) return;
 const china_hat::Point anchor{player.world.x-origin[0],player.world.y-origin[1]-1.62f+player.height+.03f+param(VisualParam::AccessoryOffset),player.world.z-origin[2]};
 const float scale=param(VisualParam::AccessoryScale), opacity=param(VisualParam::AccessoryOpacity);
 // Validate the entire geometry before touching the game's tessellator.
 if (!visual_accessory::crown(anchor,scale,[](china_hat::Point){})) return;
 float tint[4]{param(VisualParam::AccessoryRed),param(VisualParam::AccessoryGreen),param(VisualParam::AccessoryBlue),opacity};
 std::memcpy(reinterpret_cast<void*>(color),tint,sizeof(tint)); *reinterpret_cast<std::uint8_t*>(flag)=1;
 auto* t=reinterpret_cast<void*>(tess); tessBegin(t,2,visual_accessory::vertexCount);
 visual_accessory::crown(anchor,scale,[&](china_hat::Point p) { tessColor(t,255,255,255,255); tessVertex(t,p.x,p.y,p.z); });
 submitVisual(t,opacity);
 alignas(16) std::uint8_t scratch[0x200]{}; tessEnd(t,scratch,0,0);
 std::memcpy(reinterpret_cast<void*>(color),saved,sizeof(saved)); *reinterpret_cast<std::uint8_t*>(flag)=oldFlag;
 lastAccessorySubmit.store(now,std::memory_order_relaxed); accessoryStatus.store(1,std::memory_order_relaxed);
}

// Return addresses, ABI and material are inferred from this exact executable,
// not a stable game API. Hooks are limited to the candidate local sword
// paths below. Mismatches and unavailable graphics state call the original.
bool katanaCodeMatches() {
 struct Sig { std::uintptr_t rva; std::uint8_t bytes[16]; };
 constexpr Sig sigs[]={
  {0x59f300,{0x48,0x8b,0xc4,0x55,0x56,0x57,0x41,0x54,0x41,0x55,0x41,0x56,0x41,0x57,0x48,0x8d}},
  {0x59f200,{0x48,0x83,0xec,0x38,0x80,0xba,0xe8,0,0,0,0,0x4c,0x8b,0xd2,0x74,0x2f}},
  {0x5a1d28,{0xe8,0xd3,0xd5,0xff,0xff,0xe9,0x65,0x04,0,0,0x45,0x38,0xbe,0x54,0x12,0}},
  {0x59fd19,{0xe8,0xe2,0xf4,0xff,0xff,0x90,0x48,0x85,0xff,0x74,0x09,0xc6,0x47,0x18,0x01,0x48}},
  {0x5a22f0,{0x48,0x89,0x5c,0x24,0x08,0x48,0x89,0x54,0x24,0x10,0x55,0x56,0x57,0x48,0x83,0xec}},
  {0x59fcba,{0xe8,0x31,0x26,0,0,0x41,0x80,0xbf,0xe8,0,0,0,0,0x4c,0x8d,0x0d}},
  {0x56cd2a,{0xe8,0xc1,0x55,0x03,0,0x80,0xbb,0xe8,0,0,0,0,0x4c,0x8d,0x0d,0x03}},
  {0x5d1790,{0x40,0x53,0x48,0x83,0xec,0x60,0x80,0xb9,0x26,0x01,0,0,0,0x48,0x8b,0xd9}}
 };
 for(const auto& sig:sigs) { std::uint8_t bytes[16]{}; if(!read(base+sig.rva,bytes,sizeof(bytes)) || std::memcmp(bytes,sig.bytes,sizeof(bytes))) return false; }
 return true;
}
const katana_mesh::View& katanaMesh() {
 static const auto mesh=[] { katana_mesh::View m; m.load(katana_asset::data,sizeof(katana_asset::data)); return m; }();
 return mesh;
}
// Submit one prepared tessellator batch through the same guarded path as the
// proven hat/jump effects, wrapped in an occlusion query when available.
bool submitKatanaBatch(void* tess, float opacity, ID3D11DeviceContext* ctx) {
 const bool guarded=visual_gpu_depth::expectedContext!=nullptr;
 if (guarded) { visual_gpu_depth::opacity=opacity; visual_gpu_depth::inVisualDraw=true; }
 ID3D11Query* query=katanaQueries.wrapBegin(ctx);
 const auto before=visual_gpu_depth::submitted.load(std::memory_order_relaxed);
 tessDraw(tess,reinterpret_cast<void*>(base+contextRva),0);
 if (query) katanaQueries.wrapEnd(ctx,query);
 if (guarded) {
  visual_gpu_depth::inVisualDraw=false;
  if (visual_gpu_depth::submitted.load(std::memory_order_relaxed)==before) return false;
 }
 return true;
}
// Draw the katana through the user-verified tessellator path used by the hat,
// trail and jump rings: same material globals, same guarded depth/blend draw,
// same restore. The model is positioned at the local player's hand from the
// observed look rotation; first person anchors at the camera, third person at
// the player. Nothing here changes the camera, hand or original item.
void renderKatana(const PlayerFrame& player) {
 if (!katanaEnabled.load(std::memory_order_acquire)) return;
 const auto now=GetTickCount64();
 // Item identity windows. First person is verified directly by the outer
 // held-item hook; third person by the streaming observer or the texture
 // fingerprint learned from a verified first-person draw. The one-second
 // bridge only covers the F5 view transition and never lingers.
 const ULONGLONG fpSeen=lastFpSwordSeen.load(std::memory_order_relaxed);
 const ULONGLONG tpSeen=lastTpSwordSeen.load(std::memory_order_relaxed);
 const ULONGLONG swordSeen=fpSeen>tpSeen?fpSeen:tpSeen;
 const bool bridge=fpSeen && now>=fpSeen && now-fpSeen<1000;
 const bool swordRecent=swordSeen && now>=swordSeen && now-swordSeen<=250;
 if (!swordRecent && !bridge) { katanaStatus.store(0,std::memory_order_relaxed); return; }
 const bool viaBridge=!swordRecent && bridge;
 if (viaBridge) katanaEvents.fetch_or(katana_evidence::bridgeWindow,std::memory_order_relaxed);
 if (!player.valid || now<player.at || now-player.at>250) return;
 std::uintptr_t actor=0;
 if (!localPlayer(actor) || actor!=player.entity) return;
 auto memory=[](std::uintptr_t address,void* out,std::size_t n) { return read(address,out,n); };
 katana_rotation::Reading rotation;
 if (!katana_rotation::readRotation(base,clientInstance.load(std::memory_order_acquire),memory,rotation)) {
  katanaStatus.store(13,std::memory_order_relaxed); return;
 }
 katanaEvents.fetch_or(katana_evidence::rotationRead|(rotation.agree?katana_evidence::rotationAgree:0u),std::memory_order_relaxed);
 float yaw=rotation.yaw;
 if (katanaYawSign.load(std::memory_order_relaxed)<0) yaw+=3.14159265f;
 // Sanity-check the yaw convention against the direction the player actually
 // moves; a sustained majority of anti-aligned samples flips it once.
 if (katanaTrack.count>=2) {
  const std::size_t newest=(katanaTrack.head+KatanaTrack::capacity-1)%KatanaTrack::capacity;
  for (std::size_t step=1;step<katanaTrack.count;++step) {
   const std::size_t older=(newest+KatanaTrack::capacity-step)%KatanaTrack::capacity;
   const ULONGLONG span=katanaTrack.t[newest]-katanaTrack.t[older];
   if (span<150) continue;
   const float dx=katanaTrack.x[newest]-katanaTrack.x[older];
   const float dz=katanaTrack.z[newest]-katanaTrack.z[older];
   const float horizontal=std::sqrt(dx*dx+dz*dz);
   if (horizontal<0.25f) break;
   const auto forward=katana_pose::horizontalForward(yaw);
   katanaYawVotes.sample((forward.x*dx+forward.z*dz)/horizontal);
   const int sign=katanaYawVotes.result();
   if (sign!=0 && sign!=katanaYawSign.load(std::memory_order_relaxed)) {
    katanaYawSign.store(sign,std::memory_order_relaxed);
    katanaEvents.fetch_or(katana_evidence::yawFlipped,std::memory_order_relaxed);
    yaw=rotation.yaw+(sign<0?3.14159265f:0.f);
   }
   break;
  }
 }
 float origin[3]{};
 if (!read(base+originRva,origin,sizeof(origin))) { katanaStatus.store(2,std::memory_order_relaxed); return; }
 for (float v:origin) if (!std::isfinite(v)) { katanaStatus.store(2,std::memory_order_relaxed); return; }
 const katana_pose::Vec3 eye{player.world.x,player.world.y,player.world.z};
 katanaFirstPerson=katana_pose::firstPersonView({origin[0],origin[1],origin[2]},eye,katanaFirstPerson);
 katana_pose::Frame frame;
 frame.yaw=yaw; frame.pitchUp=rotation.pitchUp; frame.firstPerson=katanaFirstPerson;
 frame.tilt=param(VisualParam::KatanaTilt); frame.scale=param(VisualParam::KatanaScale);
 const auto look=katana_pose::look(yaw,rotation.pitchUp);
 const auto right=katana_pose::right(yaw);
 const auto forward=katana_pose::horizontalForward(yaw);
 if (katanaFirstPerson) {
  // Grip relative to the first-person camera: right, below, in front.
  frame.anchor=katana_pose::add(katana_pose::add(
   katana_pose::scale(right,param(VisualParam::KatanaFirstSide)),katana_pose::Vec3{0.f,-0.40f,0.f}),
   katana_pose::scale(look,param(VisualParam::KatanaFirstForward)));
 } else {
  // Grip at the player's right hand in world space, converted into the
  // camera-relative space the tessellator pipeline expects.
  const katana_pose::Vec3 hand=katana_pose::add(katana_pose::add(katana_pose::add(
   katana_pose::Vec3{player.world.x,player.world.y-1.62f,player.world.z},
   katana_pose::scale(right,param(VisualParam::KatanaSide))),
   katana_pose::Vec3{0.f,param(VisualParam::KatanaHeight),0.f}),
   katana_pose::scale(forward,param(VisualParam::KatanaForward)));
  frame.anchor=katana_pose::add(hand,katana_pose::Vec3{-origin[0],-origin[1],-origin[2]});
 }
 katana_geometry::Pose pose;
 if (!katana_pose::worldPose(frame,pose)) { katanaStatus.store(2,std::memory_order_relaxed); return; }
 const auto& mesh=katanaMesh();
 if (!mesh.vertices() || !katana_geometry::triangles(mesh,pose,true,[](katana_geometry::ColoredVertex){})) {
  katanaStatus.store(15,std::memory_order_relaxed); return;
 }
 float saved[4]{}; std::uint8_t oldFlag=0;
 const auto color=base+shaderColorRva, flag=base+shaderFlagRva, tess=base+tessRva;
 if (!read(color,saved,sizeof(saved)) || !fetch(flag,oldFlag) || !writable(color,sizeof(saved)) || !writable(flag,1)) {
  katanaStatus.store(2,std::memory_order_relaxed); return;
 }
 auto* ctx=sameThreadContext();
 if (!ctx || !visual_gpu_depth::matches(ctx)) {
  if (ctx) ctx->Release();
  katanaStatus.store(2,std::memory_order_relaxed); return;
 }
 katanaQueries.poll(ctx,now);
 const float opacity=param(VisualParam::KatanaOpacity);
 const float tint[4]={1.f,1.f,1.f,opacity};  // per-vertex colors carry the model colors
 std::memcpy(reinterpret_cast<void*>(color),tint,sizeof(tint));
 *reinterpret_cast<std::uint8_t*>(flag)=1;
 auto* t=reinterpret_cast<void*>(tess);
 static thread_local std::vector<katana_geometry::ColoredVertex> vertices;
 vertices.clear();
 katana_geometry::triangles(mesh,pose,true,[&](const katana_geometry::ColoredVertex& v) { vertices.push_back(v); });
 auto channel=[](float x) { return static_cast<std::uint8_t>(std::clamp(x,0.f,1.f)*255.f+.5f); };
 constexpr std::size_t batchVertices=2048;
 bool drew=false;
 for (std::size_t offset=0;offset<vertices.size();offset+=batchVertices) {
  if (!tessIdle(tess)) break;
  const std::size_t count=std::min(batchVertices,vertices.size()-offset);
  tessBegin(t,2,static_cast<int>(count));
  for (std::size_t i=0;i<count;++i) {
   const auto& v=vertices[offset+i];
   tessColor(t,channel(v.rgba[0]),channel(v.rgba[1]),channel(v.rgba[2]),channel(v.rgba[3]));
   tessVertex(t,v.position.x,v.position.y,v.position.z);
  }
  drew|=submitKatanaBatch(t,opacity,ctx);
  alignas(16) std::uint8_t scratch[0x200]{};
  tessEnd(t,scratch,0,0);
 }
 std::memcpy(reinterpret_cast<void*>(color),saved,sizeof(saved));
 *reinterpret_cast<std::uint8_t*>(flag)=oldFlag;
 ctx->Release();
 if (!drew) { katanaStatus.store(2,std::memory_order_relaxed); return; }
 lastKatanaSubmit.store(now,std::memory_order_relaxed);
 katanaEvents.fetch_or(katana_evidence::worldDraw,std::memory_order_relaxed);
 if (katanaVisibleRecently(now)) katanaStatus.store(14,std::memory_order_relaxed);
 else if (viaBridge && !katanaFirstPerson) katanaStatus.store(12,std::memory_order_relaxed);
 else katanaStatus.store(1,std::memory_order_relaxed);
}
// Pure observer for the streaming geometry preparation call. It never skips,
// prepares or replays anything; it only records whether the local player's
// diamond sword is being prepared, as one of the third-person identity
// sources. Every call always reaches the original function.
std::uintptr_t heldGeometryHook(void* self,void* a2,void* a3,void* stack,int a5) {
 if (katanaEnabled.load(std::memory_order_acquire)) {
  const auto returnAddress=reinterpret_cast<std::uintptr_t>(__builtin_return_address(0));
  const auto rva=returnAddress>=base?returnAddress-base:0;
  auto memory=[](std::uintptr_t address,void* out,std::size_t n) { return read(address,out,n); };
  if (katanaScope && rva==0x59fcbf) {
   if (diamond_sword_candidate::matches(base,reinterpret_cast<std::uintptr_t>(stack),memory)) {
    katanaEvents.fetch_or(katana_evidence::firstStream,std::memory_order_relaxed);
    lastFpSwordSeen.store(GetTickCount64(),std::memory_order_relaxed);
   }
  } else if (katanaLocalPlayerPass) {
   // Any call site inside the local player's renderer scope: the item stack
   // layout is checked strictly, so unrelated calls simply do not match.
   if (diamond_sword_candidate::matches(base,reinterpret_cast<std::uintptr_t>(stack),memory)) {
    katanaEvents.fetch_or(katana_evidence::thirdIdentity|(rva==0x56cd2f?katana_evidence::thirdStream:0u),std::memory_order_relaxed);
    katanaThirdSwordSeen=true;
    lastTpSwordSeen.store(GetTickCount64(),std::memory_order_relaxed);
   }
  }
 }
 return originalHeldGeometry(self,a2,a3,stack,a5);
}

std::uintptr_t heldDrawHook(void* renderer,void* context,std::uint8_t mode) {
 auto returnAddress=reinterpret_cast<std::uintptr_t>(__builtin_return_address(0));
 if (!katanaEnabled.load(std::memory_order_acquire) || !base || returnAddress<base)
  return originalHeldDraw(renderer,context,mode);
 const auto rva=returnAddress-base;
 // First-person cached call: the outer held-item hook has already verified
 // the local player and the diamond sword candidate for this scope.
 const bool firstCall=rva==0x59fd1e && katanaScope;
 // Third-person cached call: recognised by the streaming observer's identity
 // or by the texture fingerprint learned from a verified first-person draw.
 bool thirdCall=false;
 if (rva==0x56cd8e && katanaLocalPlayerPass) {
  katanaTpCallsInPass.fetch_add(1,std::memory_order_relaxed);
  std::uintptr_t seenTexture=0;
  if (fetch(reinterpret_cast<std::uintptr_t>(context)+0x90,seenTexture))
   katanaTpLastTexture.store(seenTexture,std::memory_order_relaxed);
  thirdCall=katanaThirdSwordSeen;
  if (!thirdCall) {
   const auto texture=katanaSwordTexture.load(std::memory_order_relaxed);
   std::uintptr_t candidate=0;
   if (texture && fetch(reinterpret_cast<std::uintptr_t>(context)+0x90,candidate) && candidate==texture)
    thirdCall=true;
  }
  if (thirdCall) {
   lastTpSwordSeen.store(GetTickCount64(),std::memory_order_relaxed);
   katanaEvents.fetch_or(katana_evidence::thirdCached,std::memory_order_relaxed);
  }
 } else if (rva==0x56cd8e) {
  // The same call site outside the local player's scope: other actors, or a
  // scope the observer missed. Counted for diagnostics only.
  katanaTpCallsOutOfPass.fetch_add(1,std::memory_order_relaxed);
 }
 if (!firstCall && !thirdCall)
  return originalHeldDraw(renderer,context,mode);
 katanaEvents.fetch_or(firstCall?katana_evidence::cachedCall:katana_evidence::thirdCached,std::memory_order_relaxed);
 if (firstCall) {
  // Learn the item-context texture pointer for third-person recognition.
  std::uintptr_t texture=0;
  if (fetch(reinterpret_cast<std::uintptr_t>(context)+0x90,texture) && katana_rotation::validPointer(texture)) {
   if (katanaSwordTexture.load(std::memory_order_relaxed)!=texture)
    katanaEvents.fetch_or(katana_evidence::fingerprintLearned,std::memory_order_relaxed);
   katanaSwordTexture.store(texture,std::memory_order_relaxed);
  }
 }
 auto* ctx=sameThreadContext();
 if (!ctx || !visual_gpu_depth::matches(ctx)) {
  if (ctx) ctx->Release();
  return originalHeldDraw(renderer,context,mode);
 }
 // The original item is suppressed only while the replacement is verified
 // visible by an occlusion query that counted real rasterised samples.
 // Otherwise both are drawn and the held item is never hidden.
 if (!katanaVisibleRecently(GetTickCount64())) {
  ctx->Release();
  return originalHeldDraw(renderer,context,mode);
 }
 visual_gpu_depth::katanaSuppress=ctx;
 const auto result=originalHeldDraw(renderer,context,mode);
 visual_gpu_depth::katanaSuppress=nullptr;
 ctx->Release();
 katanaEvents.fetch_or(firstCall?katana_evidence::suppressedFirst:katana_evidence::suppressedThird,std::memory_order_relaxed);
 lastKatanaSuppress.store(GetTickCount64(),std::memory_order_relaxed);
 return result;
}
std::uintptr_t heldRenderHook(void* self,void* actor,void* stack,std::uintptr_t a4,float a5,std::uint8_t a6,std::uint8_t a7) {
 const auto returnAddress=reinterpret_cast<std::uintptr_t>(__builtin_return_address(0));
 std::uintptr_t local=0;
 auto memory=[](std::uintptr_t address,void* out,std::size_t n) { return read(address,out,n); };
 if (katanaScope || !katanaEnabled.load(std::memory_order_acquire) || returnAddress!=base+0x5a1d2d)
  return originalHeldRender(self,actor,stack,a4,a5,a6,a7);
 if (!localPlayer(local) || local!=reinterpret_cast<std::uintptr_t>(actor)) {
  katanaStatus.store(9,std::memory_order_relaxed); return originalHeldRender(self,actor,stack,a4,a5,a6,a7);
 }
 if (!diamond_sword_candidate::matches(base,reinterpret_cast<std::uintptr_t>(stack),memory)) {
  katanaStatus.store(10,std::memory_order_relaxed); return originalHeldRender(self,actor,stack,a4,a5,a6,a7);
 }
 std::uint8_t stackByte=0;
 const bool stackNonzero=fetch(reinterpret_cast<std::uintptr_t>(stack),stackByte) && stackByte!=0;
 katanaEvents.fetch_or(katana_evidence::addFirst(0,stackNonzero),std::memory_order_relaxed);
 lastFpSwordSeen.store(GetTickCount64(),std::memory_order_relaxed);
 katanaStatus.store(11,std::memory_order_relaxed); // Candidate only, not evidence that a later path did not run.
 struct Scope { Scope(){katanaScope=true;} ~Scope(){katanaScope=false;} } scope;
 return originalHeldRender(self,actor,stack,a4,a5,a6,a7);
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
 const auto ttl=static_cast<std::uint64_t>(param(VisualParam::TrailSeconds)*1000.0f);
 trail.update(actor,feet,now,ttl);
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
 const bool gradient=materialTint.load(std::memory_order_acquire) && trailRainbow.load(std::memory_order_acquire) && vertexGradient.load(std::memory_order_acquire);
 const float phase=visual_gradient::phase(now,param(VisualParam::TrailRainbowSpeed));
 for (std::size_t i=0;i<trail.size();++i) {
  const auto& p=trail[i]; auto& v=points[i];
  v.position={p.point.x-origin[0],p.point.y-origin[1],p.point.z-origin[2]};
  valid[i]=std::fabs(v.position.x)<32 && std::fabs(v.position.y)<32 && std::fabs(v.position.z)<32;
  float age=static_cast<float>(now-p.time)/static_cast<float>(ttl);
  const auto alpha=static_cast<std::uint8_t>(255.f*(1.f-std::clamp(age,0.f,1.f)));
  v.color=gradient ? visual_gradient::trail(i,trail.size(),phase,alpha) : china_hat::rainbow(.38f+static_cast<float>(i)*.002f,alpha);
  if (i && valid[i] && valid[i-1]) ++segments;
 }
 if (!segments) return;
 auto tint=visual_color::animated(materialTint.load(std::memory_order_acquire),trailRainbow.load(std::memory_order_acquire),
    {{param(VisualParam::TrailRed),param(VisualParam::TrailGreen),param(VisualParam::TrailBlue)}},param(VisualParam::TrailRainbowSpeed),now);
 if (gradient) tint={{1.f,1.f,1.f,1.f}};
 tint[3]=param(VisualParam::TrailOpacity); // The game's material must support alpha/blending.
 std::memcpy(reinterpret_cast<void*>(color),tint.data(),sizeof(float)*tint.size()); *reinterpret_cast<std::uint8_t*>(flag)=1;
 auto* t=reinterpret_cast<void*>(tess);
 tessBegin(t,4,segments*2);
 auto vertex=[&](const china_hat::Vertex& v) { tessColor(t,v.color.r,v.color.g,v.color.b,v.color.a); tessVertex(t,v.position.x,v.position.y,v.position.z); };
 for (std::size_t i=1;i<trail.size();++i) if (valid[i] && valid[i-1]) { vertex(points[i-1]); vertex(points[i]); }
 submitVisual(t,param(VisualParam::TrailOpacity));
 lastTrailSubmit.store(GetTickCount64(),std::memory_order_relaxed);
 trailStatus.store(1,std::memory_order_relaxed);
 alignas(16) std::uint8_t scratch[0x200]{}; tessEnd(t,scratch,0,0);
 if (trailFill.load(std::memory_order_acquire)) {
  if (!tessIdle(tess)) { trailFillStatus.store(2,std::memory_order_relaxed); }
  else {
  std::memcpy(reinterpret_cast<void*>(color),tint.data(),sizeof(float)*tint.size()); *reinterpret_cast<std::uint8_t*>(flag)=1;
  // 12 vertices per segment: two triangles, each drawn in both directions.
  // Screen/material state and transparency remain controlled by the game.
  float height=std::clamp(player.height*param(VisualParam::TrailHeight),0.4f,2.0f);
  tessBegin(t,2,segments*12);
  for (std::size_t i=1;i<trail.size();++i)
   if (valid[i] && valid[i-1]) visual_ribbon::segment(points[i-1],points[i],height,vertex);
  submitVisual(t,param(VisualParam::TrailOpacity));
  trailFillStatus.store(1,std::memory_order_relaxed);
  alignas(16) std::uint8_t ribbonScratch[0x200]{}; tessEnd(t,ribbonScratch,0,0);
  }
 } else trailFillStatus.store(0,std::memory_order_relaxed);
 std::memcpy(reinterpret_cast<void*>(color),saved,sizeof(saved)); *reinterpret_cast<std::uint8_t*>(flag)=oldFlag;
}
void renderJump(const PlayerFrame& player) {
 const unsigned gen=jumpGeneration.load(std::memory_order_acquire);
 if (gen!=seenJumpGeneration) { jumps.clear(); seenJumpGeneration=gen; }
 if (!jumpEnabled.load(std::memory_order_acquire)) { jumps.clear(); jumpStatus.store(0,std::memory_order_relaxed); return; }
 const ULONGLONG now=GetTickCount64();
 std::uintptr_t actor=0;
 if (!player.valid || now<player.at || now-player.at>250 || !localPlayer(actor) || actor!=player.entity) {
  jumps.clear(); jumpStatus.store(0,std::memory_order_relaxed); return;
 }
 const auto ttl=static_cast<std::uint64_t>(param(VisualParam::JumpSeconds)*1000.f);
 jumps.update(actor,{player.world.x,player.world.y-1.62f+.06f,player.world.z},now,ttl);
 if (!jumps.size()) { jumpStatus.store(0,std::memory_order_relaxed); return; }
 jumpStatus.store(2,std::memory_order_relaxed);
 float origin[3]{},saved[4]{}; std::uint8_t oldFlag=0;
 auto tess=base+tessRva,color=base+shaderColorRva,flag=base+shaderFlagRva;
 if (!read(base+originRva,origin,sizeof(origin)) || !read(color,saved,sizeof(saved)) || !fetch(flag,oldFlag) ||
     !writable(color,sizeof(saved)) || !writable(flag,1)) return;
 for(float v:origin) if(!std::isfinite(v)) return;
 auto* t=reinterpret_cast<void*>(tess);
 for(std::size_t i=0;i<jumps.size();++i) {
  const auto& ring=jumps[i]; const float p=visual_jump::progress(now,ring.start,ttl);
  china_hat::Point c{ring.center.x-origin[0],ring.center.y-origin[1],ring.center.z-origin[2]};
  if(std::fabs(c.x)>32 || std::fabs(c.y)>32 || std::fabs(c.z)>32 || !tessIdle(tess)) continue;
  const float opacity=param(VisualParam::JumpOpacity)*visual_jump::alpha(p);
  auto tint=visual_color::animated(true,jumpRainbow.load(std::memory_order_acquire),
      {{param(VisualParam::JumpRed),param(VisualParam::JumpGreen),param(VisualParam::JumpBlue)}},param(VisualParam::JumpRainbowSpeed),now);
  const float radius=visual_jump::radius(p,param(VisualParam::JumpRadius));
  // Two bounded batches per ring: bright detail first, translucent halo
  // second. Each uses the game's existing material and guarded depth draw;
  // no per-vertex alpha or post-processing is assumed.
  for (int layer=0;layer<2;++layer) {
   if (!tessIdle(tess)) break;
   const float layerOpacity=opacity*(layer==0 ? 1.f : .22f);
   tint[3]=layerOpacity;
   std::memcpy(reinterpret_cast<void*>(color),tint.data(),sizeof(float)*tint.size()); *reinterpret_cast<std::uint8_t*>(flag)=1;
   tessBegin(t,2,layer==0 ? visual_jump::detailVertices : visual_jump::haloVertices);
   auto emit=[&](china_hat::Point v) { tessColor(t,255,255,255,255); tessVertex(t,v.x,v.y,v.z); };
   if (layer==0) visual_jump::detail(c,radius,p,emit);
   else visual_jump::halo(c,radius,emit);
   submitVisual(t,layerOpacity);
   alignas(16) std::uint8_t scratch[0x200]{}; tessEnd(t,scratch,0,0);
   lastJumpSubmit.store(now,std::memory_order_relaxed); jumpStatus.store(1,std::memory_order_relaxed);
  }
 }
 std::memcpy(reinterpret_cast<void*>(color),saved,sizeof(saved)); *reinterpret_cast<std::uint8_t*>(flag)=oldFlag;
}
void screenHook(void* self,void* ctx) {
 if (inScreen) { originalScreen(self,ctx); return; }
 inScreen=true;
 if (anyVisual()) sampleOutputMerger(depthBefore);
 originalScreen(self,ctx);
 if (anyVisual()) sampleOutputMerger(depthAfter);
 if (!visual_depth_policy::overlayDraw(strictDepth.load(std::memory_order_acquire))) { inScreen=false; return; }
 PlayerFrame visible=frame; frame.valid=false;
 // Submit the verified hat before collecting/drawing the experimental trail.
 renderHat(visible);
 PlayerFrame local=visible;
 if ((trailEnabled.load(std::memory_order_acquire) || jumpEnabled.load(std::memory_order_acquire) || accessoryEnabled.load(std::memory_order_acquire)) && !local.valid) {
  std::uintptr_t actor=0;
  if (localPlayer(actor)) { rememberPlayer(reinterpret_cast<void*>(actor),1.0f); local=frame; frame.valid=false; }
 }
 renderTrail(local);
 renderJump(local);
 renderAccessory(local);
 renderKatana(local);
 inScreen=false;
}
}
void ObserveVisualContext(ID3D11DeviceContext* context) {
 if (!context) return;
 context->AddRef();
 AcquireSRWLockExclusive(&contextLock);
 ID3D11DeviceContext* old=observedContext;
 observedContext=context; observedThread=GetCurrentThreadId();
 ReleaseSRWLockExclusive(&contextLock);
 if (old) old->Release();
}
int VisualDepthBefore() { return depthBefore.load(std::memory_order_relaxed); }
int VisualDepthAfter() { return depthAfter.load(std::memory_order_relaxed); }
int VisualDepthWorld() { return depthWorld.load(std::memory_order_relaxed); }
int VisualOcclusionStatus() {
 if (hookState.load(std::memory_order_acquire)==2 &&
     (!worldHookReady.load(std::memory_order_acquire) || !gpuHooksReady.load(std::memory_order_acquire))) return 3;
 return occlusionStatus.load(std::memory_order_relaxed);
}
bool HatPlayerPass() { return hatPlayerPass.load(std::memory_order_acquire); }
void SetHatPlayerPass(bool value) {
 hatPlayerPass.store(value,std::memory_order_release);
 lastPlayerPassAttempt.store(0,std::memory_order_relaxed); hatPlayerPassStatus.store(0,std::memory_order_relaxed);
 hatPlayerCompare.store(0,std::memory_order_relaxed);
}
int HatPlayerDepthCompare() { return hatPlayerCompare.load(std::memory_order_relaxed); }
int WorldDepthCompare() { return worldDepthCompare.load(std::memory_order_relaxed); }
int HatPlayerPassStatus() {
 const auto then=lastPlayerPassAttempt.load(std::memory_order_relaxed),now=GetTickCount64();
 return !then || now<then || now-then>1000 ? 0 : hatPlayerPassStatus.load(std::memory_order_relaxed);
}
bool VisualLegacyMode() { return !strictDepth.load(std::memory_order_acquire); }
void SetVisualLegacyMode(bool value) { strictDepth.store(!value,std::memory_order_release); occlusionStatus.store(0,std::memory_order_relaxed); }
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
 if (success) {
  // Optional diagnostics cannot disable the previously working visual hooks.
  visual_hooks::Hook world[]={{reinterpret_cast<void*>(base+worldEntitiesRva),reinterpret_cast<void*>(&entitiesDiagnosticHook),reinterpret_cast<void**>(&originalEntities)}};
  const bool installed=visual_hooks::install(world,1,createHook,enableHook,disableHook,removeHook);
  worldHookReady.store(installed,std::memory_order_release);
  if (!installed) log("world render unavailable");
  auto* ctx=installed?sameThreadContext():nullptr;
  if (ctx) {
   bool gpu=visual_gpu_depth::install(ctx,createHook,enableHook,disableHook,removeHook);
   gpuHooksReady.store(gpu,std::memory_order_release);
   if (gpu) {
    visual_gpu_depth::playerObserver.store(&visual_player_skin::observe,std::memory_order_release);
    visual_gpu_depth::replayEnd.store(&visual_player_replay::endDraw,std::memory_order_release);
    visual_gpu_depth::replayBegin.store(&visual_player_replay::beginDraw,std::memory_order_release);
   }
   ctx->Release();
   if (!gpu) log("GPU depth guard unavailable; strict visual draw disabled");
  }
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
  enabled.store(false,std::memory_order_release); trailEnabled.store(false,std::memory_order_release); jumpEnabled.store(false,std::memory_order_release); accessoryEnabled.store(false,std::memory_order_release);
  available.store(false,std::memory_order_release); hookState.store(3,std::memory_order_release);
  log("failed to enable; overlay remains available"); return false;
 }
}
}
bool KatanaEnabled() { return katanaEnabled.load(std::memory_order_acquire); }
std::uint32_t KatanaEvidence() { return katanaEvents.load(std::memory_order_relaxed); }
// One read-only diagnostics snapshot for the katana menu page: the live
// rotation reading plus the raw pointer/angle trail and the third-person
// identity counters. Computed on demand; nothing here renders or hooks.
KatanaDiagnostics KatanaDebugSnapshot() {
 KatanaDiagnostics d{};
 const auto instance=clientInstance.load(std::memory_order_acquire);
 auto memory=[](std::uintptr_t address,void* out,std::size_t n) { return read(address,out,n); };
 katana_rotation::Diagnostics raw;
 katana_rotation::snapshot(base,instance,memory,raw);
 katana_rotation::Reading r;
 d.valid=katana_rotation::readRotation(base,instance,memory,r);
 d.yaw=r.yaw; d.pitchUp=r.pitchUp; d.sources=r.sources; d.agree=r.agree; d.wrapped=r.wrapped;
 d.clientInstance=raw.clientInstance; d.player=raw.player; d.rotationHolder=raw.rotationHolder;
 d.rootA=raw.rootA; d.interA=raw.interA; d.holderA=raw.holderA;
 d.rootB=raw.rootB; d.interB=raw.interB; d.holderB=raw.holderB;
 for (int i=0;i<2;++i) {
  d.playerAngles[i]=raw.playerAngles[i]; d.holderAngles[i]=raw.holderAngles[i];
  d.holderAAngles[i]=raw.holderAAngles[i]; d.holderBAngles[i]=raw.holderBAngles[i];
 }
 d.playerHead=raw.playerHead;
 d.playerAnglesRead=raw.playerAnglesRead; d.holderAnglesRead=raw.holderAnglesRead;
 d.holderAAnglesRead=raw.holderAAnglesRead; d.holderBAnglesRead=raw.holderBAnglesRead;
 d.playerHeadRead=raw.playerHeadRead;
 d.tpCallsInPass=katanaTpCallsInPass.load(std::memory_order_relaxed);
 d.tpCallsOutOfPass=katanaTpCallsOutOfPass.load(std::memory_order_relaxed);
 d.tpLastTexture=katanaTpLastTexture.load(std::memory_order_relaxed);
 d.learnedTexture=katanaSwordTexture.load(std::memory_order_relaxed);
 return d;
}
int KatanaStatus() {
 const int status=katanaStatus.load(std::memory_order_relaxed);
 const auto now=GetTickCount64(), then=lastKatanaSubmit.load(std::memory_order_relaxed);
 return (status==1||status==12||status==14) && (!then || now<then || now-then>1000) ? 0 : status;
}
void SetKatanaEnabled(bool value) {
 if (!value) {
  katanaEnabled.store(false,std::memory_order_release);
  katanaStatus.store(0,std::memory_order_relaxed); katanaEvents.store(0,std::memory_order_relaxed);
  lastFpSwordSeen.store(0,std::memory_order_relaxed); lastTpSwordSeen.store(0,std::memory_order_relaxed);
  katanaSwordTexture.store(0,std::memory_order_relaxed);
  katanaThirdSwordSeen=false;
  return;
 }
 katanaEvents.store(0,std::memory_order_relaxed); katanaStatus.store(0,std::memory_order_relaxed);
 if (!enableVisual()) { katanaStatus.store(3,std::memory_order_relaxed); return; }
 if (!gpuHooksReady.load(std::memory_order_acquire)) { katanaStatus.store(4,std::memory_order_relaxed); return; }
 int state=katanaHookState.load(std::memory_order_acquire);
 if (state==0 && katanaHookState.compare_exchange_strong(state,1,std::memory_order_acq_rel)) {
  int failure=0;
  if (!validateGame(true)) failure=5;
  else if (!katanaCodeMatches()) failure=6;
  else if (katanaMesh().vertices()!=4572 || katanaMesh().indices()!=7866) failure=7;
  else {
   visual_hooks::Hook hooks[]={{reinterpret_cast<void*>(base+0x59f300),reinterpret_cast<void*>(&heldRenderHook),reinterpret_cast<void**>(&originalHeldRender)},
                               {reinterpret_cast<void*>(base+0x59f200),reinterpret_cast<void*>(&heldDrawHook),reinterpret_cast<void**>(&originalHeldDraw)},
                               {reinterpret_cast<void*>(base+0x5a22f0),reinterpret_cast<void*>(&heldGeometryHook),reinterpret_cast<void**>(&originalHeldGeometry)}};
   if (!visual_hooks::install(hooks,3,createHook,enableHook,disableHook,removeHook)) failure=8;
  }
  if (failure) katanaHookFailure.store(failure,std::memory_order_relaxed);
  katanaHookState.store(failure?3:2,std::memory_order_release); state=failure?3:2;
 }
 if(state!=2) { katanaStatus.store(state==3?katanaHookFailure.load(std::memory_order_relaxed):8,std::memory_order_relaxed); return; }
 katanaEnabled.store(true,std::memory_order_release);
}

void SetPlayerSkinPreviewActive(bool value) {
 if (!value) { visual_player_skin::setRequested(false); return; }
 if (visual_player_skin::isRequested()) return;
 if (enableVisual() && gpuHooksReady.load(std::memory_order_acquire)) visual_player_skin::setRequested(true);
}
void SetPlayerPreviewHatActive(bool value) { previewHatCapture.store(value,std::memory_order_release); }
void SetPlayerModelPreviewActive(bool value) {
 if (!value) { visual_player_replay::setRequested(false); return; }
 if (visual_player_replay::isRequested()) return;
 if (enableVisual() && gpuHooksReady.load(std::memory_order_acquire)) visual_player_replay::setRequested(true);
}
void SetChinaHatEnabled(bool value) {
 if (!value) { enabled.store(false,std::memory_order_release); hatStatus.store(0,std::memory_order_relaxed); return; }
 hatStatus.store(0,std::memory_order_relaxed);
 if (enableVisual()) enabled.store(true,std::memory_order_release);
}
bool HatFillEnabled() { return hatFill.load(std::memory_order_acquire); }
void SetHatFillEnabled(bool value) { hatFill.store(value,std::memory_order_release); hatFillStatus.store(0,std::memory_order_relaxed); }
bool TrailFillEnabled() { return trailFill.load(std::memory_order_acquire); }
void SetTrailFillEnabled(bool value) { trailFill.store(value,std::memory_order_release); trailFillStatus.store(0,std::memory_order_relaxed); }
bool VisualTintEnabled() { return materialTint.load(std::memory_order_acquire); }
void SetVisualTintEnabled(bool value) { materialTint.store(value,std::memory_order_release); }
bool AccessoryEnabled() { return accessoryEnabled.load(std::memory_order_acquire); }
void SetAccessoryEnabled(bool value) {
 if (!value) { accessoryEnabled.store(false,std::memory_order_release); accessoryStatus.store(0,std::memory_order_relaxed); return; }
 accessoryStatus.store(0,std::memory_order_relaxed);
 if (enableVisual()) accessoryEnabled.store(true,std::memory_order_release);
}
int AccessoryStatus() {
 int status=accessoryStatus.load(std::memory_order_relaxed); auto then=lastAccessorySubmit.load(std::memory_order_relaxed),now=GetTickCount64();
 return status==1 && (!then || now<then || now-then>1000) ? 0 : status;
}
bool JumpEnabled() { return jumpEnabled.load(std::memory_order_acquire); }
void SetJumpEnabled(bool value) {
 if (!value) { jumpEnabled.store(false,std::memory_order_release); jumpGeneration.fetch_add(1,std::memory_order_acq_rel); jumpStatus.store(0,std::memory_order_relaxed); return; }
 jumpStatus.store(0,std::memory_order_relaxed);
 if (enableVisual()) jumpEnabled.store(true,std::memory_order_release);
}
bool JumpRainbowEnabled() { return jumpRainbow.load(std::memory_order_acquire); }
void SetJumpRainbowEnabled(bool value) { jumpRainbow.store(value,std::memory_order_release); }
int JumpStatus() {
 int status=jumpStatus.load(std::memory_order_relaxed); auto then=lastJumpSubmit.load(std::memory_order_relaxed), now=GetTickCount64();
 return status==1 && (!then || now<then || now-then>1000) ? 0 : status;
}
bool TrailEnabled() { return trailEnabled.load(std::memory_order_acquire); }
void SetTrailEnabled(bool value) {
 if (!value) { trailEnabled.store(false,std::memory_order_release); trailGeneration.fetch_add(1,std::memory_order_acq_rel); trailStatus.store(0,std::memory_order_relaxed); return; }
 trailStatus.store(0,std::memory_order_relaxed);
 if (enableVisual()) trailEnabled.store(true,std::memory_order_release);
}
float GetVisualParam(VisualParam p) {
 auto i=static_cast<std::size_t>(p); return i<static_cast<std::size_t>(VisualParam::Count)?params.values[i].load(std::memory_order_acquire):0.f;
}
void SetVisualParam(VisualParam p,float value) {
 auto i=static_cast<std::size_t>(p); if (i<static_cast<std::size_t>(VisualParam::Count)) params.values[i].store(sanitizeVisualParam(p,value),std::memory_order_release);
}
float ChinaHatHeight() { return GetVisualParam(VisualParam::HatOffset); }
void SetChinaHatHeight(float value) { SetVisualParam(VisualParam::HatOffset,value); }
bool HatRainbowEnabled() { return hatRainbow.load(std::memory_order_acquire); }
void SetHatRainbowEnabled(bool value) { hatRainbow.store(value,std::memory_order_release); }
bool TrailRainbowEnabled() { return trailRainbow.load(std::memory_order_acquire); }
void SetTrailRainbowEnabled(bool value) { trailRainbow.store(value,std::memory_order_release); }
bool VertexGradientEnabled() { return vertexGradient.load(std::memory_order_acquire); }
void SetVertexGradientEnabled(bool value) { vertexGradient.store(value,std::memory_order_release); }
int HatFillStatus() { return hatFillStatus.load(std::memory_order_relaxed); }
int TrailFillStatus() { return trailFillStatus.load(std::memory_order_relaxed); }
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
