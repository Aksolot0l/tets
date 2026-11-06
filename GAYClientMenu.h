// GAYClientMenu.h — визуальная часть внутриигрового меню GAYClient.
// C++17 / Dear ImGui 1.91.9b / DirectX 11.
//
// Этот заголовок НЕ создаёт ImGui-контекст и не подключается к Minecraft.
// Другой разработчик вызывает gayclient::RenderMenu(state, actions) внутри
// уже существующего кадра ImGui, передаёт реальные значения в MenuState и
// получает изменения через MenuActions.
#pragma once

#include <functional>
#include <string>
#include <vector>

#include "imgui.h"

namespace gayclient
{
    // ─────────────────────────────────────────────────────────────────────────
    // Идентификаторы разделов
    // ─────────────────────────────────────────────────────────────────────────
    enum class Tab
    {
        Settings,
        Mods,
        Profiles,
        Statistics
    };

    enum class Section
    {
        Home,
        Hud,
        Interface,
        Graphics,
        Sound,
        Controls,
        Other
    };

    enum class ModuleId
    {
        Fps,
        Cps,
        Clock,
        Keystrokes,
        Watermark,
        Coordinates,
        Performance,
        Resolution,
        ModuleList,
        ChinaHat,
        Trail,
        JumpRings,
        // Adapter-only test modules. Added by the client integration: the
        // entries live in the runtime module list, the icon falls back to
        // Glyph::Dots and EnabledFlag returns nullptr (the client owns the
        // real state through moduleToggled).
        Katana,
        Crown
    };

    inline constexpr const char* kPositions[] = {
        "Сверху слева",
        "Сверху справа",
        "Снизу слева",
        "Снизу справа",
        "Собственная позиция"
    };
    inline constexpr int kPositionCount = IM_ARRAYSIZE(kPositions);

    inline constexpr const char* kPositionShort[] = { "СЛ", "СП", "НЛ", "НП", "Своя" };

    inline constexpr const char* kThemes[] = { "Aogiri Green", "Emerald", "Graphite" };
    inline constexpr int kThemeCount = IM_ARRAYSIZE(kThemes);
    inline const ImVec4 kThemeAccents[] = {
        ImVec4(0.22f, 1.00f, 0.49f, 1.00f), // Aogiri Green #38FF7D
        ImVec4(0.17f, 0.89f, 0.69f, 1.00f), // Emerald      #2BE3B0
        ImVec4(0.71f, 0.76f, 0.72f, 1.00f)  // Graphite     #B4C1B7
    };

    inline constexpr const char* kModuleCategories[] = { "Все", "HUD", "Графика" };
    inline constexpr int kModuleCategoryCount = IM_ARRAYSIZE(kModuleCategories);

    // ─────────────────────────────────────────────────────────────────────────
    // Настройки HUD
    // ─────────────────────────────────────────────────────────────────────────
    struct HudModuleListSettings
    {
        bool enabled = true;
        float scale = 1.0f;
        float backgroundOpacity = 42.0f;
        ImVec4 textColor = ImVec4(0.72f, 1.00f, 0.80f, 1.00f);
        int position = 1;
    };

    struct HudSettings
    {
        bool showFps = true;
        bool showCps = true;
        bool showKeystrokes = false;
        bool showClock = true;
        bool showWatermark = false;
        bool showCoordinates = false;
        bool showPerformance = false;
        bool showResolution = false;
        bool showModuleList = true;
        bool showPing = false;

        float scale = 1.0f;
        float elementSize = 1.0f;
        float opacity = 88.0f;       // проценты
        float spacing = 6.0f;        // пиксели
        float padding = 8.0f;        // пиксели
        float cardRounding = 10.0f;  // пиксели
        int position = 0;            // индекс kPositions
        ImVec4 textColor = ImVec4(0.72f, 1.00f, 0.80f, 1.00f);

        bool clockSeconds = false;
        int clockFormat = 0;         // 0 — 24 часа, 1 — 12 часов
        bool compactPerformance = true;

        HudModuleListSettings moduleList;
    };

    // ─────────────────────────────────────────────────────────────────────────
    // Настройки интерфейса
    // ─────────────────────────────────────────────────────────────────────────
    struct InterfaceSettings
    {
        int theme = 0;
        ImVec4 accent = ImVec4(0.22f, 1.00f, 0.49f, 1.00f);
        ImVec4 textColor = ImVec4(0.94f, 0.99f, 0.95f, 1.00f);
        ImVec4 backgroundColor = ImVec4(0.016f, 0.063f, 0.035f, 1.00f);

