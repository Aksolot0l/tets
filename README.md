# GAYClient - визуальное меню (C++17 / Dear ImGui 1.91.9b / DirectX 11)

Нативный исходный код визуальной части внутриигрового меню GAYClient.
Это **не** веб-прототип и не отдельный лаунчер: интерфейс рисуется внутри
существующего ImGui-кадра, а игровой кадр демонстрации остаётся чёрным.

Меню не содержит маскота, хуков, чтения памяти, игровой логики и рендера
эффектов. Все значения по умолчанию - демонстрационные.

Верхние вкладки убраны. Разделы "Моды", "Профили" и "Статистика" перенесены
в левую навигацию. В отрисовке все длинные тире заменяются на "-", символ "·"
удаляется. У окна добавлены угловые скобки, штриховой орнамент, декоративное
свечение области предпросмотра и засечки.

## Файлы

| Файл | Назначение |
|---|---|
| `GAYClientMenu.h` | `MenuState`, `MenuActions`, публичный API, структуры HUD/графики/звука/профилей |
| `GAYClientMenu.cpp` | Вся визуальная реализация: единое окно, кастомные тумблеры, ползунки, вкладки, карточки, предпросмотр |
| `main.cpp` | Демонстрационный запуск DirectX 11: создаёт окно 1680×1000 и вызывает меню внутри кадра ImGui |
| `CMakeLists.txt` | Сборка, FetchContent Dear ImGui `v1.91.9b`, DX11-бэкенд |

## Сборка демонстрации (Windows)

Требования: Windows 10, Visual Studio 2022 с рабочей нагрузкой «Разработка
классических приложений на C++», CMake 3.16+ и доступ в сеть для загрузки ImGui.

```bat
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
build\Release\gayclient_menu.exe
```

При сборке используется `/utf-8` (MSVC), поэтому русские строковые литералы
читаются корректно. Шрифт с кириллицей подхватывается из `C:/Windows/Fonts`
(`segoeui.ttf`, `tahoma.ttf` или `arial.ttf`); если файл не найден, используется
встроенный шрифт ImGui.

## Подключение к существующему клиенту

`RenderMenu` не создаёт `ImGuiContext`, не создаёт `ID3D11Device` и не вызывает
`NewFrame`/`Render`. Достаточно один раз загрузить кириллицу и затем каждый кадр
вызывать меню:

```cpp
#include "GAYClientMenu.h"

gayclient::MenuState  g_menuState;
gayclient::MenuActions g_menuActions;

// 1) Один раз после ImGui::CreateContext(), до инициализации бэкенда.
g_menuState.cyrillicFontLoaded = gayclient::LoadCyrillicFont(ImGui::GetIO(), 17.0f);

// 2) Один раз — привязка изменений к существующему клиенту.
g_menuActions.settingChanged = [](const char* path) {
    // path: "hud.showFps", "graphics.chinaHat.color", "ui.accent" и т.п.
};
g_menuActions.hudEditorRequested = [] { /* открыть существующий редактор HUD */ };
g_menuActions.resetHudRequested  = [] { /* сбросить реальные настройки HUD */ };
g_menuActions.moduleToggled = [](gayclient::ModuleId id, bool enabled) { /* ... */ };

// 3) Каждый кадр после вашего ImGui::NewFrame().
gayclient::RenderMenu(g_menuState, g_menuActions);

// 4) После — ваш ImGui::Render() и игровой рендер. Меню рисуется как обычное окно ImGui.
```

Меню автоматически центрируется, масштабируется под размер игрового окна,
ограничено разумным максимумом и не растягивается на весь экран. Содержимое
левой панели прокручивается внутри одного окна.

### Передача текстуры предпросмотра

Клиент сам создаёт и владеет текстурой. Меню только рисует её:

```cpp
g_menuState.preview.texture = (ImTextureID)myShaderResourceView; // SRV DirectX 11
g_menuState.preview.textureSize = ImVec2(width, height);
g_menuState.preview.label = "Текстура из клиента";

// Убрать изображение и вернуть заглушку:
g_menuState.preview.texture = 0;
g_menuState.preview.label = "Текстура не передана";
```

Пока `texture == 0`, панель показывает «Предпросмотр недоступен» и не
имитирует игровую модель.

### Что можно менять программно

- `MenuState::tab` и `MenuState::section` — активные вкладки;
- `MenuState::hud`, `ui`, `graphics`, `sound`, `controls` — значения настроек;
- `MenuState::modules` и `SyncModuleStates(state)` — связь со списком модулей;
- `SetModuleEnabled(state, id, bool)` — программное включение/выключение модуля;
- `MenuState::visible` — показать/скрыть меню.

Флаги модулей синхронизируются с реальными полями состояния перед каждым кадром
через `SyncModuleStates`; меню не хранит второй независимый набор.

### Колбэки

Основные: `settingChanged`, `moduleToggled`, `sectionChanged`, `tabChanged`,
`hudEditorRequested`, `previewTextureRequested`, `closeRequested`,
`profileSelected/Activated/Created/Renamed/Deleted/SaveRequested`,
`resetHudRequested`, `resetInterfaceRequested`, `resetGraphicsRequested`,
`resetSoundRequested`, `resetControlsRequested`, `resetAllRequested`,
`statsRefreshRequested`.

Клавиша `Right Shift` указана в интерфейсе как назначение по умолчанию, но
здесь **не перехватывается**: фактическое открытие меню, ввод и игровая мышь
остаются задачей существующего клиента.

## Данные и границы

- FPS, CPS и системные показатели помечены как «Нет данных»;
- предпросмотр — условная схема, а не реальный игровой экран;
- профили меняются только в памяти демонстрации;
- отсутствуют хуки, чтение памяти, игровой рендер и логика модулей.

## Отличие от файла `index.html`

`index.html` — вспомогательный браузерный предпросмотр композиции. Маскот из
него удалён. Итоговым требованием задания является именно C++-реализация в
`GAYClientMenu.h/.cpp`; HTML не используется клиентом.
