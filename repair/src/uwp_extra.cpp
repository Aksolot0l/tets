// Additional UWP input collected on the same dispatcher thread as the legacy
// CoreWindow bridge. Interface order follows Windows SDK / windows-rs metadata.
#include "uwp_extra.h"
#include "pointer_mapping.h"
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <roapi.h>
#include <winstring.h>
#include <inspectable.h>
#include <atomic>
#include <new>

struct UwpPoint { float x, y; };
struct UwpRect { float x, y, w, h; };
struct CoreWindow : IInspectable {
    virtual HRESULT STDMETHODCALLTYPE unusedAutomation() = 0;
    virtual HRESULT STDMETHODCALLTYPE get_Bounds(UwpRect*) = 0;
    virtual HRESULT STDMETHODCALLTYPE unusedProperties() = 0;
    virtual HRESULT STDMETHODCALLTYPE unusedDispatcher() = 0;
    virtual HRESULT STDMETHODCALLTYPE unusedFlow() = 0;
    virtual HRESULT STDMETHODCALLTYPE unusedSetFlow() = 0;
    virtual HRESULT STDMETHODCALLTYPE unusedInput() = 0;
    virtual HRESULT STDMETHODCALLTYPE unusedSetInput() = 0;
    virtual HRESULT STDMETHODCALLTYPE unusedCursor() = 0;
    virtual HRESULT STDMETHODCALLTYPE unusedSetCursor() = 0;
    virtual HRESULT STDMETHODCALLTYPE get_PointerPosition(UwpPoint*) = 0;
    virtual HRESULT STDMETHODCALLTYPE unusedVisible() = 0;
    virtual HRESULT STDMETHODCALLTYPE unusedActivate() = 0;
    virtual HRESULT STDMETHODCALLTYPE unusedClose() = 0;
    virtual HRESULT STDMETHODCALLTYPE unusedAsyncKey() = 0;
    virtual HRESULT STDMETHODCALLTYPE unusedKey() = 0;
    virtual HRESULT STDMETHODCALLTYPE unusedReleaseCapture() = 0;
    virtual HRESULT STDMETHODCALLTYPE unusedSetCapture() = 0;
#define UNUSED_PAIR(n) virtual HRESULT STDMETHODCALLTYPE unused##n() = 0; virtual HRESULT STDMETHODCALLTYPE unusedRemove##n() = 0;
    UNUSED_PAIR(Activated) UNUSED_PAIR(AutomationEvent) UNUSED_PAIR(Character) UNUSED_PAIR(Closed)
    UNUSED_PAIR(InputEnabled) UNUSED_PAIR(KeyDown) UNUSED_PAIR(KeyUp) UNUSED_PAIR(CaptureLost)
    UNUSED_PAIR(PointerEntered) UNUSED_PAIR(PointerExited) UNUSED_PAIR(PointerMoved)
    UNUSED_PAIR(PointerPressed) UNUSED_PAIR(PointerReleased) UNUSED_PAIR(TouchHitTesting)
#undef UNUSED_PAIR
    virtual HRESULT STDMETHODCALLTYPE add_PointerWheelChanged(IUnknown*, INT64*) = 0;
};
struct CoreWindowStatics : IInspectable {
    virtual HRESULT STDMETHODCALLTYPE GetForCurrentThread(CoreWindow**) = 0;
};
struct PointerArgs : IInspectable {
    virtual HRESULT STDMETHODCALLTYPE get_CurrentPoint(IInspectable**) = 0;
};
struct PointerPoint : IInspectable {
    virtual HRESULT STDMETHODCALLTYPE unusedDevice() = 0;
    virtual HRESULT STDMETHODCALLTYPE get_Position(UwpPoint*) = 0;
    virtual HRESULT STDMETHODCALLTYPE unusedRawPosition() = 0;
    virtual HRESULT STDMETHODCALLTYPE unusedId() = 0;
    virtual HRESULT STDMETHODCALLTYPE unusedFrame() = 0;
    virtual HRESULT STDMETHODCALLTYPE unusedTimestamp() = 0;
    virtual HRESULT STDMETHODCALLTYPE unusedContact() = 0;
    virtual HRESULT STDMETHODCALLTYPE get_Properties(IInspectable**) = 0;
};
struct PointerProperties : IInspectable {
#define UNUSED(n) virtual HRESULT STDMETHODCALLTYPE unused##n() = 0;
    UNUSED(0) UNUSED(1) UNUSED(2) UNUSED(3) UNUSED(4) UNUSED(5) UNUSED(6)
    UNUSED(7) UNUSED(8) UNUSED(9) UNUSED(10) UNUSED(11) UNUSED(12)
#undef UNUSED
    virtual HRESULT STDMETHODCALLTYPE get_MouseWheelDelta(INT32*) = 0;
    virtual HRESULT STDMETHODCALLTYPE get_IsHorizontalMouseWheel(boolean*) = 0;
};
struct PointerHandler : IUnknown {
    virtual HRESULT STDMETHODCALLTYPE Invoke(IInspectable*, IInspectable*) = 0;
};