        float menuScale = 100.0f;   // проценты, не влияет на ImGui-контекст
        float brightness = 100.0f;  // проценты
        float opacity = 92.0f;      // проценты
        float rounding = 16.0f;     // пиксели

        bool glow = true;
        float glowIntensity = 55.0f; // проценты
        bool animations = true;
        float animationSpeed = 55.0f; // проценты
    };

    // ─────────────────────────────────────────────────────────────────────────
    // Эффекты графики
    // ─────────────────────────────────────────────────────────────────────────
    struct ChinaHatSettings
    {
        bool enabled = false;
        bool fill = true;
        ImVec4 color = ImVec4(0.29f, 1.00f, 0.54f, 1.00f);
        bool rainbow = false;
        float rainbowSpeed = 45.0f;
        float opacity = 72.0f;
        float radius = 0.85f;
        float height = 0.65f;
        float offset = 0.18f;
        float floatAmplitude = 0.08f;
        float floatSpeed = 1.2f;
    };

    struct TrailSettings
    {
        bool enabled = false;
        bool fill = true;
        ImVec4 color = ImVec4(0.28f, 1.00f, 0.69f, 1.00f);
        bool rainbow = false;
        float rainbowSpeed = 40.0f;
        float opacity = 65.0f;
        float height = 0.8f;
        float life = 1.2f;
    };

    struct JumpRingsSettings
    {
        bool enabled = false;
        ImVec4 color = ImVec4(0.49f, 1.00f, 0.49f, 1.00f);
        bool rainbow = false;
        float rainbowSpeed = 50.0f;
        float radius = 0.75f;
        float opacity = 70.0f;
        float duration = 0.8f;
    };

    struct GraphicsSettings
    {
        ChinaHatSettings chinaHat;
        TrailSettings trail;
        JumpRingsSettings jumpRings;
    };

    struct SoundSettings
    {
        bool muted = false;
        float master = 70.0f;
        float ui = 60.0f;
        float effects = 75.0f;
        int device = 0;
    };

    struct ControlsSettings
    {
        std::string menuKey = "Right Shift";
        bool moduleHotkeys = true;
        bool cursorVisible = true;
        bool scrollEnabled = true;
    };

    struct Profile
    {
        std::string name;
        std::string note;
        bool active = false;
    };

    struct ModuleEntry
    {
        ModuleId id = ModuleId::Fps;
        std::string name;
        std::string category;
        std::string description;
        bool enabled = false;
    };

    // Изображение/текстура предпросмотра передаётся клиентом (ImTextureID).
    // Пока texture == 0, панель показывает заглушку «Предпросмотр недоступен».
    struct PreviewAsset
    {
        ImTextureID texture = 0;
        ImVec2 textureSize = ImVec2(0.0f, 0.0f);
        std::string label = "Текстура не передана";
    };

    // ─────────────────────────────────────────────────────────────────────────
    // Состояние меню
    // ─────────────────────────────────────────────────────────────────────────
    struct MenuState
    {
        bool visible = true;

        Tab tab = Tab::Settings;
        Section section = Section::Hud;

        HudSettings hud;
        InterfaceSettings ui;
        GraphicsSettings graphics;
        SoundSettings sound;
        ControlsSettings controls;

        // Демонстрационные профили. Реальное хранение — на стороне клиента.
        std::vector<Profile> profiles = {
            { "Aogiri - базовый", "Демонстрационный профиль по умолчанию", true },
            { "Компактный HUD", "Пример профиля для PvP-раскладки", false },
            { "Кинематографичный", "Пример профиля с крупным HUD", false }
        };
        int selectedProfile = 0;

