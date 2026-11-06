cmake_minimum_required(VERSION 3.16)

project(GAYClientMenuDemo LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ─────────────────────────────────────────────────────────────────────────────
# Dear ImGui 1.91.9b
# ─────────────────────────────────────────────────────────────────────────────
include(FetchContent)

FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG        v1.91.9b
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(imgui)

# ─────────────────────────────────────────────────────────────────────────────
# Демонстрационное приложение: GAYClientMenu + пустой чёрный кадр DX11
# ─────────────────────────────────────────────────────────────────────────────
add_executable(gayclient_menu WIN32
    main.cpp
    GAYClientMenu.cpp
    GAYClientMenu.h

    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/imgui_demo.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_win32.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_dx11.cpp
)

target_include_directories(gayclient_menu PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
)

target_compile_definitions(gayclient_menu PRIVATE
    WIN32_LEAN_AND_MEAN
    NOMINMAX
    UNICODE
    _UNICODE
)

target_link_libraries(gayclient_menu PRIVATE
    d3d11
    dxgi
    d3dcompiler
)

if (MSVC)
    # /utf-8 обязателен: исходники содержат русские строковые литералы.
    target_compile_options(gayclient_menu PRIVATE /utf-8 /W4 /permissive-)
    target_link_options(gayclient_menu PRIVATE /ENTRY:mainCRTStartup)
endif()

# Визуальная часть как отдельная библиотека для встраивания в клиент.
# Клиент может добавить только GAYClientMenu.cpp/.h и линковать свой imgui/DX11.
add_library(gayclient_menu_ui STATIC
    GAYClientMenu.cpp
    GAYClientMenu.h

    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
)

target_include_directories(gayclient_menu_ui PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${imgui_SOURCE_DIR}
)

target_compile_definitions(gayclient_menu_ui PUBLIC
    WIN32_LEAN_AND_MEAN
    NOMINMAX
)

if (MSVC)
    target_compile_options(gayclient_menu_ui PRIVATE /utf-8 /W4 /permissive-)
endif()