namespace {
constexpr GUID iidStatics = {0x4d239005,0x3c2a,0x41b1,{0x90,0x22,0x53,0x6b,0xb9,0xcf,0x93,0xb1}};
constexpr GUID iidArgs = {0x920d9cb1,0xa5fc,0x4a21,{0x8c,0x09,0x49,0xdf,0xe6,0xff,0xe2,0x5f}};
constexpr GUID iidPoint = {0xe995317d,0x7296,0x42d9,{0x82,0x33,0xc5,0xbe,0x73,0xb7,0x4a,0x4a}};
constexpr GUID iidProperties = {0xc79d8a4b,0xc163,0x4ee7,{0x80,0x3f,0x67,0xce,0x79,0xf9,0x97,0x2d}};
constexpr GUID iidHandler = {0x420e1bb6,0xe99d,0x5e64,{0x8e,0x25,0x07,0x46,0x7e,0x3c,0xae,0x9e}};
SRWLOCK sampleLock = SRWLOCK_INIT;
UwpPoint pointer{};
UwpRect bounds{};
bool haveSample = false;
std::atomic<int> wheelX{0}, wheelY{0};
std::atomic<bool> inputEnabled{true};
bool attached = false;
ULONGLONG nextAttempt = 0;

template<class T> void release(T*& p) { if (p) { p->Release(); p = nullptr; } }
class WheelHandler final : public PointerHandler {
    std::atomic<ULONG> refs{1};
public:
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, void** out) override {
        if (!out) return E_POINTER;
        *out = nullptr;
        if (iid == IID_IUnknown || iid == IID_IAgileObject || iid == iidHandler) {
            *out = static_cast<PointerHandler*>(this); AddRef(); return S_OK;
        }
        return E_NOINTERFACE;
    }
    ULONG STDMETHODCALLTYPE AddRef() override { return ++refs; }
    ULONG STDMETHODCALLTYPE Release() override { ULONG count = --refs; if (!count) delete this; return count; }
    HRESULT STDMETHODCALLTYPE Invoke(IInspectable*, IInspectable* arg) override {
        if (!arg || !inputEnabled.load(std::memory_order_acquire)) return S_OK;
        PointerArgs* args = nullptr; IInspectable* rawPoint = nullptr; PointerPoint* point = nullptr;
        IInspectable* rawProperties = nullptr; PointerProperties* properties = nullptr;
        HRESULT hr = arg->QueryInterface(iidArgs, reinterpret_cast<void**>(&args));
        if (SUCCEEDED(hr) && args) hr = args->get_CurrentPoint(&rawPoint);
        if (SUCCEEDED(hr) && rawPoint) hr = rawPoint->QueryInterface(iidPoint, reinterpret_cast<void**>(&point));
        if (SUCCEEDED(hr) && point) hr = point->get_Properties(&rawProperties);
        if (SUCCEEDED(hr) && rawProperties) hr = rawProperties->QueryInterface(iidProperties, reinterpret_cast<void**>(&properties));
        if (SUCCEEDED(hr) && properties) {
            INT32 delta = 0; boolean horizontal = 0;
            if (SUCCEEDED(properties->get_MouseWheelDelta(&delta)) &&
                SUCCEEDED(properties->get_IsHorizontalMouseWheel(&horizontal))) {
                if (delta > 12000) delta = 12000;
                if (delta < -12000) delta = -12000;
                (horizontal ? wheelX : wheelY).fetch_add(delta, std::memory_order_relaxed);
            }
        }
        release(properties); release(rawProperties); release(point); release(rawPoint); release(args);
        return S_OK;
    }
};
}

void SampleUwpInput() {
    if (!inputEnabled.load(std::memory_order_acquire)) return;
    HSTRING className = nullptr;
    CoreWindowStatics* statics = nullptr;
    CoreWindow* window = nullptr;
    constexpr wchar_t name[] = L"Windows.UI.Core.CoreWindow";
    HRESULT hr = WindowsCreateString(name, static_cast<UINT32>(sizeof(name)/sizeof(name[0])-1), &className);
    if (SUCCEEDED(hr)) hr = RoGetActivationFactory(className, iidStatics, reinterpret_cast<void**>(&statics));
    if (SUCCEEDED(hr) && statics) hr = statics->GetForCurrentThread(&window);
    if (SUCCEEDED(hr) && window) {
        UwpPoint p{}; UwpRect r{};
        if (SUCCEEDED(window->get_PointerPosition(&p)) && SUCCEEDED(window->get_Bounds(&r)) && r.w > 0 && r.h > 0) {
            AcquireSRWLockExclusive(&sampleLock);
            pointer = p; bounds = r; haveSample = true;
            ReleaseSRWLockExclusive(&sampleLock);
        }
        ULONGLONG now = GetTickCount64();
        if (!attached && now >= nextAttempt) {
            nextAttempt = now + 2000;
            WheelHandler* handler = new (std::nothrow) WheelHandler;
            if (handler) {
                INT64 token = 0;
                attached = SUCCEEDED(window->add_PointerWheelChanged(handler, &token));
                handler->Release();
                if (attached) OutputDebugStringA("GAYClient repair: UWP mouse wheel handler attached\n");
            }
        }
    }
    release(window); release(statics);
    if (className) WindowsDeleteString(className);
}
bool GetSampledUwpPointer(float& x, float& y, float& w, float& h) {
    AcquireSRWLockShared(&sampleLock);
    bool valid = haveSample && uwp_client_pointer(pointer.x, pointer.y, bounds.x, bounds.y,
                                                   bounds.w, bounds.h, x, y, w, h);
    ReleaseSRWLockShared(&sampleLock);
    return valid;
}
void DisableUwpExtraInput() {
    inputEnabled.store(false, std::memory_order_release);
    wheelX.store(0, std::memory_order_relaxed); wheelY.store(0, std::memory_order_relaxed);
}
void TakeUwpWheel(int& horizontal, int& vertical) {
    horizontal = wheelX.exchange(0, std::memory_order_relaxed);
    vertical = wheelY.exchange(0, std::memory_order_relaxed);
}
