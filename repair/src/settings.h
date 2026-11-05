#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace ag {

inline constexpr const char* kVersion = "0.3.5-mouse-test";

enum class ModuleId : std::size_t {
    Fps, Cps, Clock, Session, Date, SystemTime, Cpu, Ram, Resolution,
    WindowTitle, Keystrokes, Watermark, Information, Notifications,
    ActiveModules, Hotkeys, TimeIndicator, Coordinates, Count
};
inline constexpr std::size_t kModuleCount = static_cast<std::size_t>(ModuleId::Count);
constexpr bool ModuleAvailable(ModuleId id) {
    return static_cast<std::size_t>(id) < kModuleCount && id != ModuleId::Session && id != ModuleId::Date &&
           id != ModuleId::SystemTime && id != ModuleId::WindowTitle && id != ModuleId::TimeIndicator;
}
inline constexpr std::size_t kPageCount = 4;
inline constexpr std::size_t kPanelCount = 6;
enum class PanelId : std::size_t { Home, Hud, Modules, Appearance, Layout, Settings };
inline constexpr std::array<std::size_t,5> kDisplayedPanelIds{{0,1,2,3,5}};

struct ModuleInfo {
    ModuleId id;
    const char* name;
    const char* category;
    const char* description;
    bool defaultEnabled;
};
extern const std::array<ModuleInfo, kModuleCount> kModules;

enum class EffectId : std::size_t {
    Fade, Slide, Glow, Hover, Borders, Particles, ColorPulse,
    Notifications, ButtonPress, Count
};
inline constexpr std::size_t kEffectCount = static_cast<std::size_t>(EffectId::Count);
struct EffectInfo { EffectId id; const char* name; const char* description; };
extern const std::array<EffectInfo, kEffectCount> kEffects;

struct ThemePreset {
    const char* name;
    std::array<float, 4> accent;
    std::array<float, 4> text;
    std::array<float, 4> background;
    std::array<float, 4> panel;
};
inline constexpr std::size_t kThemeCount = 6;
extern const std::array<ThemePreset, kThemeCount> kThemes;

struct KeyChoice { int vk; const char* name; };
extern const std::array<KeyChoice, 20> kKeyChoices;
bool IsBindableKey(int key, bool allowNone = true);
const char* KeyName(int key);

struct Settings {
    std::array<bool, kModuleCount> enabled{};
    std::array<int, kModuleCount> keys{};
    std::array<int, kPageCount> navigationOrder{{0,1,2,3}};
    std::array<bool, kPanelCount> panelVisible{};
    std::array<bool, kPanelCount> panelExpanded{};
    int menuKey = 0xA1;
    int panicKey = 0x2E;
    int theme = 0;
    std::array<float, 4> accent{};
    std::array<float, 4> text{};
    std::array<float, 4> background{};
    float opacity = 0.94f;
    float glow = 0.65f;
    float rounding = 10.0f;
    float uiScale = 1.0f;
    float hudScale = 1.0f;
    float brightness = 1.0f;
    float animationSpeed = 1.0f;
    int refreshHz = 60;
    bool effectsEnabled = true;
    std::array<bool, kEffectCount> effects{};
    float effectIntensity = 0.80f;
    float keystrokeScale = 1.0f;
    std::array<float, 4> keyActive{};
    std::array<float, 3> coordinates{};
    int coordinatePrecision = 1;
    bool coordinateLabels = true;

    Settings();
    bool IsEnabled(ModuleId id) const { return ModuleAvailable(id) && enabled[static_cast<std::size_t>(id)]; }
    bool Effect(EffectId id) const { return effectsEnabled && effects[static_cast<std::size_t>(id)]; }
    void SelectTheme(int index);
    bool ReorderNavigation(int movedPage, int targetPage);
    void Normalize();
    std::size_t EnabledCount() const;
};

} // namespace ag