        std::vector<ModuleEntry> modules = {
            { ModuleId::Fps, "FPS", "HUD", "Счётчик кадров в игровом HUD", true },
            { ModuleId::Cps, "CPS", "HUD", "Счётчик кликов в секунду", true },
            { ModuleId::Clock, "Clock", "HUD", "Часы в выбранном формате", true },
            { ModuleId::Keystrokes, "Keystrokes", "HUD", "Отображение нажатых клавиш", false },
            { ModuleId::Watermark, "Watermark", "HUD", "Водяной знак клиента", false },
            { ModuleId::Coordinates, "Coordinates", "HUD", "Координаты игрока — данные клиента", false },
            { ModuleId::Performance, "CPU / RAM", "HUD", "Производительность системы — нет данных", false },
            { ModuleId::Resolution, "Resolution", "HUD", "Разрешение игрового окна", false },
            { ModuleId::ModuleList, "Список модулей", "HUD", "Активные модули отдельной карточкой", true },
            { ModuleId::ChinaHat, "China Hat", "Графика", "Конус над головой персонажа", false },
            { ModuleId::Trail, "Trail", "Графика", "След за движением персонажа", false },
            { ModuleId::JumpRings, "Jump Rings", "Графика", "Кольца при прыжке", false }
        };
        int selectedModule = 0;
        int moduleCategory = 0;
        char moduleSearch[64] = {};

        bool effectOpen[3] = { true, false, false };

        PreviewAsset preview;
        bool cyrillicFontLoaded = false;
        bool dataConnected = false; // клиент ещё не передал реальные данные

        // Удобные вспомогательные функции для интеграции.
        Profile* ActiveProfile()
        {
            for (Profile& profile : profiles)
            {
                if (profile.active)
                    return &profile;
            }
            return profiles.empty() ? nullptr : &profiles.front();
        }

        ModuleEntry* SelectedModule()
        {
            if (selectedModule < 0 || selectedModule >= static_cast<int>(modules.size()))
                return modules.empty() ? nullptr : &modules.front();
            return &modules[static_cast<std::size_t>(selectedModule)];
        }
    };

    // ─────────────────────────────────────────────────────────────────────────
    // Действия. Меню никогда не пишет в игру напрямую — только вызывает эти
    // колбэки. Все они опциональны: проверяйте std::function перед вызовом.
    // ─────────────────────────────────────────────────────────────────────────
    struct MenuActions
    {
        std::function<void(Tab)> tabChanged;
        std::function<void(Section)> sectionChanged;

        // Универсальное уведомление об изменении значения: путь в MenuState,
        // например "hud.showFps" или "graphics.chinaHat.color".
        std::function<void(const char* path)> settingChanged;

        std::function<void(ModuleId, bool)> moduleToggled;

        std::function<void(int)> profileSelected;
        std::function<void(int)> profileActivated;
        std::function<void(const char*)> profileCreated;
        std::function<void(int, const char*)> profileRenamed;
        std::function<void(int)> profileDeleted;
        std::function<void(int)> profileSaveRequested;

        std::function<void()> closeRequested;
        std::function<void()> hudEditorRequested;
        std::function<void()> previewTextureRequested;

        std::function<void()> resetHudRequested;
        std::function<void()> resetInterfaceRequested;
        std::function<void()> resetGraphicsRequested;
        std::function<void()> resetSoundRequested;
        std::function<void()> resetControlsRequested;
        std::function<void()> resetAllRequested;
        std::function<void()> statsRefreshRequested;
    };

    // ─────────────────────────────────────────────────────────────────────────
    // Публичный API визуального слоя
    // ─────────────────────────────────────────────────────────────────────────

    // Загружает системный шрифт с кириллицей. Вызвать ОДИН РАЗ до первой
    // отрисовки, используя уже существующий ImGuiIO/ImGui-контекст.
    // Возвращает false, если TTF не найден (тогда используется встроенный шрифт).
    bool LoadCyrillicFont(ImGuiIO& io, float pixelSize = 17.0f);

    // Применяет цвета/скругления к текущему стилю ImGui.
    // Вызывается внутри RenderMenu; отдельно вызывать не обязательно.
    void ApplyMenuStyle(const MenuState& state, float scale);

    // Отрисовывает ЕДИНСТВЕННОЕ окно меню. Контекст ImGui и NewFrame/Render
    // остаются за вызывающим кодом. Возвращает false, если меню скрыто.
    bool RenderMenu(MenuState& state, const MenuActions& actions);

    // Синхронизирует флаги modules[] с реальными полями MenuState.
    // Полезно после программного изменения настроек клиентом.
    void SyncModuleStates(MenuState& state);

    // Утилита для клиента: включает/выключает модуль по идентификатору.
    bool SetModuleEnabled(MenuState& state, ModuleId id, bool enabled);

} // namespace gayclient
