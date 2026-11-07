// GAYClientMenu.cpp — визуальная реализация меню GAYClient.
// Dear ImGui 1.91.9b. Контекст ImGui не создаётся.
#include "GAYClientMenu.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <unordered_map>

namespace gayclient
{
namespace
{
    // ─────────────────────────────────────────────────────────────────────────
    // Константы и мелкие утилиты
    // ─────────────────────────────────────────────────────────────────────────
    // Адаптер: канвас уменьшен в полтора раза (было 1560x870, пропорции
    // сохранены), чтобы на игровом экране ~1080p масштаб был около 1.5x и
    // текст оставался читаемым; весь остальной макет не менялся.
    constexpr float kShellWidth = 1040.0f;
    constexpr float kShellHeight = 580.0f;
    constexpr float kRailWidth = 268.0f;
    constexpr float kTopbarHeight = 54.0f;
    constexpr float kFooterHeight = 34.0f;
    constexpr float kColumnGap = 13.0f;
    constexpr float kContentRatio = 0.485f;
    constexpr float kPi = 3.14159265358979323846f;

    std::unordered_map<ImGuiID, float> g_toggleAnimations;

    inline ImU32 Color(float r, float g, float b, float a = 1.0f)
    {
        return ImGui::ColorConvertFloat4ToU32(ImVec4(r, g, b, a));
    }

    inline ImU32 ToColor(const ImVec4& color, float alpha = 1.0f)
    {
        ImVec4 copy = color;
        copy.w *= alpha;
        return ImGui::ColorConvertFloat4ToU32(copy);
    }

    inline ImU32 Accent(const MenuState& state, float alpha = 1.0f)
    {
        return ToColor(state.ui.accent, alpha);
    }

    inline ImU32 WithAlpha(ImU32 color, float alpha)
    {
        alpha = std::clamp(alpha, 0.0f, 1.0f);
        return (color & ~IM_COL32_A_MASK) | (static_cast<ImU32>(alpha * 255.0f) << IM_COL32_A_SHIFT);
    }

    inline ImVec4 Mix(const ImVec4& a, const ImVec4& b, float t)
    {
        t = std::clamp(t, 0.0f, 1.0f);
        return ImVec4(a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t,
                      a.z + (b.z - a.z) * t, a.w + (b.w - a.w) * t);
    }

    inline ImVec2 V2(float x, float y) { return ImVec2(x, y); }

    inline void Notify(const MenuActions& actions, const char* path)
    {
        if (actions.settingChanged)
            actions.settingChanged(path);
    }

    inline bool HasTexture(const MenuState& state)
    {
        return state.preview.texture != 0;
    }

    float AnimationLerp(float current, float target, float speed)
    {
        const float dt = std::min(ImGui::GetIO().DeltaTime, 0.1f);
        const float t = std::clamp(dt * speed, 0.0f, 1.0f);
        return current + (target - current) * t;
    }

    // ─────────────────────────────────────────────────────────────────────────
    // Пиктограммы, нарисованные примитивами ImDrawList (без иконочного шрифта)
    // ─────────────────────────────────────────────────────────────────────────
    enum class Glyph
    {
        Home, Monitor, Palette, Graphics, Volume, Sliders, Dots,
        Cube, User, Chart, Shield, Sparkles, Eye, Image, Info,
        Activity, Clock, Cpu, Layers, Wifi, Grid, Droplet,
        Search, Plus, Reset, Trash, Pencil, Save, Folder,
        Keyboard, Lock, Mouse, Check, Warning, Bolt, Arrow, Minus, Cross
    };

    void DrawGlyph(ImDrawList* draw, Glyph glyph, ImVec2 center, float box, ImU32 color)
    {
        const float h = box * 0.5f;
        const ImVec2 c = center;
        const float thickness = std::max(1.0f, box * 0.09f);
        auto line = [&](float x1, float y1, float x2, float y2)
        {
            draw->AddLine(ImVec2(c.x + x1 * h, c.y + y1 * h), ImVec2(c.x + x2 * h, c.y + y2 * h), color, thickness);
        };
        auto rect = [&](float x1, float y1, float x2, float y2, float rounding)
        {
            draw->AddRect(ImVec2(c.x + x1 * h, c.y + y1 * h), ImVec2(c.x + x2 * h, c.y + y2 * h), color, rounding * h, 0, thickness);
        };
        auto circle = [&](float x, float y, float radius, bool filled)
        {
            const ImVec2 p(c.x + x * h, c.y + y * h);
            if (filled)
                draw->AddCircleFilled(p, radius * h, color, 20);
            else
                draw->AddCircle(p, radius * h, color, 20, thickness);
        };

        switch (glyph)
        {
        case Glyph::Home:
            line(-0.85f, -0.05f, 0.0f, -0.8f);
            line(0.0f, -0.8f, 0.85f, -0.05f);
            line(-0.62f, -0.2f, -0.62f, 0.82f);
            line(0.62f, -0.2f, 0.62f, 0.82f);
            line(-0.62f, 0.82f, 0.62f, 0.82f);
            line(-0.18f, 0.82f, -0.18f, 0.28f);
            line(-0.18f, 0.28f, 0.18f, 0.28f);
            line(0.18f, 0.28f, 0.18f, 0.82f);
            break;
        case Glyph::Monitor:
            rect(-0.9f, -0.8f, 0.9f, 0.42f, 0.12f);
            line(0.0f, 0.42f, 0.0f, 0.82f);
            line(-0.4f, 0.82f, 0.4f, 0.82f);
            break;
        case Glyph::Palette:
            circle(0.0f, 0.0f, 0.8f, false);
            circle(-0.38f, -0.18f, 0.12f, true);
            circle(0.02f, -0.42f, 0.12f, true);
            circle(0.4f, -0.05f, 0.12f, true);
            circle(-0.05f, 0.28f, 0.12f, true);
            break;
        case Glyph::Graphics:
        case Glyph::Image:
            rect(-0.88f, -0.72f, 0.88f, 0.72f, 0.14f);
            circle(-0.45f, -0.3f, 0.13f, true);
            line(-0.7f, 0.5f, -0.1f, -0.1f);
            line(-0.1f, -0.1f, 0.25f, 0.24f);
            line(0.25f, 0.24f, 0.75f, -0.28f);
            break;
        case Glyph::Volume:
            draw->AddTriangleFilled(ImVec2(c.x - 0.8f * h, c.y - 0.22f * h), ImVec2(c.x - 0.8f * h, c.y + 0.22f * h),
                                    ImVec2(c.x - 0.15f * h, c.y + 0.65f * h), color);
            draw->AddTriangleFilled(ImVec2(c.x - 0.8f * h, c.y - 0.22f * h), ImVec2(c.x - 0.15f * h, c.y + 0.65f * h),
                                    ImVec2(c.x - 0.15f * h, c.y - 0.65f * h), color);
            draw->PathClear();
            draw->PathArcTo(ImVec2(c.x - 0.15f * h, c.y), 0.55f * h, -0.8f, 0.8f, 12);
            draw->PathStroke(color, 0, thickness);
            draw->PathClear();
            draw->PathArcTo(ImVec2(c.x - 0.15f * h, c.y), 0.86f * h, -0.8f, 0.8f, 14);
            draw->PathStroke(color, 0, thickness);
            break;
        case Glyph::Sliders:
            line(-0.85f, -0.5f, 0.85f, -0.5f);
            line(-0.85f, 0.1f, 0.85f, 0.1f);
            line(-0.85f, 0.7f, 0.85f, 0.7f);
            circle(-0.3f, -0.5f, 0.17f, true);
            circle(0.35f, 0.1f, 0.17f, true);
            circle(-0.05f, 0.7f, 0.17f, true);
            break;
        case Glyph::Dots:
            circle(-0.55f, 0.0f, 0.14f, true);
            circle(0.0f, 0.0f, 0.14f, true);
            circle(0.55f, 0.0f, 0.14f, true);
            break;
        case Glyph::Cube:
            line(-0.8f, -0.25f, 0.0f, -0.75f);
            line(0.0f, -0.75f, 0.8f, -0.25f);
            line(0.8f, -0.25f, 0.0f, 0.25f);
            line(0.0f, 0.25f, -0.8f, -0.25f);
            line(-0.8f, -0.25f, -0.8f, 0.45f);
            line(-0.8f, 0.45f, 0.0f, 0.9f);
            line(0.0f, 0.9f, 0.8f, 0.45f);
            line(0.8f, 0.45f, 0.8f, -0.25f);
            line(0.0f, 0.25f, 0.0f, 0.9f);
            break;
        case Glyph::User:
            circle(0.0f, -0.38f, 0.36f, false);
            draw->PathClear();
            draw->PathArcTo(ImVec2(c.x, c.y + 0.5f * h), 0.72f * h, kPi * 1.15f, kPi * 1.85f, 18);
            draw->PathStroke(color, 0, thickness);
            break;
        case Glyph::Chart:
            line(-0.7f, 0.8f, 0.8f, 0.8f);
            line(-0.35f, 0.8f, -0.35f, 0.05f);
            line(0.05f, 0.8f, 0.05f, -0.45f);
            line(0.45f, 0.8f, 0.45f, 0.25f);
            line(-0.7f, 0.8f, -0.7f, 0.5f);
            break;
        case Glyph::Shield:
            line(0.0f, -0.82f, 0.72f, -0.5f);
            line(0.72f, -0.5f, 0.6f, 0.35f);
            line(0.6f, 0.35f, 0.0f, 0.85f);
            line(0.0f, 0.85f, -0.6f, 0.35f);
            line(-0.6f, 0.35f, -0.72f, -0.5f);
            line(-0.72f, -0.5f, 0.0f, -0.82f);
            line(-0.32f, 0.05f, -0.05f, 0.32f);
            line(-0.05f, 0.32f, 0.4f, -0.25f);
            break;
        case Glyph::Sparkles:
            line(-0.35f, -0.8f, -0.35f, -0.2f);
            line(-0.65f, -0.5f, -0.05f, -0.5f);
            line(0.4f, 0.05f, 0.4f, 0.65f);
            line(0.1f, 0.35f, 0.7f, 0.35f);
            line(-0.5f, 0.45f, -0.5f, 0.8f);
            line(-0.65f, 0.62f, -0.35f, 0.62f);
            break;
        case Glyph::Eye:
            draw->PathClear();
            draw->PathBezierQuadraticCurveTo(ImVec2(c.x, c.y + 0.5f * h), ImVec2(c.x + 0.9f * h, c.y), 18);
            draw->PathBezierQuadraticCurveTo(ImVec2(c.x, c.y - 0.5f * h), ImVec2(c.x - 0.9f * h, c.y), 18);
            draw->PathStroke(color, 0, thickness);
            circle(0.0f, 0.0f, 0.25f, true);
            break;
        case Glyph::Info:
            circle(0.0f, 0.0f, 0.82f, false);
            circle(0.0f, -0.36f, 0.1f, true);
            line(0.0f, -0.02f, 0.0f, 0.45f);
            break;
        case Glyph::Activity:
            line(-0.85f, 0.0f, -0.35f, 0.0f);
            line(-0.35f, 0.0f, -0.1f, -0.7f);
            line(-0.1f, -0.7f, 0.2f, 0.65f);
            line(0.2f, 0.65f, 0.42f, 0.0f);
            line(0.42f, 0.0f, 0.85f, 0.0f);
            break;
        case Glyph::Clock:
            circle(0.0f, 0.0f, 0.8f, false);
            line(0.0f, -0.45f, 0.0f, 0.0f);
            line(0.0f, 0.0f, 0.36f, 0.2f);
            break;
        case Glyph::Cpu:
            rect(-0.55f, -0.55f, 0.55f, 0.55f, 0.1f);
            rect(-0.2f, -0.2f, 0.2f, 0.2f, 0.04f);
            line(-0.55f, -0.3f, -0.82f, -0.3f);
            line(-0.55f, 0.3f, -0.82f, 0.3f);
            line(0.55f, -0.3f, 0.82f, -0.3f);
            line(0.55f, 0.3f, 0.82f, 0.3f);
            line(-0.3f, -0.55f, -0.3f, -0.82f);
            line(0.3f, -0.55f, 0.3f, -0.82f);
            line(-0.3f, 0.55f, -0.3f, 0.82f);
            line(0.3f, 0.55f, 0.3f, 0.82f);
            break;
        case Glyph::Layers:
            line(-0.8f, -0.25f, 0.0f, -0.7f);
            line(0.0f, -0.7f, 0.8f, -0.25f);
            line(0.8f, -0.25f, 0.0f, 0.2f);
            line(0.0f, 0.2f, -0.8f, -0.25f);
            line(-0.8f, 0.2f, 0.0f, 0.65f);
            line(0.0f, 0.65f, 0.8f, 0.2f);
            break;
        case Glyph::Wifi:
            draw->PathClear();
            draw->PathArcTo(ImVec2(c.x, c.y + 0.55f * h), 0.85f * h, kPi * 1.25f, kPi * 1.75f, 14);
            draw->PathStroke(color, 0, thickness);
            draw->PathClear();
            draw->PathArcTo(ImVec2(c.x, c.y + 0.55f * h), 0.55f * h, kPi * 1.25f, kPi * 1.75f, 12);
            draw->PathStroke(color, 0, thickness);
            circle(0.0f, 0.45f, 0.11f, true);
            break;
        case Glyph::Grid:
            rect(-0.8f, -0.8f, -0.08f, -0.08f, 0.08f);
            rect(0.08f, -0.8f, 0.8f, -0.08f, 0.08f);
            rect(-0.8f, 0.08f, -0.08f, 0.8f, 0.08f);
            rect(0.08f, 0.08f, 0.8f, 0.8f, 0.08f);
            break;
        case Glyph::Droplet:
            line(0.0f, -0.85f, 0.0f, -0.55f);
            draw->PathClear();
            draw->PathBezierQuadraticCurveTo(ImVec2(c.x + 0.78f * h, c.y + 0.25f * h), ImVec2(c.x, c.y + 0.78f * h), 18);
            draw->PathBezierQuadraticCurveTo(ImVec2(c.x - 0.78f * h, c.y + 0.25f * h), ImVec2(c.x, c.y - 0.55f * h), 18);
            draw->PathStroke(color, 0, thickness);
            break;
        case Glyph::Search:
            circle(-0.12f, -0.12f, 0.6f, false);
            line(0.35f, 0.35f, 0.82f, 0.82f);
            break;
        case Glyph::Plus:
            line(-0.6f, 0.0f, 0.6f, 0.0f);
            line(0.0f, -0.6f, 0.0f, 0.6f);
            break;
        case Glyph::Reset:
            draw->PathClear();
            draw->PathArcTo(ImVec2(c.x, c.y), 0.62f * h, -2.5f, 1.4f, 20);
            draw->PathStroke(color, 0, thickness);
            draw->AddTriangleFilled(ImVec2(c.x + 0.7f * h, c.y - 0.45f * h), ImVec2(c.x + 0.2f * h, c.y - 0.35f * h),
                                    ImVec2(c.x + 0.55f * h, c.y + 0.1f * h), color);
            break;
        case Glyph::Trash:
            line(-0.7f, -0.55f, 0.7f, -0.55f);
            line(-0.3f, -0.55f, -0.3f, -0.8f);
            line(-0.3f, -0.8f, 0.3f, -0.8f);
            line(0.3f, -0.8f, 0.3f, -0.55f);
            line(-0.55f, -0.55f, -0.42f, 0.78f);
            line(0.55f, -0.55f, 0.42f, 0.78f);
            line(-0.42f, 0.78f, 0.42f, 0.78f);
            line(-0.18f, -0.2f, -0.12f, 0.4f);
            line(0.18f, -0.2f, 0.12f, 0.4f);
            break;
        case Glyph::Pencil:
            line(-0.6f, 0.6f, 0.45f, -0.55f);
            line(0.45f, -0.55f, 0.72f, -0.28f);
            line(0.72f, -0.28f, -0.32f, 0.83f);
            line(-0.32f, 0.83f, -0.72f, 0.78f);
            line(-0.72f, 0.78f, -0.6f, 0.6f);
            break;
        case Glyph::Save:
            rect(-0.7f, -0.75f, 0.7f, 0.75f, 0.12f);
            rect(-0.3f, -0.75f, 0.35f, -0.25f, 0.05f);
            rect(-0.35f, 0.2f, 0.35f, 0.75f, 0.05f);
            break;
        case Glyph::Folder:
            line(-0.85f, -0.45f, -0.2f, -0.45f);
            line(-0.2f, -0.45f, 0.0f, -0.7f);
            line(0.0f, -0.7f, 0.85f, -0.7f);
            line(0.85f, -0.7f, 0.85f, 0.65f);
            line(0.85f, 0.65f, -0.85f, 0.65f);
            line(-0.85f, 0.65f, -0.85f, -0.45f);
            break;
        case Glyph::Keyboard:
            rect(-0.88f, -0.55f, 0.88f, 0.5f, 0.12f);
            circle(-0.55f, -0.22f, 0.07f, true);
            circle(-0.2f, -0.22f, 0.07f, true);
            circle(0.15f, -0.22f, 0.07f, true);
            circle(0.5f, -0.22f, 0.07f, true);
            line(-0.4f, 0.2f, 0.4f, 0.2f);
            break;
        case Glyph::Lock:
            rect(-0.65f, -0.05f, 0.65f, 0.75f, 0.12f);
            draw->PathClear();
            draw->PathArcTo(ImVec2(c.x, c.y - 0.05f * h), 0.42f * h, kPi, kPi * 2.0f, 14);
            draw->PathStroke(color, 0, thickness);
            break;
        case Glyph::Mouse:
            rect(-0.4f, -0.8f, 0.4f, 0.8f, 0.4f);
            line(0.0f, -0.8f, 0.0f, -0.2f);
            break;
        case Glyph::Check:
            line(-0.65f, 0.05f, -0.15f, 0.55f);
            line(-0.15f, 0.55f, 0.7f, -0.5f);
            break;
        case Glyph::Warning:
            line(0.0f, -0.78f, 0.8f, 0.7f);
            line(0.8f, 0.7f, -0.8f, 0.7f);
            line(-0.8f, 0.7f, 0.0f, -0.78f);
            line(0.0f, -0.25f, 0.0f, 0.28f);
            circle(0.0f, 0.5f, 0.08f, true);
            break;
        case Glyph::Bolt:
            line(0.18f, -0.85f, -0.45f, 0.08f);
            line(-0.45f, 0.08f, 0.02f, 0.08f);
            line(0.02f, 0.08f, -0.12f, 0.85f);
            line(-0.12f, 0.85f, 0.5f, -0.12f);
            line(0.5f, -0.12f, 0.05f, -0.12f);
            line(0.05f, -0.12f, 0.18f, -0.85f);
            break;
        case Glyph::Arrow:
            line(-0.55f, 0.0f, 0.5f, 0.0f);
            line(0.5f, 0.0f, 0.12f, -0.38f);
            line(0.5f, 0.0f, 0.12f, 0.38f);
            break;
        case Glyph::Minus:
            line(-0.62f, 0.0f, 0.62f, 0.0f);
            break;
        case Glyph::Cross:
            line(-0.55f, -0.55f, 0.55f, 0.55f);
            line(0.55f, -0.55f, -0.55f, 0.55f);
            break;
        }
    }

    // ─────────────────────────────────────────────────────────────────────────
    // Стиль ImGui
    // ─────────────────────────────────────────────────────────────────────────
    // ─────────────────────────────────────────────────────────────────────────
    // Стиль ImGui, применяемый к визуальной части
    // ─────────────────────────────────────────────────────────────────────────
    void ApplyStyleInternal(const MenuState& st, float s)
    {
        ImGuiStyle& style = ImGui::GetStyle();
        style.WindowPadding = ImVec2(0, 0);
        style.WindowRounding = 22 * s;
        style.WindowBorderSize = 0;
        style.ChildRounding = 12 * s;
        style.ChildBorderSize = 1.0f;
        style.PopupRounding = 10 * s;
        style.PopupBorderSize = 1.0f;
        style.FrameRounding = 8 * s;
        style.FrameBorderSize = 1.0f;
        style.GrabRounding = 6 * s;
        style.ItemSpacing = ImVec2(8 * s, 6 * s);
        style.ItemInnerSpacing = ImVec2(6 * s, 4 * s);
        style.FramePadding = ImVec2(10 * s, 6 * s);
        style.ScrollbarSize = 7 * s;
        style.ScrollbarRounding = 6 * s;
        style.GrabMinSize = 12 * s;

        const ImVec4 accent = st.ui.accent;
        const ImVec4 panelBg(0.020f, 0.070f, 0.038f, 0.96f);
        const ImVec4 panelHover = Mix(panelBg, accent, 0.10f);

        style.Colors[ImGuiCol_Text] = st.ui.textColor;
        style.Colors[ImGuiCol_TextDisabled] = ImVec4(0.44f, 0.57f, 0.49f, 1.0f);
        style.Colors[ImGuiCol_WindowBg] = ImVec4(0.006f, 0.024f, 0.012f, 1.0f);
        style.Colors[ImGuiCol_ChildBg] = ImVec4(0.015f, 0.050f, 0.026f, 0.72f);
        style.Colors[ImGuiCol_PopupBg] = ImVec4(0.018f, 0.070f, 0.036f, 0.98f);
        style.Colors[ImGuiCol_Border] = ImVec4(accent.x, accent.y, accent.z, 0.24f);
        style.Colors[ImGuiCol_BorderShadow] = ImVec4(0, 0, 0, 0);
        style.Colors[ImGuiCol_FrameBg] = ImVec4(0.028f, 0.110f, 0.058f, 1.0f);
        style.Colors[ImGuiCol_FrameBgHovered] = Mix(panelBg, accent, 0.16f);
        style.Colors[ImGuiCol_FrameBgActive] = Mix(panelBg, accent, 0.22f);
        style.Colors[ImGuiCol_TitleBg] = ImVec4(0.012f, 0.045f, 0.024f, 1.0f);
        style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.020f, 0.080f, 0.040f, 1.0f);
        style.Colors[ImGuiCol_MenuBarBg] = ImVec4(0.015f, 0.050f, 0.028f, 1.0f);
        style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.008f, 0.030f, 0.016f, 0.70f);
        style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(accent.x, accent.y, accent.z, 0.35f);
        style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(accent.x, accent.y, accent.z, 0.55f);
        style.Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(accent.x, accent.y, accent.z, 0.75f);
        style.Colors[ImGuiCol_CheckMark] = accent;
        style.Colors[ImGuiCol_SliderGrab] = accent;
        style.Colors[ImGuiCol_SliderGrabActive] = Mix(accent, ImVec4(1, 1, 1, 1), 0.25f);
        style.Colors[ImGuiCol_Button] = ImVec4(0.035f, 0.130f, 0.070f, 0.92f);
        style.Colors[ImGuiCol_ButtonHovered] = Mix(panelBg, accent, 0.22f);
        style.Colors[ImGuiCol_ButtonActive] = Mix(panelBg, accent, 0.32f);
        style.Colors[ImGuiCol_Header] = ImVec4(accent.x, accent.y, accent.z, 0.16f);
        style.Colors[ImGuiCol_HeaderHovered] = ImVec4(accent.x, accent.y, accent.z, 0.24f);
        style.Colors[ImGuiCol_HeaderActive] = ImVec4(accent.x, accent.y, accent.z, 0.32f);
        style.Colors[ImGuiCol_Separator] = ImVec4(accent.x, accent.y, accent.z, 0.16f);
        style.Colors[ImGuiCol_SeparatorHovered] = ImVec4(accent.x, accent.y, accent.z, 0.35f);
        style.Colors[ImGuiCol_SeparatorActive] = accent;
        style.Colors[ImGuiCol_ResizeGrip] = ImVec4(accent.x, accent.y, accent.z, 0.10f);
        style.Colors[ImGuiCol_ResizeGripHovered] = ImVec4(accent.x, accent.y, accent.z, 0.28f);
        style.Colors[ImGuiCol_ResizeGripActive] = accent;
        style.Colors[ImGuiCol_Tab] = ImVec4(0.025f, 0.090f, 0.048f, 0.9f);
        style.Colors[ImGuiCol_TabHovered] = Mix(panelBg, accent, 0.20f);
        style.Colors[ImGuiCol_TabSelected] = Mix(panelBg, accent, 0.26f);
        style.Colors[ImGuiCol_TabDimmed] = ImVec4(0.020f, 0.070f, 0.036f, 0.8f);
        style.Colors[ImGuiCol_TabDimmedSelected] = Mix(panelBg, accent, 0.18f);
        style.Colors[ImGuiCol_PlotLines] = accent;
        style.Colors[ImGuiCol_PlotLinesHovered] = Mix(accent, ImVec4(1, 1, 1, 1), 0.4f);
        style.Colors[ImGuiCol_PlotHistogram] = accent;
        style.Colors[ImGuiCol_PlotHistogramHovered] = Mix(accent, ImVec4(1, 1, 1, 1), 0.4f);
        style.Colors[ImGuiCol_TableHeaderBg] = ImVec4(0.025f, 0.090f, 0.048f, 1.0f);
        style.Colors[ImGuiCol_TableBorderStrong] = ImVec4(accent.x, accent.y, accent.z, 0.22f);
        style.Colors[ImGuiCol_TableBorderLight] = ImVec4(accent.x, accent.y, accent.z, 0.10f);
        style.Colors[ImGuiCol_TextSelectedBg] = ImVec4(accent.x, accent.y, accent.z, 0.28f);
        style.Colors[ImGuiCol_NavCursor] = accent;
        style.Colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.0f, 0.015f, 0.006f, 0.78f);
    }

    // ─────────────────────────────────────────────────────────────────────────
    // Текст и декоративные примитивы
    // ─────────────────────────────────────────────────────────────────────────
    inline float TextLine(float scale = 1.0f)
    {
        return ImGui::GetFontSize() * scale;
    }

    // Все подписи проходят через очистку: длинные тире превращаются в "-",
    // символ "·" удаляется, повторные пробелы схлопываются.
    std::string SanitizeText(const char* text)
    {
        std::string value = (text != nullptr) ? text : "";
        const char* dashes[] = { "\xE2\x80\x94", "\xE2\x80\x93", "\xE2\x80\x95", "\xE2\x88\x92" };
        for (const char* dash : dashes)
        {
            std::size_t position = 0;
            while ((position = value.find(dash, position)) != std::string::npos)
                value.replace(position, 3, "-");
        }
        const char* middleDot = "\xC2\xB7";
        std::size_t dotPosition = 0;
        while ((dotPosition = value.find(middleDot, dotPosition)) != std::string::npos)
            value.erase(dotPosition, 2);
        std::size_t doubleSpace = 0;
        while ((doubleSpace = value.find("  ", doubleSpace)) != std::string::npos)
            value.replace(doubleSpace, 2, " ");
        return value;
    }

    void TextAt(ImDrawList* draw, ImVec2 pos, ImU32 color, const char* text, float scale = 1.0f);
    void TextAt(ImDrawList* draw, ImVec2 pos, ImU32 color, const char* text, float scale)
    {
        if (text == nullptr || *text == '\0')
            return;
        const std::string clean = SanitizeText(text);
        if (clean.empty())
            return;
        draw->AddText(ImGui::GetFont(), ImGui::GetFontSize() * scale, pos, color, clean.c_str());
    }

    ImVec2 Measure(const char* text, float scale = 1.0f)
    {
        const std::string clean = SanitizeText(text);
        const ImVec2 size = ImGui::CalcTextSize(clean.c_str());
        return ImVec2(size.x * scale, size.y * scale);
    }

    void GlowFrame(ImDrawList* draw, ImVec2 a, ImVec2 b, ImU32 color, float rounding, float s,
                   bool glow, float intensity = 1.0f)
    {
        if (glow)
        {
            for (int i = 3; i >= 1; --i)
            {
                const float spread = i * 2.0f * s;
                draw->AddRect(ImVec2(a.x - spread, a.y - spread), ImVec2(b.x + spread, b.y + spread),
                              WithAlpha(color, 0.05f * intensity), rounding + spread, 0, 1.0f + i * s);
            }
        }
        draw->AddRect(a, b, color, rounding, 0, 1.0f);
    }

    void DrawPixelSpray(ImDrawList* draw, ImVec2 origin, float s, ImU32 color)
    {
        const float blocks[][3] = {
            { 0, 0, 6 }, { 9, -7, 4 }, { -6, 9, 3 }, { 17, 4, 4 }, { 5, 14, 3 }, { 24, -2, 3 }
        };
        for (const auto& block : blocks)
        {
            const ImVec2 p(origin.x + block[0] * s, origin.y + block[1] * s);
            draw->AddRectFilled(p, ImVec2(p.x + block[2] * s, p.y + block[2] * s), color, block[2] * 0.15f * s);
        }
    }

    // ─────────────────────────────────────────────────────────────────────────
    // Кнопки
    // ─────────────────────────────────────────────────────────────────────────
    bool PaintButton(const MenuState& st, float s, const char* id, const char* label,
                     bool primary, float designWidth, float designHeight = 36.0f,
                     Glyph glyph = Glyph::Check, bool glyphVisible = true)
    {
        ImGui::PushID(id);
        const ImVec2 labelSize = Measure(label, 0.70f);
        float width = designWidth > 0.0f ? designWidth * s : labelSize.x + 34.0f * s;
        if (primary && designWidth <= 0.0f)
            width = std::max(width, 140.0f * s);
        const ImVec2 size(width, designHeight * s);
        const ImVec2 p = ImGui::GetCursorScreenPos();
        ImGui::InvisibleButton("##button", size);

        const bool hovered = ImGui::IsItemHovered();
        const bool active = ImGui::IsItemActive();
        const bool pressed = ImGui::IsItemClicked();

        ImDrawList* draw = ImGui::GetWindowDrawList();
        const float rounding = 8.0f * s;
        const ImVec4 baseFill = primary ? ImVec4(0.06f, 0.24f, 0.12f, 0.90f) : ImVec4(0.028f, 0.10f, 0.055f, 0.90f);
        ImU32 bg = ImGui::ColorConvertFloat4ToU32(Mix(baseFill, st.ui.accent, hovered ? (primary ? 0.34f : 0.16f) : (primary ? 0.16f : 0.0f)));
        if (active)
            bg = Accent(st, 0.48f);

        draw->AddRectFilled(p, ImVec2(p.x + size.x, p.y + size.y), bg, rounding);
        if (st.ui.glow && primary)
            GlowFrame(draw, p, ImVec2(p.x + size.x, p.y + size.y), Accent(st, hovered ? 0.65f : 0.42f), rounding, s, true,
                      st.ui.glowIntensity / 55.0f);
        else
            draw->AddRect(p, ImVec2(p.x + size.x, p.y + size.y), Accent(st, 0.32f), rounding, 0, 1.0f);

        float contentWidth = labelSize.x;
        if (glyphVisible)
            contentWidth += 15.0f * s;
        float x = p.x + (size.x - contentWidth) * 0.5f;
        if (glyphVisible)
        {
            DrawGlyph(draw, glyph, ImVec2(x + 6.0f * s, p.y + size.y * 0.5f), 11.0f * s,
                      primary ? Color(0.90f, 1.0f, 0.93f, 1.0f) : Accent(st, 0.95f));
            x += 15.0f * s;
        }
        TextAt(draw, ImVec2(x, p.y + (size.y - TextLine(0.70f)) * 0.5f),
               primary ? Color(0.94f, 1.0f, 0.96f, 1.0f) : Color(0.80f, 0.90f, 0.83f, 1.0f), label, 0.70f);

        ImGui::SetCursorScreenPos(ImVec2(p.x, p.y + size.y + 4.0f * s));
        ImGui::PopID();
        return pressed;
    }

    // ─────────────────────────────────────────────────────────────────────────
    // Переключатель с плавным ходом
    // ─────────────────────────────────────────────────────────────────────────
    bool ToggleControl(const MenuState& st, float s, bool& value)
    {
        const ImVec2 p = ImGui::GetCursorScreenPos();
        const ImVec2 size(45.0f * s, 25.0f * s);
        ImGui::InvisibleButton("##toggle", size);
        const bool hovered = ImGui::IsItemHovered();
        bool changed = false;
        if (ImGui::IsItemClicked())
        {
            value = !value;
            changed = true;
        }

        float& animation = g_toggleAnimations[ImGui::GetID("##toggle")];
        animation = AnimationLerp(animation, value ? 1.0f : 0.0f,
                                  st.ui.animations ? (9.0f + st.ui.animationSpeed * 0.16f) : 1000.0f);

        ImDrawList* draw = ImGui::GetWindowDrawList();
        const float rounding = size.y * 0.5f;
        const ImU32 offFill = Color(0.10f, 0.17f, 0.12f, 1.0f);
        const ImU32 onFill = Accent(st, 0.88f);
        const ImU32 fill = animation > 0.02f
                               ? ImGui::ColorConvertFloat4ToU32(Mix(ImVec4(0.10f, 0.17f, 0.12f, 1.0f), st.ui.accent, animation))
                               : offFill;
        draw->AddRectFilled(p, ImVec2(p.x + size.x, p.y + size.y), fill, rounding);
        const ImU32 border = animation > 0.5f ? Accent(st, 0.95f) : Accent(st, hovered ? 0.55f : 0.34f);
        if (st.ui.glow && animation > 0.4f)
            GlowFrame(draw, p, ImVec2(p.x + size.x, p.y + size.y), Accent(st, 0.40f * animation), rounding, s, true,
                      st.ui.glowIntensity / 55.0f);
        else
            draw->AddRect(p, ImVec2(p.x + size.x, p.y + size.y), border, rounding, 0, 1.0f);

        const float knobSize = size.y - 4.0f * s;
        const float knobX = p.x + 2.0f * s + animation * (size.x - knobSize - 4.0f * s);
        const float knobY = p.y + 2.0f * s;
        const ImU32 knobColor = animation > 0.5f ? Color(1.0f, 1.0f, 1.0f, 1.0f) : Color(0.76f, 0.85f, 0.79f, 1.0f);
        draw->AddCircleFilled(ImVec2(knobX + knobSize * 0.5f, knobY + knobSize * 0.5f), knobSize * 0.5f, knobColor, 24);
        if (animation > 0.5f && st.ui.glow)
            draw->AddCircle(ImVec2(knobX + knobSize * 0.5f, knobY + knobSize * 0.5f), knobSize * 0.62f,
                            Color(1.0f, 1.0f, 1.0f, 0.20f), 24, 2.0f * s);

        ImGui::SetCursorScreenPos(ImVec2(p.x, p.y + size.y));
        return changed;
    }

    // ─────────────────────────────────────────────────────────────────────────
    // Кастомный ползунок
    // ─────────────────────────────────────────────────────────────────────────
    bool SliderControl(const MenuState& st, float s, const char* id, float& value, float min, float max,
                       const char* suffix, int decimals, float designWidth)
    {
        ImGui::PushID(id);
        const float width = designWidth * s;
        const float trackWidth = width - 50.0f * s;
        const float height = 22.0f * s;
        const ImVec2 p = ImGui::GetCursorScreenPos();

        ImGui::InvisibleButton("##track", ImVec2(trackWidth, height));
        bool changed = false;
        if (ImGui::IsItemActive())
        {
            const float t = std::clamp((ImGui::GetIO().MousePos.x - p.x) / std::max(1.0f, trackWidth), 0.0f, 1.0f);
            const float next = min + (max - min) * t;
            if (std::fabs(next - value) > 0.0001f)
            {
                value = next;
                changed = true;
            }
        }
        if (ImGui::IsItemHovered() || ImGui::IsItemActive())
        {
            ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
        }

        const float t = std::clamp((value - min) / std::max(0.0001f, max - min), 0.0f, 1.0f);
        ImDrawList* draw = ImGui::GetWindowDrawList();
        const float cy = p.y + height * 0.5f;
        ImU32 trackColor = Color(0.14f, 0.23f, 0.17f, 1.0f);
        draw->AddRectFilled(ImVec2(p.x, cy - 2.0f * s), ImVec2(p.x + trackWidth, cy + 2.0f * s), trackColor, 2.0f * s);
        draw->AddRectFilled(ImVec2(p.x, cy - 2.0f * s), ImVec2(p.x + trackWidth * t, cy + 2.0f * s), Accent(st), 2.0f * s);
        const ImVec2 knob(p.x + trackWidth * t, cy);
        if (st.ui.glow)
            draw->AddCircleFilled(knob, 9.0f * s, Accent(st, 0.16f), 20);
        draw->AddCircleFilled(knob, 5.5f * s, Color(0.94f, 1.0f, 0.96f, 1.0f), 20);
        draw->AddCircle(knob, 6.5f * s, Accent(st), 20, 1.8f * s);

        char buffer[48];
        std::snprintf(buffer, sizeof(buffer), "%.*f%s", decimals, value, suffix != nullptr ? suffix : "");
        ImGui::SetCursorScreenPos(ImVec2(p.x + trackWidth + 8.0f * s, cy - TextLine(0.68f) * 0.5f));
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.86f, 0.96f, 0.89f, 1.0f));
        ImGui::TextUnformatted(buffer);
        ImGui::PopStyleColor();

        ImGui::SetCursorScreenPos(ImVec2(p.x, p.y + height));
        ImGui::PopID();
        return changed;
    }

    // ─────────────────────────────────────────────────────────────────────────
    // Цвет и выпадающий список
    // ─────────────────────────────────────────────────────────────────────────
    bool ColorControl(const MenuState& st, float s, const char* id, ImVec4& color)
    {
        ImGui::PushID(id);
        bool changed = false;
        const ImGuiColorEditFlags flags = ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoDragDrop |
                                          ImGuiColorEditFlags_NoInputs;
        if (ImGui::ColorButton("##swatch", color, flags, ImVec2(25.0f * s, 24.0f * s)))
            ImGui::OpenPopup("##picker");
        if (ImGui::BeginPopup("##picker"))
        {
            ImGui::TextUnformatted("Цвет");
            ImGui::Separator();
            changed = ImGui::ColorPicker4("##picker4", &color.x,
                                          ImGuiColorEditFlags_NoSidePreview | ImGuiColorEditFlags_NoSmallPreview |
                                              ImGuiColorEditFlags_AlphaBar);
            ImGui::EndPopup();
        }
        ImGui::SameLine(0.0f, 6.0f * s);
        char hex[24];
        std::snprintf(hex, sizeof(hex), "#%02X%02X%02X", static_cast<int>(color.x * 255.0f),
                      static_cast<int>(color.y * 255.0f), static_cast<int>(color.z * 255.0f));
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.62f, 0.75f, 0.65f, 1.0f));
        ImGui::TextUnformatted(hex);
        ImGui::PopStyleColor();
        ImGui::PopID();
        return changed;
    }

    bool ComboControl(float s, int& current, const char* const* items, int count, float designWidth)
    {
        ImGui::SetNextItemWidth(designWidth * s);
        return ImGui::Combo("##combo", &current, items, count);
    }

    // ─────────────────────────────────────────────────────────────────────────
    // Строки настроек
    // ─────────────────────────────────────────────────────────────────────────
    struct RowBox
    {
        ImVec2 start;
        float width;
        float height;
    };

    RowBox BeginSettingRow(float s, const char* id, float designHeight = 39.0f)
    {
        ImGui::PushID(id);
        const ImVec2 p = ImGui::GetCursorScreenPos();
        const float width = ImGui::GetContentRegionAvail().x;
        const float height = designHeight * s;
        // Сначала строка, затем контрол поверх неё: разрешаем перекрытие,
        // иначе тумблер/ползунок/список не получат клик.
        ImGui::SetNextItemAllowOverlap();
        ImGui::InvisibleButton("##row", ImVec2(width, height));
        const bool hovered = ImGui::IsItemHovered();
        ImDrawList* draw = ImGui::GetWindowDrawList();
        if (hovered)
            draw->AddRectFilled(p, ImVec2(p.x + width, p.y + height), Color(0.14f, 0.45f, 0.23f, 0.13f), 6.0f * s);
        return RowBox{ p, width, height };
    }

    void RowLabel(const MenuState& st, float s, const RowBox& row, Glyph glyph,
                  const char* label, const char* note = nullptr)
    {
        ImDrawList* draw = ImGui::GetWindowDrawList();
        const ImVec2 iconPos(row.start.x + 10.0f * s, row.start.y + (row.height - 25.0f * s) * 0.5f);
        draw->AddRectFilled(iconPos, ImVec2(iconPos.x + 25.0f * s, iconPos.y + 25.0f * s),
                            Color(0.035f, 0.13f, 0.07f, 0.88f), 6.0f * s);
        draw->AddRect(iconPos, ImVec2(iconPos.x + 25.0f * s, iconPos.y + 25.0f * s), Accent(st, 0.24f), 6.0f * s, 0,
                      1.0f);
        DrawGlyph(draw, glyph, ImVec2(iconPos.x + 12.5f * s, iconPos.y + 12.5f * s), 13.0f * s, Accent(st, 0.95f));

        const float line = TextLine(0.75f);
        const float textY = row.start.y + (row.height - line) * 0.5f;
        TextAt(draw, ImVec2(row.start.x + 44.0f * s, textY), Color(0.82f, 0.91f, 0.84f, 1.0f), label, 0.75f);
        if (note != nullptr && *note != '\0')
        {
            const float labelWidth = Measure(label, 0.75f).x;
            TextAt(draw, ImVec2(row.start.x + 50.0f * s + labelWidth, textY + 1.5f * s),
                   Color(0.45f, 0.58f, 0.49f, 1.0f), note, 0.62f);
        }
    }

    void PlaceRowControl(const RowBox& row, float s, float controlWidth, float controlHeight)
    {
        ImGui::SetCursorScreenPos(ImVec2(row.start.x + row.width - controlWidth * s - 10.0f * s,
                                         row.start.y + (row.height - controlHeight * s) * 0.5f));
    }

    void EndSettingRow(const RowBox& row, float s)
    {
        ImDrawList* draw = ImGui::GetWindowDrawList();
        draw->AddLine(ImVec2(row.start.x + 10.0f * s, row.start.y + row.height),
                      ImVec2(row.start.x + row.width - 10.0f * s, row.start.y + row.height),
                      Color(0.40f, 0.85f, 0.55f, 0.055f), 1.0f);
        ImGui::SetCursorScreenPos(ImVec2(row.start.x, row.start.y + row.height));
        ImGui::PopID();
    }

    // Готовая строка с тумблером. Возвращает true при изменении.
    bool ToggleRow(const MenuState& st, const MenuActions& actions, float s, const char* id,
                   Glyph glyph, const char* label, bool& value, const char* path, const char* note = nullptr)
    {
        RowBox row = BeginSettingRow(s, id);
        RowLabel(st, s, row, glyph, label, note);
        PlaceRowControl(row, s, 45.0f, 25.0f);
        const bool changed = ToggleControl(st, s, value);
        EndSettingRow(row, s);
        if (changed)
            Notify(actions, path);
        return changed;
    }

    bool SliderRow(const MenuState& st, const MenuActions& actions, float s, const char* id,
                   Glyph glyph, const char* label, float& value, float min, float max,
                   const char* suffix, int decimals, const char* path)
    {
        RowBox row = BeginSettingRow(s, id);
        RowLabel(st, s, row, glyph, label);
        PlaceRowControl(row, s, 210.0f, 22.0f);
        const bool changed = SliderControl(st, s, id, value, min, max, suffix, decimals, 210.0f);
        EndSettingRow(row, s);
        if (changed)
            Notify(actions, path);
        return changed;
    }

    bool ColorRow(const MenuState& st, const MenuActions& actions, float s, const char* id,
                  const char* label, ImVec4& color, const char* path)
    {
        RowBox row = BeginSettingRow(s, id);
        RowLabel(st, s, row, Glyph::Palette, label);
        PlaceRowControl(row, s, 120.0f, 25.0f);
        const bool changed = ColorControl(st, s, id, color);
        EndSettingRow(row, s);
        if (changed)
            Notify(actions, path);
        return changed;
    }

    bool ComboRow(const MenuState& st, const MenuActions& actions, float s, const char* id,
                  Glyph glyph, const char* label, int& current, const char* const* items, int count,
                  const char* path)
    {
        RowBox row = BeginSettingRow(s, id);
        RowLabel(st, s, row, glyph, label);
        PlaceRowControl(row, s, 170.0f, 30.0f);
        const bool changed = ComboControl(s, current, items, count, 170.0f);
        EndSettingRow(row, s);
        if (changed)
            Notify(actions, path);
        return changed;
    }

    // ─────────────────────────────────────────────────────────────────────────
    // Карточка группы настроек
    // ─────────────────────────────────────────────────────────────────────────
    void BeginCard(const MenuState& st, float s, const char* id, const char* title, const char* note = nullptr)
    {
        ImGui::PushID(id);
        ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 12.0f * s);
        ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 1.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(6.0f * s, 5.0f * s));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));
        ImGui::PushStyleColor(ImGuiCol_ChildBg, Color(0.027f, 0.085f, 0.045f, 0.82f));
        ImGui::PushStyleColor(ImGuiCol_Border, Accent(st, st.ui.glow ? 0.25f : 0.18f));
        ImGui::BeginChild("##card", ImVec2(0.0f, 0.0f), ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_Borders,
                          ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

        const ImVec2 p = ImGui::GetCursorScreenPos();
        const float width = ImGui::GetContentRegionAvail().x;
        const float headerHeight = 36.0f * s;
        ImDrawList* draw = ImGui::GetWindowDrawList();
        draw->AddRectFilled(p, ImVec2(p.x + width, p.y + headerHeight), Color(0.055f, 0.16f, 0.085f, 0.46f),
                            8.0f * s);
        draw->AddRectFilled(ImVec2(p.x + 11.0f * s, p.y + 15.0f * s),
                            ImVec2(p.x + 16.0f * s, p.y + 20.0f * s), Accent(st), 1.0f * s);
        TextAt(draw, ImVec2(p.x + 24.0f * s, p.y + (headerHeight - TextLine(0.80f)) * 0.5f),
               Color(0.90f, 0.99f, 0.92f, 1.0f), title, 0.80f);
        if (note != nullptr && *note != '\0')
        {
            const ImVec2 noteSize = Measure(note, 0.60f);
            TextAt(draw, ImVec2(p.x + width - noteSize.x - 11.0f * s,
                                p.y + (headerHeight - TextLine(0.60f)) * 0.5f),
                   Color(0.39f, 0.52f, 0.43f, 1.0f), note, 0.60f);
        }
        draw->AddLine(ImVec2(p.x, p.y + headerHeight), ImVec2(p.x + width, p.y + headerHeight),
                      Accent(st, 0.12f), 1.0f);
        ImGui::Dummy(ImVec2(0.0f, headerHeight));
        ImGui::Dummy(ImVec2(0.0f, 2.0f * s));
    }

    void EndCard(float s)
    {
        ImGui::Dummy(ImVec2(0.0f, 2.0f * s));
        ImGui::EndChild();
        ImGui::PopStyleColor(2);
        ImGui::PopStyleVar(4);
        ImGui::PopID();
        ImGui::Dummy(ImVec2(0.0f, 4.0f * s));
    }

    // ─────────────────────────────────────────────────────────────────────────
    // Заголовок раздела
    // ─────────────────────────────────────────────────────────────────────────
    void SectionHeader(const MenuState& st, float s, Glyph glyph, const char* title,
                       const char* subtitle, const char* badge)
    {
        const ImVec2 p = ImGui::GetCursorScreenPos();
        const float width = ImGui::GetContentRegionAvail().x;
        ImDrawList* draw = ImGui::GetWindowDrawList();
        const float iconBox = 38.0f * s;
        draw->AddRectFilled(p, ImVec2(p.x + iconBox, p.y + iconBox), Color(0.04f, 0.16f, 0.08f, 0.9f), 10.0f * s);
        GlowFrame(draw, p, ImVec2(p.x + iconBox, p.y + iconBox), Accent(st, 0.30f), 10.0f * s, s, st.ui.glow);
        DrawGlyph(draw, glyph, ImVec2(p.x + iconBox * 0.5f, p.y + iconBox * 0.5f), 20.0f * s, Accent(st));

        TextAt(draw, ImVec2(p.x + 49.0f * s, p.y + 1.0f * s), Color(0.95f, 1.0f, 0.96f, 1.0f), title, 1.16f);
        TextAt(draw, ImVec2(p.x + 50.0f * s, p.y + 25.0f * s), Color(0.52f, 0.66f, 0.56f, 1.0f), subtitle, 0.64f);

        if (badge != nullptr && *badge != '\0')
        {
            const ImVec2 badgeSize = Measure(badge, 0.56f);
            const float badgeW = badgeSize.x + 14.0f * s;
            const ImVec2 bp(p.x + width - badgeW, p.y + 8.0f * s);
            draw->AddRectFilled(bp, ImVec2(bp.x + badgeW, bp.y + 18.0f * s), Accent(st, 0.09f), 5.0f * s);
            draw->AddRect(bp, ImVec2(bp.x + badgeW, bp.y + 18.0f * s), Accent(st, 0.28f), 5.0f * s, 0, 1.0f);
            TextAt(draw, ImVec2(bp.x + 7.0f * s, bp.y + 3.5f * s), Accent(st, 0.95f), badge, 0.56f);
        }

        draw->AddLine(ImVec2(p.x, p.y + 46.0f * s), ImVec2(p.x + width, p.y + 46.0f * s), Accent(st, 0.14f), 1.0f);
        ImGui::SetCursorScreenPos(ImVec2(p.x, p.y + 55.0f * s));
    }

    // ─────────────────────────────────────────────────────────────────────────
    // Навигация и вкладки
    // ─────────────────────────────────────────────────────────────────────────
    bool NavItem(const MenuState& st, float s, const char* label, Glyph glyph, bool active)
    {
        const ImVec2 p = ImGui::GetCursorScreenPos();
        const float width = ImGui::GetContentRegionAvail().x;
        const ImVec2 size(width, 38.0f * s);
        ImGui::InvisibleButton(label, size);
        const bool hovered = ImGui::IsItemHovered();
        const bool pressed = ImGui::IsItemClicked();

        ImDrawList* draw = ImGui::GetWindowDrawList();
        const float rounding = 10.0f * s;
        if (active)
        {
            draw->AddRectFilled(p, ImVec2(p.x + size.x, p.y + size.y), Accent(st, 0.15f), rounding);
            draw->AddRect(p, ImVec2(p.x + size.x, p.y + size.y), Accent(st, 0.46f), rounding, 0, 1.0f);
            draw->AddRectFilled(ImVec2(p.x - 6.0f * s, p.y + 11.0f * s),
                                ImVec2(p.x - 3.0f * s, p.y + size.y - 11.0f * s), Accent(st), 2.0f * s);
        }
        else if (hovered)
        {
            draw->AddRectFilled(p, ImVec2(p.x + size.x, p.y + size.y), Color(0.09f, 0.30f, 0.16f, 0.30f), rounding);
        }

        const ImVec2 iconCenter(p.x + 22.0f * s, p.y + size.y * 0.5f);
        draw->AddRectFilled(ImVec2(iconCenter.x - 14.0f * s, iconCenter.y - 13.0f * s),
                            ImVec2(iconCenter.x + 14.0f * s, iconCenter.y + 13.0f * s),
                            Accent(st, active ? 0.15f : 0.06f), 7.0f * s);
        draw->AddRect(ImVec2(iconCenter.x - 14.0f * s, iconCenter.y - 13.0f * s),
                      ImVec2(iconCenter.x + 14.0f * s, iconCenter.y + 13.0f * s), Accent(st, active ? 0.34f : 0.14f),
                      7.0f * s, 0, 1.0f);
        DrawGlyph(draw, glyph, iconCenter, 15.0f * s, Accent(st, 0.95f));
        TextAt(draw, ImVec2(p.x + 45.0f * s, p.y + (size.y - TextLine(0.76f)) * 0.5f),
               active ? Color(0.96f, 1.0f, 0.97f, 1.0f) : Color(0.60f, 0.74f, 0.63f, 1.0f), label, 0.76f);

        ImGui::SetCursorScreenPos(ImVec2(p.x, p.y + size.y + 3.0f * s));
        return pressed;
    }

    bool ChromeButton(const MenuState& st, float s, Glyph glyph, bool danger)
    {
        ImGui::PushID(static_cast<int>(glyph));
        const ImVec2 p = ImGui::GetCursorScreenPos();
        const ImVec2 size(27.0f * s, 27.0f * s);
        ImGui::InvisibleButton("##chrome", size);
        const bool hovered = ImGui::IsItemHovered();
        const bool pressed = ImGui::IsItemClicked();
        ImDrawList* draw = ImGui::GetWindowDrawList();
        if (hovered)
            draw->AddRectFilled(p, ImVec2(p.x + size.x, p.y + size.y),
                                danger ? Color(0.55f, 0.13f, 0.16f, 0.35f) : Accent(st, 0.12f), 6.0f * s);
        DrawGlyph(draw, glyph, ImVec2(p.x + size.x * 0.5f, p.y + size.y * 0.5f), 11.0f * s,
                  hovered ? (danger ? Color(1.0f, 0.62f, 0.64f, 1.0f) : Color(0.95f, 1.0f, 0.96f, 1.0f))
                          : Color(0.52f, 0.68f, 0.57f, 1.0f));
        ImGui::SetCursorScreenPos(ImVec2(p.x + size.x + 2.0f * s, p.y));
        ImGui::PopID();
        return pressed;
    }

} // namespace
}

namespace gayclient
{
namespace
{
    Glyph GlyphForModule(ModuleId id)
    {
        switch (id)
        {
        case ModuleId::Fps: return Glyph::Activity;
        case ModuleId::Cps: return Glyph::Mouse;
        case ModuleId::Clock: return Glyph::Clock;
        case ModuleId::Keystrokes: return Glyph::Keyboard;
        case ModuleId::Watermark: return Glyph::Droplet;
        case ModuleId::Coordinates: return Glyph::Grid;
        case ModuleId::Performance: return Glyph::Cpu;
        case ModuleId::Resolution: return Glyph::Monitor;
        case ModuleId::ModuleList: return Glyph::Layers;
        case ModuleId::ChinaHat: return Glyph::Cube;
        case ModuleId::Trail: return Glyph::Bolt;
        case ModuleId::JumpRings: return Glyph::Sparkles;
        }
        return Glyph::Dots;
    }

    bool* EnabledFlag(MenuState& state, ModuleId id)
    {
        switch (id)
        {
        case ModuleId::Fps: return &state.hud.showFps;
        case ModuleId::Cps: return &state.hud.showCps;
        case ModuleId::Clock: return &state.hud.showClock;
        case ModuleId::Keystrokes: return &state.hud.showKeystrokes;
        case ModuleId::Watermark: return &state.hud.showWatermark;
        case ModuleId::Coordinates: return &state.hud.showCoordinates;
        case ModuleId::Performance: return &state.hud.showPerformance;
        case ModuleId::Resolution: return &state.hud.showResolution;
        case ModuleId::ModuleList: return &state.hud.showModuleList;
        case ModuleId::ChinaHat: return &state.graphics.chinaHat.enabled;
        case ModuleId::Trail: return &state.graphics.trail.enabled;
        case ModuleId::JumpRings: return &state.graphics.jumpRings.enabled;
        }
        return nullptr;
    }

    const char* ModuleEnabledPath(ModuleId id)
    {
        switch (id)
        {
        case ModuleId::Fps: return "modules.fps";
        case ModuleId::Cps: return "modules.cps";
        case ModuleId::Clock: return "modules.clock";
        case ModuleId::Keystrokes: return "modules.keystrokes";
        case ModuleId::Watermark: return "modules.watermark";
        case ModuleId::Coordinates: return "modules.coordinates";
        case ModuleId::Performance: return "modules.performance";
        case ModuleId::Resolution: return "modules.resolution";
        case ModuleId::ModuleList: return "modules.moduleList";
        case ModuleId::ChinaHat: return "modules.chinaHat";
        case ModuleId::Trail: return "modules.trail";
        case ModuleId::JumpRings: return "modules.jumpRings";
        }
        return "modules.unknown";
    }

    void NoteText(float s, const char* text)
    {
        const std::string clean = SanitizeText(text);
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.46f, 0.60f, 0.50f, 1.0f));
        ImGui::PushTextWrapPos(0.0f);
        ImGui::TextWrapped("%s", clean.c_str());
        ImGui::PopTextWrapPos();
        ImGui::PopStyleColor();
        ImGui::Dummy(ImVec2(0.0f, 3.0f * s));
    }

    void SmallCaption(float s, const char* text)
    {
        const std::string clean = SanitizeText(text);
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.47f, 0.62f, 0.51f, 1.0f));
        ImGui::TextUnformatted(clean.c_str());
        ImGui::PopStyleColor();
    }

    // ─────────────────────────────────────────────────────────────────────────
    // Раздел «Главная»
    // ─────────────────────────────────────────────────────────────────────────
    void RenderHome(MenuState& st, const MenuActions& ac, float s)
    {
        SectionHeader(st, s, Glyph::Home, "Главная", "Обзор демонстрационного интерфейса GAYClient", "1.1.5");
        const ImVec2 p = ImGui::GetCursorScreenPos();
        const float width = ImGui::GetContentRegionAvail().x;
        const float height = 118.0f * s;
        ImDrawList* draw = ImGui::GetWindowDrawList();
        draw->AddRectFilled(p, ImVec2(p.x + width, p.y + height), Color(0.045f, 0.17f, 0.085f, 0.65f), 12.0f * s);
        GlowFrame(draw, p, ImVec2(p.x + width, p.y + height), Accent(st, 0.25f), 12.0f * s, s, st.ui.glow);
        DrawPixelSpray(draw, ImVec2(p.x + width - 42.0f * s, p.y + 18.0f * s), s, Accent(st, 0.16f));
        TextAt(draw, ImVec2(p.x + 14.0f * s, p.y + 12.0f * s), Accent(st, 0.9f), "ВИЗУАЛЬНАЯ ЧАСТЬ КЛИЕНТА", 0.58f);
        TextAt(draw, ImVec2(p.x + 14.0f * s, p.y + 30.0f * s), Color(0.95f, 1.0f, 0.96f, 1.0f), "GAYCLIENT", 1.45f);
        TextAt(draw, ImVec2(p.x + 14.0f * s, p.y + 65.0f * s), Color(0.54f, 0.69f, 0.58f, 1.0f),
               "Единое меню для Minecraft Bedrock. Данные игры не подключены.", 0.62f);
        const char* chips[] = { "Демо-режим", "Версия 1.1.5", "Нет подключения" };
        float chipX = p.x + 14.0f * s;
        for (int i = 0; i < 3; ++i)
        {
            const float chipW = Measure(chips[i], 0.58f).x + 14.0f * s;
            const ImVec2 cp(chipX, p.y + 88.0f * s);
            draw->AddRectFilled(cp, ImVec2(cp.x + chipW, cp.y + 19.0f * s), Accent(st, i == 0 ? 0.12f : 0.05f), 4.0f * s);
            draw->AddRect(cp, ImVec2(cp.x + chipW, cp.y + 19.0f * s), Accent(st, 0.24f), 4.0f * s, 0, 1.0f);
            TextAt(draw, ImVec2(cp.x + 7.0f * s, cp.y + 4.0f * s), Accent(st, 0.85f), chips[i], 0.58f);
            chipX += chipW + 6.0f * s;
        }
        ImGui::Dummy(ImVec2(0.0f, height + 4.0f * s));

        BeginCard(st, s, "quick", "Быстрый переход", "разделы");
        if (PaintButton(st, s, "quickHud", "Настроить HUD", false, 0.0f, 31.0f, Glyph::Monitor))
        {
            st.tab = Tab::Settings;
            st.section = Section::Hud;
            if (ac.sectionChanged) ac.sectionChanged(Section::Hud);
        }
        ImGui::SameLine();
        if (PaintButton(st, s, "quickGraphics", "Эффекты графики", false, 0.0f, 31.0f, Glyph::Sparkles))
        {
            st.tab = Tab::Settings;
            st.section = Section::Graphics;
            if (ac.sectionChanged) ac.sectionChanged(Section::Graphics);
        }
        ImGui::SameLine();
        if (PaintButton(st, s, "quickInterface", "Оформление", false, 0.0f, 31.0f, Glyph::Palette))
        {
            st.tab = Tab::Settings;
            st.section = Section::Interface;
            if (ac.sectionChanged) ac.sectionChanged(Section::Interface);
        }
        EndCard(s);

        int enabled = 0;
        for (const ModuleEntry& module : st.modules)
            enabled += module.enabled ? 1 : 0;

        BeginCard(st, s, "enabledModules", "Включённые модули", "демо");
        ToggleRow(st, ac, s, "homeFps", Glyph::Activity, "FPS", st.hud.showFps, "hud.showFps");
        ToggleRow(st, ac, s, "homeCps", Glyph::Mouse, "CPS", st.hud.showCps, "hud.showCps");
        ToggleRow(st, ac, s, "homeClock", Glyph::Clock, "Часы", st.hud.showClock, "hud.showClock");
        NoteText(s, "Счётчик активных модулей ниже отражает только демонстрационные переключатели.");
        char summary[96];
        std::snprintf(summary, sizeof(summary), "Активно модулей: %d из %d", enabled, static_cast<int>(st.modules.size()));
        SmallCaption(s, summary);
        EndCard(s);

        BeginCard(st, s, "clientInfo", "Сведения о клиенте", "только UI");
        const char* rows[][2] = {
            { "Название клиента", "GAYClient" },
            { "Версия интерфейса", "1.1.5 · демонстрация" },
            { "Подключение к игре", "Не подключено" }
        };
        for (const auto& pair : rows)
        {
            RowBox row = BeginSettingRow(s, pair[0], 36.0f);
            RowLabel(st, s, row, Glyph::Info, pair[0]);
            const ImVec2 valueSize = Measure(pair[1], 0.66f);
            TextAt(ImGui::GetWindowDrawList(),
                   ImVec2(row.start.x + row.width - valueSize.x - 10.0f * s,
                          row.start.y + (row.height - TextLine(0.66f)) * 0.5f),
                   Color(0.55f, 0.70f, 0.59f, 1.0f), pair[1], 0.66f);
            EndSettingRow(row, s);
        }
        EndCard(s);
        NoteText(s, "Реальные игровые модули и рендер не подключены: источник данных должен передать существующий клиент.");
    }

    // ─────────────────────────────────────────────────────────────────────────
    // Раздел HUD
    // ─────────────────────────────────────────────────────────────────────────
    void PositionTiles(MenuState& st, const MenuActions& ac, float s)
    {
        const float available = ImGui::GetContentRegionAvail().x;
        const float gap = 4.0f * s;
        const float width = (available - gap * 4.0f) / 5.0f;
        const ImVec2 origin = ImGui::GetCursorScreenPos();
        ImDrawList* draw = ImGui::GetWindowDrawList();
        for (int i = 0; i < kPositionCount; ++i)
        {
            const ImVec2 p(origin.x + i * (width + gap), origin.y);
            ImGui::SetCursorScreenPos(p);
            ImGui::PushID(i);
            ImGui::InvisibleButton("##pos", ImVec2(width, 27.0f * s));
            const bool active = st.hud.position == i;
            const bool hovered = ImGui::IsItemHovered();
            if (ImGui::IsItemClicked())
            {
                st.hud.position = i;
                Notify(ac, "hud.position");
            }
            draw->AddRectFilled(p, ImVec2(p.x + width, p.y + 27.0f * s),
                                active ? Accent(st, 0.13f) : Color(0.025f, 0.09f, 0.05f, 0.75f), 5.0f * s);
            draw->AddRect(p, ImVec2(p.x + width, p.y + 27.0f * s),
                          active ? Accent(st, 0.45f) : Accent(st, hovered ? 0.25f : 0.10f), 5.0f * s, 0, 1.0f);
            const ImVec2 labelSize = Measure(kPositionShort[i], 0.56f);
            TextAt(draw, ImVec2(p.x + (width - labelSize.x) * 0.5f, p.y + (27.0f * s - TextLine(0.56f)) * 0.5f),
                   active ? Accent(st) : Color(0.42f, 0.56f, 0.46f, 1.0f), kPositionShort[i], 0.56f);
            ImGui::PopID();
        }
        ImGui::SetCursorScreenPos(ImVec2(origin.x, origin.y + 31.0f * s));
    }

    void RenderHud(MenuState& st, const MenuActions& ac, float s)
    {
        SectionHeader(st, s, Glyph::Monitor, "HUD", "Настройте отображение игрового интерфейса", "ВИЗУАЛ");
        BeginCard(st, s, "hudMain", "Основные настройки", "10 параметров");
        ToggleRow(st, ac, s, "hudFps", Glyph::Activity, "Показывать FPS", st.hud.showFps, "hud.showFps");
        ToggleRow(st, ac, s, "hudCps", Glyph::Mouse, "Показывать CPS", st.hud.showCps, "hud.showCps");
        ToggleRow(st, ac, s, "hudKeys", Glyph::Keyboard, "Показывать нажатые клавиши", st.hud.showKeystrokes, "hud.showKeystrokes");
        ToggleRow(st, ac, s, "hudClock", Glyph::Clock, "Показывать часы", st.hud.showClock, "hud.showClock");
        ToggleRow(st, ac, s, "hudWatermark", Glyph::Droplet, "Показывать водяной знак", st.hud.showWatermark, "hud.showWatermark");
        ToggleRow(st, ac, s, "hudCoords", Glyph::Grid, "Показывать координаты", st.hud.showCoordinates, "hud.showCoordinates");
        ToggleRow(st, ac, s, "hudPerf", Glyph::Cpu, "Показывать производительность CPU и RAM", st.hud.showPerformance, "hud.showPerformance");
        ToggleRow(st, ac, s, "hudRes", Glyph::Monitor, "Показывать разрешение", st.hud.showResolution, "hud.showResolution");
        ToggleRow(st, ac, s, "hudMods", Glyph::Layers, "Показывать список активных модулей", st.hud.showModuleList, "hud.showModuleList");
        ToggleRow(st, ac, s, "hudPing", Glyph::Wifi, "Показывать пинг", st.hud.showPing, "hud.showPing", "если доступен");
        EndCard(s);

        BeginCard(st, s, "hudSizes", "Размер и внешний вид", nullptr);
        SliderRow(st, ac, s, "hudScale", Glyph::Monitor, "Масштаб HUD", st.hud.scale, 0.5f, 2.0f, "x", 2, "hud.scale");
        SliderRow(st, ac, s, "hudElementSize", Glyph::Sliders, "Размер элементов", st.hud.elementSize, 0.5f, 2.0f, "x", 2, "hud.elementSize");
        SliderRow(st, ac, s, "hudOpacity", Glyph::Eye, "Прозрачность", st.hud.opacity, 0.0f, 100.0f, "%", 0, "hud.opacity");
        ColorRow(st, ac, s, "hudTextColor", "Цвет текста", st.hud.textColor, "hud.textColor");
        SliderRow(st, ac, s, "hudSpacing", Glyph::Sliders, "Отступы между элементами", st.hud.spacing, 0.0f, 24.0f, "px", 0, "hud.spacing");
        SliderRow(st, ac, s, "hudPadding", Glyph::Grid, "Внутренние отступы", st.hud.padding, 0.0f, 24.0f, "px", 0, "hud.padding");
        SliderRow(st, ac, s, "hudRadius", Glyph::Cube, "Скругление карточек", st.hud.cardRounding, 0.0f, 24.0f, "px", 0, "hud.cardRounding");
        EndCard(s);

        BeginCard(st, s, "hudPosition", "Положение", nullptr);
        ComboRow(st, ac, s, "hudPosCombo", Glyph::Grid, "Позиция HUD", st.hud.position, kPositions, kPositionCount, "hud.position");
        PositionTiles(st, ac, s);
        EndCard(s);

        BeginCard(st, s, "hudModuleList", "Список модулей", "доп. карточка");
        ToggleRow(st, ac, s, "hudModsEnabled", Glyph::Layers, "Показывать список модулей", st.hud.moduleList.enabled, "hud.moduleList.enabled");
        SliderRow(st, ac, s, "hudModsScale", Glyph::Sliders, "Масштаб списка", st.hud.moduleList.scale, 0.5f, 2.0f, "x", 2, "hud.moduleList.scale");
        ColorRow(st, ac, s, "hudModsColor", "Цвет текста", st.hud.moduleList.textColor, "hud.moduleList.textColor");
        SliderRow(st, ac, s, "hudModsOpacity", Glyph::Droplet, "Прозрачность фона", st.hud.moduleList.backgroundOpacity, 0.0f, 100.0f, "%", 0, "hud.moduleList.backgroundOpacity");
        ComboRow(st, ac, s, "hudModsPos", Glyph::Grid, "Положение списка", st.hud.moduleList.position, kPositions, kPositionCount, "hud.moduleList.position");
        EndCard(s);

        if (PaintButton(st, s, "hudEditor", "Открыть редактор HUD", true, 0.0f, 35.0f, Glyph::Sliders))
        {
            if (ac.hudEditorRequested)
                ac.hudEditorRequested();
        }
        ImGui::SameLine(0.0f, 8.0f * s);
        if (PaintButton(st, s, "hudReset", "Сбросить", false, 0.0f, 35.0f, Glyph::Reset))
        {
            if (ac.resetHudRequested)
                ac.resetHudRequested();
        }
        NoteText(s, "Предпросмотр справа — условный макет. Числа заменены прочерками и не являются данными Minecraft.");
    }

    // ─────────────────────────────────────────────────────────────────────────
    // Раздел «Интерфейс»
    // ─────────────────────────────────────────────────────────────────────────
    void RenderInterface(MenuState& st, const MenuActions& ac, float s)
    {
        SectionHeader(st, s, Glyph::Palette, "Интерфейс", "Тема, акцентный цвет и оформление меню", "ТЕМА");
        BeginCard(st, s, "themes", "Цветовая тема", "Aogiri Green по умолчанию");
        const float available = ImGui::GetContentRegionAvail().x;
        const float gap = 6.0f * s;
        const float width = (available - gap * 2.0f) / 3.0f;
        const ImVec2 origin = ImGui::GetCursorScreenPos();
        ImDrawList* draw = ImGui::GetWindowDrawList();
        for (int i = 0; i < kThemeCount; ++i)
        {
            const ImVec2 p(origin.x + i * (width + gap), origin.y);
            ImGui::SetCursorScreenPos(p);
            ImGui::PushID(i);
            ImGui::InvisibleButton("##theme", ImVec2(width, 68.0f * s));
            const bool active = st.ui.theme == i;
            if (ImGui::IsItemClicked())
            {
                st.ui.theme = i;
                st.ui.accent = kThemeAccents[i];
                Notify(ac, "ui.theme");
            }
            const ImVec2 previewP(p.x + 5.0f * s, p.y + 5.0f * s);
            draw->AddRectFilled(p, ImVec2(p.x + width, p.y + 68.0f * s), Color(0.025f, 0.09f, 0.05f, 0.78f), 7.0f * s);
            draw->AddRect(p, ImVec2(p.x + width, p.y + 68.0f * s), active ? ToColor(kThemeAccents[i], 0.65f) : Accent(st, 0.16f),
                          7.0f * s, 0, active ? 1.4f : 1.0f);
            draw->AddRectFilled(previewP, ImVec2(p.x + width - 5.0f * s, p.y + 37.0f * s), Color(0.01f, 0.045f, 0.02f, 1.0f), 4.0f * s);
            for (int lineIndex = 0; lineIndex < 3; ++lineIndex)
            {
                const ImVec2 lp(previewP.x + 6.0f * s, previewP.y + (7.0f + lineIndex * 8.0f) * s);
                draw->AddRectFilled(lp, ImVec2(lp.x + (18.0f + lineIndex * 5.0f) * s, lp.y + 2.4f * s),
                                    ToColor(kThemeAccents[i], 0.55f), 1.0f * s);
            }
            draw->AddRect(ImVec2(p.x + width - 17.0f * s, p.y + 9.0f * s),
                          ImVec2(p.x + width - 9.0f * s, p.y + 27.0f * s), ToColor(kThemeAccents[i], 0.35f), 1.5f * s, 0, 1.0f);
            draw->AddText(ImGui::GetFont(), ImGui::GetFontSize() * 0.56f,
                          ImVec2(p.x + 7.0f * s, p.y + 48.0f * s), Color(0.70f, 0.82f, 0.73f, 1.0f), kThemes[i]);
            if (active)
                DrawGlyph(draw, Glyph::Check, ImVec2(p.x + width - 12.0f * s, p.y + 55.0f * s), 10.0f * s,
                          ToColor(kThemeAccents[i]));
            ImGui::PopID();
        }
        ImGui::SetCursorScreenPos(ImVec2(origin.x, origin.y + 74.0f * s));
        EndCard(s);

        BeginCard(st, s, "uiColors", "Цвета", nullptr);
        ColorRow(st, ac, s, "uiAccent", "Акцентный цвет", st.ui.accent, "ui.accent");
        ColorRow(st, ac, s, "uiText", "Цвет текста", st.ui.textColor, "ui.textColor");
        ColorRow(st, ac, s, "uiBackground", "Цвет фона", st.ui.backgroundColor, "ui.backgroundColor");
        EndCard(s);

        BeginCard(st, s, "uiScale", "Размер и прозрачность", nullptr);
        SliderRow(st, ac, s, "uiMenuScale", Glyph::Sliders, "Масштаб меню", st.ui.menuScale, 80.0f, 120.0f, "%", 0, "ui.menuScale");
        SliderRow(st, ac, s, "uiBrightness", Glyph::Bolt, "Яркость", st.ui.brightness, 55.0f, 115.0f, "%", 0, "ui.brightness");
        SliderRow(st, ac, s, "uiOpacity", Glyph::Eye, "Прозрачность", st.ui.opacity, 40.0f, 100.0f, "%", 0, "ui.opacity");
        SliderRow(st, ac, s, "uiRounding", Glyph::Cube, "Скругление", st.ui.rounding, 0.0f, 24.0f, "px", 0, "ui.rounding");
        EndCard(s);

        BeginCard(st, s, "uiFx", "Свечение и анимации", "эффекты");
        ToggleRow(st, ac, s, "uiGlow", Glyph::Sparkles, "Свечение элементов", st.ui.glow, "ui.glow");
        SliderRow(st, ac, s, "uiGlowStrength", Glyph::Bolt, "Интенсивность свечения", st.ui.glowIntensity, 0.0f, 100.0f, "%", 0, "ui.glowIntensity");
        ToggleRow(st, ac, s, "uiAnims", Glyph::Sparkles, "Анимации интерфейса", st.ui.animations, "ui.animations");
        SliderRow(st, ac, s, "uiAnimSpeed", Glyph::Activity, "Скорость анимаций", st.ui.animationSpeed, 0.0f, 100.0f, "%", 0, "ui.animationSpeed");
        EndCard(s);

        if (PaintButton(st, s, "uiReset", "Сбросить оформление", true, 0.0f, 35.0f, Glyph::Reset))
        {
            if (ac.resetInterfaceRequested)
                ac.resetInterfaceRequested();
        }
    }

    // ─────────────────────────────────────────────────────────────────────────
    // Общие группы полей эффектов
    // ─────────────────────────────────────────────────────────────────────────
    void ChinaHatFields(MenuState& st, const MenuActions& ac, float s)
    {
        ToggleRow(st, ac, s, "chinaEnabled", Glyph::Cube, "Включение эффекта", st.graphics.chinaHat.enabled, "graphics.chinaHat.enabled");
        ToggleRow(st, ac, s, "chinaFill", Glyph::Image, "Заливка конуса", st.graphics.chinaHat.fill, "graphics.chinaHat.fill");
        ColorRow(st, ac, s, "chinaColor", "Цвет конуса", st.graphics.chinaHat.color, "graphics.chinaHat.color");
        ToggleRow(st, ac, s, "chinaRainbow", Glyph::Sparkles, "Радужный режим", st.graphics.chinaHat.rainbow, "graphics.chinaHat.rainbow");
        SliderRow(st, ac, s, "chinaRainSpeed", Glyph::Activity, "Скорость радуги", st.graphics.chinaHat.rainbowSpeed, 0.0f, 100.0f, "%", 0, "graphics.chinaHat.rainbowSpeed");
        SliderRow(st, ac, s, "chinaOpacity", Glyph::Eye, "Прозрачность", st.graphics.chinaHat.opacity, 0.0f, 100.0f, "%", 0, "graphics.chinaHat.opacity");
        SliderRow(st, ac, s, "chinaRadius", Glyph::Grid, "Радиус", st.graphics.chinaHat.radius, 0.2f, 3.0f, "", 2, "graphics.chinaHat.radius");
        SliderRow(st, ac, s, "chinaHeight", Glyph::Arrow, "Высота конуса", st.graphics.chinaHat.height, 0.2f, 3.0f, "", 2, "graphics.chinaHat.height");
        SliderRow(st, ac, s, "chinaOffset", Glyph::Arrow, "Смещение над головой", st.graphics.chinaHat.offset, 0.0f, 2.0f, "", 2, "graphics.chinaHat.offset");
        SliderRow(st, ac, s, "chinaAmp", Glyph::Activity, "Амплитуда плавания", st.graphics.chinaHat.floatAmplitude, 0.0f, 1.0f, "", 2, "graphics.chinaHat.floatAmplitude");
        SliderRow(st, ac, s, "chinaSpeed", Glyph::Sliders, "Скорость плавания", st.graphics.chinaHat.floatSpeed, 0.0f, 3.0f, "x", 1, "graphics.chinaHat.floatSpeed");
    }

    void TrailFields(MenuState& st, const MenuActions& ac, float s)
    {
        ToggleRow(st, ac, s, "trailEnabled", Glyph::Bolt, "Включение эффекта", st.graphics.trail.enabled, "graphics.trail.enabled");
        ToggleRow(st, ac, s, "trailFill", Glyph::Image, "Заливка следа", st.graphics.trail.fill, "graphics.trail.fill");
        ColorRow(st, ac, s, "trailColor", "Цвет следа", st.graphics.trail.color, "graphics.trail.color");
        ToggleRow(st, ac, s, "trailRainbow", Glyph::Sparkles, "Радужный режим", st.graphics.trail.rainbow, "graphics.trail.rainbow");
        SliderRow(st, ac, s, "trailRainSpeed", Glyph::Activity, "Скорость радуги", st.graphics.trail.rainbowSpeed, 0.0f, 100.0f, "%", 0, "graphics.trail.rainbowSpeed");
        SliderRow(st, ac, s, "trailOpacity", Glyph::Eye, "Прозрачность", st.graphics.trail.opacity, 0.0f, 100.0f, "%", 0, "graphics.trail.opacity");
        SliderRow(st, ac, s, "trailHeight", Glyph::Arrow, "Высота следа", st.graphics.trail.height, 0.2f, 3.0f, "", 2, "graphics.trail.height");
        SliderRow(st, ac, s, "trailLife", Glyph::Clock, "Время жизни следа", st.graphics.trail.life, 0.2f, 4.0f, "с", 1, "graphics.trail.life");
    }

    void JumpRingsFields(MenuState& st, const MenuActions& ac, float s)
    {
        ToggleRow(st, ac, s, "jumpEnabled", Glyph::Sparkles, "Включение эффекта", st.graphics.jumpRings.enabled, "graphics.jumpRings.enabled");
        ColorRow(st, ac, s, "jumpColor", "Цвет", st.graphics.jumpRings.color, "graphics.jumpRings.color");
        ToggleRow(st, ac, s, "jumpRainbow", Glyph::Sparkles, "Радужный режим", st.graphics.jumpRings.rainbow, "graphics.jumpRings.rainbow");
        SliderRow(st, ac, s, "jumpRainSpeed", Glyph::Activity, "Скорость радуги", st.graphics.jumpRings.rainbowSpeed, 0.0f, 100.0f, "%", 0, "graphics.jumpRings.rainbowSpeed");
        SliderRow(st, ac, s, "jumpRadius", Glyph::Grid, "Радиус", st.graphics.jumpRings.radius, 0.2f, 3.0f, "", 2, "graphics.jumpRings.radius");
        SliderRow(st, ac, s, "jumpOpacity", Glyph::Eye, "Прозрачность", st.graphics.jumpRings.opacity, 0.0f, 100.0f, "%", 0, "graphics.jumpRings.opacity");
        SliderRow(st, ac, s, "jumpDuration", Glyph::Clock, "Длительность", st.graphics.jumpRings.duration, 0.1f, 3.0f, "с", 1, "graphics.jumpRings.duration");
    }

    void RenderGraphics(MenuState& st, const MenuActions& ac, float s)
    {
        SectionHeader(st, s, Glyph::Graphics, "Графика", "China Hat, Trail и Jump Rings", "ВИЗУАЛ");
        NoteText(s, "Настройки отвечают только за внешний вид. Рендер эффектов и модель персонажа подключаются клиентом позже.");
        BeginCard(st, s, "chinaCard", "China Hat", "эффект над головой");
        ChinaHatFields(st, ac, s);
        EndCard(s);
        BeginCard(st, s, "trailCard", "Trail", "след движения");
        TrailFields(st, ac, s);
        EndCard(s);
        BeginCard(st, s, "jumpCard", "Jump Rings", "эффект прыжка");
        JumpRingsFields(st, ac, s);
        EndCard(s);
        if (PaintButton(st, s, "previewTexture", "Передать область предпросмотра", true, 0.0f, 35.0f, Glyph::Image))
        {
            if (ac.previewTextureRequested)
                ac.previewTextureRequested();
        }
        ImGui::SameLine(0.0f, 8.0f * s);
        if (PaintButton(st, s, "graphicsReset", "Сбросить", false, 0.0f, 35.0f, Glyph::Reset))
        {
            if (ac.resetGraphicsRequested)
                ac.resetGraphicsRequested();
        }
        NoteText(s, "Панель справа принимает ImTextureID через MenuState::preview.texture. Без текстуры показывается заглушка.");
    }

    // ─────────────────────────────────────────────────────────────────────────
    // Звук, управление, прочее
    // ─────────────────────────────────────────────────────────────────────────
    void RenderSound(MenuState& st, const MenuActions& ac, float s)
    {
        SectionHeader(st, s, Glyph::Volume, "Звук", "Визуальные настройки громкости", "ДЕМО");
        BeginCard(st, s, "soundMain", "Основные настройки", "без аудио");
        ToggleRow(st, ac, s, "soundMute", Glyph::Volume, "Отключить звук", st.sound.muted, "sound.muted");
        SliderRow(st, ac, s, "soundMaster", Glyph::Volume, "Общая громкость", st.sound.master, 0.0f, 100.0f, "%", 0, "sound.master");
        SliderRow(st, ac, s, "soundUi", Glyph::Mouse, "Громкость интерфейса", st.sound.ui, 0.0f, 100.0f, "%", 0, "sound.ui");
        SliderRow(st, ac, s, "soundEffects", Glyph::Activity, "Громкость эффектов", st.sound.effects, 0.0f, 100.0f, "%", 0, "sound.effects");
        EndCard(s);
        BeginCard(st, s, "soundDevice", "Устройство вывода", "система");
        const char* devices[] = { "Системное устройство", "Устройство по умолчанию", "Без вывода" };
        ComboRow(st, ac, s, "soundDeviceCombo", Glyph::Monitor, "Аудиоустройство", st.sound.device, devices, IM_ARRAYSIZE(devices), "sound.device");
        EndCard(s);
        if (PaintButton(st, s, "soundCheck", "Проверить звук", true, 0.0f, 35.0f, Glyph::Volume))
        {
            Notify(ac, "sound.test");
        }
        ImGui::SameLine(0.0f, 8.0f * s);
        if (PaintButton(st, s, "soundReset", "Сбросить", false, 0.0f, 35.0f, Glyph::Reset))
        {
            if (ac.resetSoundRequested)
                ac.resetSoundRequested();
        }
        NoteText(s, "Аудиосистема клиента не подключена: ползунки меняют только демонстрационное состояние.");
    }

    void RenderControls(MenuState& st, const MenuActions& ac, float s)
    {
        SectionHeader(st, s, Glyph::Sliders, "Управление", "Клавиши, курсор и прокрутка меню", "ДЕМО");
        BeginCard(st, s, "controlsKeys", "Клавиши", "по умолчанию");
        RowBox row = BeginSettingRow(s, "menuKeyRow", 40.0f);
        RowLabel(st, s, row, Glyph::Keyboard, "Открытие и закрытие меню", "Right Shift по умолчанию");
        PlaceRowControl(row, s, 96.0f, 23.0f);
        {
            const ImVec2 keyPos = ImGui::GetCursorScreenPos();
            ImDrawList* draw = ImGui::GetWindowDrawList();
            draw->AddRectFilled(keyPos, ImVec2(keyPos.x + 96.0f * s, keyPos.y + 23.0f * s), Color(0.03f, 0.12f, 0.06f, 1.0f), 5.0f * s);
            draw->AddRect(keyPos, ImVec2(keyPos.x + 96.0f * s, keyPos.y + 23.0f * s), Accent(st, 0.3f), 5.0f * s, 0, 1.0f);
            TextAt(draw, ImVec2(keyPos.x + 9.0f * s, keyPos.y + (23.0f * s - TextLine(0.60f)) * 0.5f),
                   Color(0.75f, 0.90f, 0.79f, 1.0f), st.controls.menuKey.c_str(), 0.60f);
        }
        EndSettingRow(row, s);
        ToggleRow(st, ac, s, "hotkeys", Glyph::Keyboard, "Горячие клавиши модулей", st.controls.moduleHotkeys, "controls.moduleHotkeys");
        ToggleRow(st, ac, s, "menuCursor", Glyph::Mouse, "Курсор при открытом меню", st.controls.cursorVisible, "controls.cursorVisible");
        ToggleRow(st, ac, s, "menuScroll", Glyph::Sliders, "Прокрутка содержимого", st.controls.scrollEnabled, "controls.scrollEnabled");
        EndCard(s);
        BeginCard(st, s, "controlsInfo", "Назначение по умолчанию", nullptr);
        const char* rows[][2] = {
            { "Открыть / закрыть меню", "Right Shift" },
            { "Фокус на содержимом", "Автоматически" },
            { "Захват игровой мыши", "Не подключён" }
        };
        for (const auto& pair : rows)
        {
            RowBox infoRow = BeginSettingRow(s, pair[0], 36.0f);
            RowLabel(st, s, infoRow, Glyph::Keyboard, pair[0]);
            const ImVec2 valueSize = Measure(pair[1], 0.66f);
            TextAt(ImGui::GetWindowDrawList(),
                   ImVec2(infoRow.start.x + infoRow.width - valueSize.x - 10.0f * s,
                          infoRow.start.y + (infoRow.height - TextLine(0.66f)) * 0.5f),
                   Color(0.55f, 0.70f, 0.59f, 1.0f), pair[1], 0.66f);
            EndSettingRow(infoRow, s);
        }
        EndCard(s);
        if (PaintButton(st, s, "controlsReset", "Вернуть по умолчанию", true, 0.0f, 35.0f, Glyph::Reset))
        {
            if (ac.resetControlsRequested)
                ac.resetControlsRequested();
        }
        NoteText(s, "Клавиша Right Shift показана как назначение по умолчанию. Перехват клавиш и игровой мыши реализует основной клиент.");
    }

    void RenderOther(MenuState& st, const MenuActions& ac, float s)
    {
        SectionHeader(st, s, Glyph::Dots, "Прочее", "Сведения о клиенте и общий сброс", "1.1.5");
        BeginCard(st, s, "otherInfo", "Сведения", nullptr);
        const char* rows[][2] = {
            { "Название клиента", "GAYClient" },
            { "Версия", "1.1.5" },
            { "Платформа", "Minecraft Bedrock / Windows" },
            { "Состояние", "Демонстрационный интерфейс" },
            { "Подключение", "Нет данных" }
        };
        for (const auto& pair : rows)
        {
            RowBox row = BeginSettingRow(s, pair[0], 36.0f);
            RowLabel(st, s, row, Glyph::Info, pair[0]);
            const ImVec2 valueSize = Measure(pair[1], 0.66f);
            TextAt(ImGui::GetWindowDrawList(),
                   ImVec2(row.start.x + row.width - valueSize.x - 10.0f * s,
                          row.start.y + (row.height - TextLine(0.66f)) * 0.5f),
                   Color(0.55f, 0.70f, 0.59f, 1.0f), pair[1], 0.66f);
            EndSettingRow(row, s);
        }
        EndCard(s);
        BeginCard(st, s, "otherReset", "Сброс", nullptr);
        NoteText(s, "Сброс вернёт демонстрационные значения HUD, оформления, эффектов, звука и управления. Профили останутся без изменений.");
        if (PaintButton(st, s, "resetAll", "Сбросить все настройки", false, 0.0f, 35.0f, Glyph::Reset))
        {
            if (ac.resetAllRequested)
                ac.resetAllRequested();
        }
        EndCard(s);
        NoteText(s, "Игровые модули, хуки и рендер намеренно не подключены: эта часть отвечает только за визуальный интерфейс.");
    }

    // ─────────────────────────────────────────────────────────────────────────
    // Моды
    // ─────────────────────────────────────────────────────────────────────────
    bool ContainsNoCase(const char* haystack, const char* needle)
    {
        if (needle == nullptr || *needle == '\0')
            return true;
        std::string source(haystack != nullptr ? haystack : "");
        std::string query(needle);
        std::transform(source.begin(), source.end(), source.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        std::transform(query.begin(), query.end(), query.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        return source.find(query) != std::string::npos;
    }

    void RenderMods(MenuState& st, const MenuActions& ac, float s)
    {
        SectionHeader(st, s, Glyph::Cube, "Моды", "Демонстрационный список модулей клиента", "КАТАЛОГ");

        ImGui::SetNextItemWidth(-1.0f);
        ImGui::InputTextWithHint("##modSearch", "Поиск по названию...", st.moduleSearch, IM_ARRAYSIZE(st.moduleSearch));

        const char* categories[] = { "Все", "HUD", "Графика" };
        for (int i = 0; i < 3; ++i)
        {
            if (i > 0)
                ImGui::SameLine();
            ImGui::PushID(i);
            if (ImGui::Button(categories[i], ImVec2(0.0f, 26.0f * s)))
                st.moduleCategory = i;
            if (st.moduleCategory == i)
            {
                ImGui::GetWindowDrawList()->AddRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), Accent(st), 6.0f * s, 0, 1.5f);
            }
            ImGui::PopID();
        }
        ImGui::Dummy(ImVec2(0.0f, 2.0f * s));

        BeginCard(st, s, "modsList", "Список модулей", nullptr);
        for (std::size_t i = 0; i < st.modules.size(); ++i)
        {
            ModuleEntry& module = st.modules[i];
            if (st.moduleCategory > 0 && module.category != kModuleCategories[st.moduleCategory])
                continue;
            if (!ContainsNoCase(module.name.c_str(), st.moduleSearch) &&
                !ContainsNoCase(module.description.c_str(), st.moduleSearch))
                continue;

            const bool selected = static_cast<int>(i) == st.selectedModule;
            const ImVec2 rowStart = ImGui::GetCursorScreenPos();
            RowBox row = BeginSettingRow(s, module.name.c_str(), 46.0f);
            if (selected)
            {
                ImGui::GetWindowDrawList()->AddRectFilled(
                    row.start, ImVec2(row.start.x + row.width, row.start.y + row.height), Accent(st, 0.085f), 7.0f * s);
                ImGui::GetWindowDrawList()->AddRect(
                    row.start, ImVec2(row.start.x + row.width, row.start.y + row.height), Accent(st, 0.22f), 7.0f * s, 0, 1.0f);
            }
            RowLabel(st, s, row, GlyphForModule(module.id), module.name.c_str());
            const ImVec2 descPos(row.start.x + 44.0f * s, row.start.y + 27.0f * s);
            TextAt(ImGui::GetWindowDrawList(), descPos, Color(0.40f, 0.54f, 0.44f, 1.0f), module.description.c_str(), 0.56f);
            PlaceRowControl(row, s, 46.0f, 25.0f);
            bool enabled = module.enabled;
            if (ToggleControl(st, s, enabled))
            {
                module.enabled = enabled;
                if (bool* flag = EnabledFlag(st, module.id))
                    *flag = enabled;
                if (ac.moduleToggled)
                    ac.moduleToggled(module.id, enabled);
                Notify(ac, ModuleEnabledPath(module.id));
            }
            EndSettingRow(row, s);
            if (ImGui::IsMouseClicked(0) && ImGui::IsMouseHoveringRect(rowStart,
                    ImVec2(rowStart.x + ImGui::GetContentRegionAvail().x, rowStart.y + 46.0f * s)))
            {
                st.selectedModule = static_cast<int>(i);
                if (ac.settingChanged)
                    ac.settingChanged("mods.selected");
            }
        }
        EndCard(s);

        BeginCard(st, s, "modsSettings", "Настройки выбранного модуля", "визуал");
        ModuleEntry* selected = st.SelectedModule();
        if (selected == nullptr)
        {
            NoteText(s, "Модуль не выбран.");
        }
        else
        {
            RowBox head = BeginSettingRow(s, "selectedModule", 42.0f);
            RowLabel(st, s, head, GlyphForModule(selected->id), selected->name.c_str(), selected->description.c_str());
            PlaceRowControl(head, s, 46.0f, 25.0f);
            bool enabled = selected->enabled;
            if (ToggleControl(st, s, enabled))
            {
                selected->enabled = enabled;
                if (bool* flag = EnabledFlag(st, selected->id))
                    *flag = enabled;
                if (ac.moduleToggled)
                    ac.moduleToggled(selected->id, enabled);
            }
            EndSettingRow(head, s);

            switch (selected->id)
            {
            case ModuleId::ChinaHat: ChinaHatFields(st, ac, s); break;
            case ModuleId::Trail: TrailFields(st, ac, s); break;
            case ModuleId::JumpRings: JumpRingsFields(st, ac, s); break;
            default:
                SliderRow(st, ac, s, "moduleScale", Glyph::Sliders, "Масштаб модуля", st.hud.scale, 0.5f, 2.0f, "x", 2, "hud.scale");
                SliderRow(st, ac, s, "moduleOpacity", Glyph::Eye, "Прозрачность", st.hud.opacity, 0.0f, 100.0f, "%", 0, "hud.opacity");
                ColorRow(st, ac, s, "moduleColor", "Цвет текста", st.hud.textColor, "hud.textColor");
                ComboRow(st, ac, s, "modulePosition", Glyph::Grid, "Положение", st.hud.position, kPositions, kPositionCount, "hud.position");
                break;
            }
        }
        EndCard(s);
        NoteText(s, "Список и настройки модулей демонстрационные. Реальные данные FPS, CPS, координат и системные показатели передаёт клиент.");
    }

    // ─────────────────────────────────────────────────────────────────────────
    // Профили
    // ─────────────────────────────────────────────────────────────────────────
    void RenderProfiles(MenuState& st, const MenuActions& ac, float s)
    {
        SectionHeader(st, s, Glyph::User, "Профили", "Наборы настроек интерфейса", "ДЕМО");
        static char createName[64] = {};
        static char renameName[64] = {};
        static int renameIndex = -1;

        BeginCard(st, s, "profileList", "Список профилей", "локальный список");
        for (int i = 0; i < static_cast<int>(st.profiles.size()); ++i)
        {
            Profile& profile = st.profiles[static_cast<std::size_t>(i)];
            const bool selected = st.selectedProfile == i;
            const ImVec2 p = ImGui::GetCursorScreenPos();
            const float width = ImGui::GetContentRegionAvail().x;
            ImGui::InvisibleButton(("##profile" + std::to_string(i)).c_str(), ImVec2(width, 48.0f * s));
            if (ImGui::IsItemClicked())
            {
                st.selectedProfile = i;
                if (ac.profileSelected)
                    ac.profileSelected(i);
            }
            ImDrawList* draw = ImGui::GetWindowDrawList();
            draw->AddRectFilled(p, ImVec2(p.x + width, p.y + 48.0f * s),
                                selected ? Accent(st, 0.09f) : Color(0.02f, 0.075f, 0.04f, 0.6f), 8.0f * s);
            draw->AddRect(p, ImVec2(p.x + width, p.y + 48.0f * s),
                          selected ? Accent(st, 0.38f) : Accent(st, 0.12f), 8.0f * s, 0, 1.0f);
            const ImVec2 avatar(p.x + 9.0f * s, p.y + 9.0f * s);
            draw->AddRectFilled(avatar, ImVec2(avatar.x + 29.0f * s, avatar.y + 29.0f * s), Accent(st, 0.10f), 6.0f * s);
            draw->AddRect(avatar, ImVec2(avatar.x + 29.0f * s, avatar.y + 29.0f * s), Accent(st, 0.24f), 6.0f * s, 0, 1.0f);
            DrawGlyph(draw, Glyph::User, ImVec2(avatar.x + 14.5f * s, avatar.y + 14.5f * s), 15.0f * s, Accent(st));
            TextAt(draw, ImVec2(p.x + 47.0f * s, p.y + 9.0f * s), Color(0.86f, 0.96f, 0.88f, 1.0f), profile.name.c_str(), 0.70f);
            TextAt(draw, ImVec2(p.x + 47.0f * s, p.y + 27.0f * s), Color(0.40f, 0.54f, 0.44f, 1.0f), profile.note.c_str(), 0.56f);
            if (profile.active)
            {
                const ImVec2 tagPos(p.x + width - 62.0f * s, p.y + 15.0f * s);
                draw->AddRectFilled(tagPos, ImVec2(tagPos.x + 52.0f * s, tagPos.y + 18.0f * s), Accent(st, 0.12f), 4.0f * s);
                draw->AddRect(tagPos, ImVec2(tagPos.x + 52.0f * s, tagPos.y + 18.0f * s), Accent(st, 0.3f), 4.0f * s, 0, 1.0f);
                TextAt(draw, ImVec2(tagPos.x + 6.0f * s, tagPos.y + 3.5f * s), Accent(st), "активен", 0.52f);
            }
            ImGui::SetCursorScreenPos(ImVec2(p.x, p.y + 52.0f * s));
        }
        if (ImGui::Button("Создать профиль", ImVec2(0.0f, 30.0f * s)))
        {
            createName[0] = '\0';
            ImGui::OpenPopup("##createProfile");
        }
        ImGui::SameLine(0.0f, 6.0f * s);
        if (ImGui::Button("Сохранить настройки", ImVec2(0.0f, 30.0f * s)))
        {
            if (ac.profileSaveRequested)
                ac.profileSaveRequested(st.selectedProfile);
        }
        EndCard(s);

        BeginCard(st, s, "profileActions", "Действия с профилем", "визуал");
        if (!st.profiles.empty())
        {
            const int index = std::clamp(st.selectedProfile, 0, static_cast<int>(st.profiles.size()) - 1);
            Profile& profile = st.profiles[static_cast<std::size_t>(index)];
            NoteText(s, "Выбор, создание, переименование и удаление меняют только демонстрационный список. Хранение профилей реализует клиент.");
            if (ImGui::Button("Сделать активным", ImVec2(0.0f, 30.0f * s)))
            {
                for (Profile& item : st.profiles)
                    item.active = false;
                profile.active = true;
                if (ac.profileActivated)
                    ac.profileActivated(index);
            }
            ImGui::SameLine(0.0f, 6.0f * s);
            if (ImGui::Button("Переименовать", ImVec2(0.0f, 30.0f * s)))
            {
                renameIndex = index;
                std::snprintf(renameName, sizeof(renameName), "%s", profile.name.c_str());
                ImGui::OpenPopup("##renameProfile");
            }
            ImGui::SameLine(0.0f, 6.0f * s);
            if (ImGui::Button("Удалить", ImVec2(0.0f, 30.0f * s)) && st.profiles.size() > 1)
            {
                if (ac.profileDeleted)
                    ac.profileDeleted(index);
                st.profiles.erase(st.profiles.begin() + index);
                st.selectedProfile = std::clamp(st.selectedProfile, 0, static_cast<int>(st.profiles.size()) - 1);
            }
        }
        EndCard(s);

        if (ImGui::BeginPopupModal("##createProfile", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::TextUnformatted("Новый профиль");
            ImGui::InputText("Название", createName, IM_ARRAYSIZE(createName));
            if (ImGui::Button("Создать") && createName[0] != '\0')
            {
                if (ac.profileCreated)
                    ac.profileCreated(createName);
                st.profiles.push_back(Profile{ createName, "Демонстрационный профиль", false });
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button("Отмена"))
                ImGui::CloseCurrentPopup();
            ImGui::EndPopup();
        }
        if (ImGui::BeginPopupModal("##renameProfile", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::TextUnformatted("Переименовать профиль");
            ImGui::InputText("Название", renameName, IM_ARRAYSIZE(renameName));
            if (ImGui::Button("Сохранить") && renameName[0] != '\0' && renameIndex >= 0 &&
                renameIndex < static_cast<int>(st.profiles.size()))
            {
                if (ac.profileRenamed)
                    ac.profileRenamed(renameIndex, renameName);
                st.profiles[static_cast<std::size_t>(renameIndex)].name = renameName;
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button("Отмена"))
                ImGui::CloseCurrentPopup();
            ImGui::EndPopup();
        }
    }

    // ─────────────────────────────────────────────────────────────────────────
    // Статистика
    // ─────────────────────────────────────────────────────────────────────────
    void StatCard(const MenuState& st, float s, float width, Glyph glyph, const char* title, const char* value, const char* foot)
    {
        const ImVec2 p = ImGui::GetCursorScreenPos();
        const float height = 78.0f * s;
        ImDrawList* draw = ImGui::GetWindowDrawList();
        draw->AddRectFilled(p, ImVec2(p.x + width, p.y + height), Color(0.03f, 0.11f, 0.06f, 0.76f), 9.0f * s);
        draw->AddRect(p, ImVec2(p.x + width, p.y + height), Accent(st, 0.17f), 9.0f * s, 0, 1.0f);
        draw->AddRectFilled(ImVec2(p.x + 8.0f * s, p.y + 8.0f * s), ImVec2(p.x + 30.0f * s, p.y + 30.0f * s),
                            Accent(st, 0.08f), 5.0f * s);
        DrawGlyph(draw, glyph, ImVec2(p.x + 19.0f * s, p.y + 19.0f * s), 12.0f * s, Accent(st, 0.9f));
        TextAt(draw, ImVec2(p.x + 36.0f * s, p.y + 12.0f * s), Color(0.47f, 0.62f, 0.51f, 1.0f), title, 0.58f);
        TextAt(draw, ImVec2(p.x + 9.0f * s, p.y + 38.0f * s), Color(0.40f, 0.52f, 0.43f, 1.0f), value, 0.86f);
        TextAt(draw, ImVec2(p.x + 9.0f * s, p.y + 60.0f * s), Color(0.36f, 0.49f, 0.39f, 1.0f), foot, 0.52f);
        ImGui::Dummy(ImVec2(width, height));
    }

    void RenderStatistics(MenuState& st, const MenuActions& ac, float s)
    {
        SectionHeader(st, s, Glyph::Chart, "Статистика", "Показатели, ожидающие подключения клиента", "НЕТ ДАННЫХ");
        const ImVec2 p = ImGui::GetCursorScreenPos();
        const float width = ImGui::GetContentRegionAvail().x;
        ImDrawList* draw = ImGui::GetWindowDrawList();
        draw->AddRectFilled(p, ImVec2(p.x + width, p.y + 34.0f * s), Color(0.20f, 0.15f, 0.045f, 0.35f), 7.0f * s);
        draw->AddRect(p, ImVec2(p.x + width, p.y + 34.0f * s), Color(0.85f, 0.70f, 0.30f, 0.22f), 7.0f * s, 0, 1.0f);
        DrawGlyph(draw, Glyph::Warning, ImVec2(p.x + 15.0f * s, p.y + 17.0f * s), 13.0f * s, Color(0.88f, 0.75f, 0.42f, 1.0f));
        TextAt(draw, ImVec2(p.x + 28.0f * s, p.y + 11.0f * s), Color(0.78f, 0.69f, 0.46f, 1.0f),
               "Клиент не подключён. Реальные показатели отсутствуют и не подменяются демонстрационными.", 0.56f);
        ImGui::Dummy(ImVec2(0.0f, 40.0f * s));

        int enabled = 0;
        for (const ModuleEntry& module : st.modules)
            enabled += module.enabled ? 1 : 0;

        BeginCard(st, s, "statsMain", "Показатели клиента", "обновление от клиента");
        const float cardWidth = (ImGui::GetContentRegionAvail().x - 6.0f * s) * 0.5f;
        StatCard(st, s, cardWidth, Glyph::Activity, "FPS", "Нет данных", "Источник: клиент игры");
        ImGui::SameLine(0.0f, 6.0f * s);
        StatCard(st, s, cardWidth, Glyph::Mouse, "CPS", "Нет данных", "Источник: клиент игры");
        char modulesValue[32];
        std::snprintf(modulesValue, sizeof(modulesValue), "%d", enabled);
        StatCard(st, s, cardWidth, Glyph::Layers, "Модулей включено", modulesValue, "демонстрационные переключатели");
        ImGui::SameLine(0.0f, 6.0f * s);
        StatCard(st, s, cardWidth, Glyph::Wifi, "Пинг", "Нет данных", "Если показатель доступен");
        StatCard(st, s, cardWidth, Glyph::Cpu, "Загрузка CPU", "Нет данных", "Системные показатели");
        ImGui::SameLine(0.0f, 6.0f * s);
        StatCard(st, s, cardWidth, Glyph::Cpu, "Использование RAM", "Нет данных", "Системные показатели");
        EndCard(s);

        BeginCard(st, s, "statsChart", "Динамика показателей", "ожидание");
        const ImVec2 chartPos = ImGui::GetCursorScreenPos();
        const float chartWidth = ImGui::GetContentRegionAvail().x;
        const float chartHeight = 64.0f * s;
        draw = ImGui::GetWindowDrawList();
        draw->AddRectFilled(chartPos, ImVec2(chartPos.x + chartWidth, chartPos.y + chartHeight),
                            Color(0.01f, 0.04f, 0.02f, 0.8f), 6.0f * s);
        draw->AddRect(chartPos, ImVec2(chartPos.x + chartWidth, chartPos.y + chartHeight), Accent(st, 0.10f), 6.0f * s, 0, 1.0f);
        for (int line = 1; line < 4; ++line)
        {
            const float y = chartPos.y + chartHeight * line / 4.0f;
            draw->AddLine(ImVec2(chartPos.x, y), ImVec2(chartPos.x + chartWidth, y), Accent(st, 0.055f), 1.0f);
        }
        const char* chartLabel = "Ожидание данных клиента";
        const ImVec2 chartLabelSize = Measure(chartLabel, 0.60f);
        TextAt(draw, ImVec2(chartPos.x + (chartWidth - chartLabelSize.x) * 0.5f,
                            chartPos.y + (chartHeight - TextLine(0.60f)) * 0.5f),
               Color(0.36f, 0.49f, 0.39f, 1.0f), chartLabel, 0.60f);
        ImGui::Dummy(ImVec2(0.0f, chartHeight + 4.0f * s));
        EndCard(s);

        if (PaintButton(st, s, "statsRefresh", "Запросить данные", true, 0.0f, 35.0f, Glyph::Reset))
        {
            if (ac.statsRefreshRequested)
                ac.statsRefreshRequested();
        }
        NoteText(s, "FPS, CPS, пинг и системные показатели появятся только после подключения клиента. Случайные значения не используются.");
    }

    // ─────────────────────────────────────────────────────────────────────────
    // Предпросмотр
    // ─────────────────────────────────────────────────────────────────────────
    void DrawHudMock(const MenuState& st, float s, ImVec2 panel, ImVec2 panelSize)
    {
        if (!(st.section == Section::Hud || st.section == Section::Home))
            return;
        ImDrawList* draw = ImGui::GetWindowDrawList();
        const float width = 152.0f * s;
        const float height = 84.0f * s;
        const float top = 36.0f * s;
        const float side = 14.0f * s;
        const float bottom = panel.y + panelSize.y - height - 14.0f * s;
        ImVec2 pos(panel.x + side, panel.y + top);
        switch (st.hud.position)
        {
        case 1: pos = ImVec2(panel.x + panelSize.x - width - side, panel.y + top); break;
        case 2: pos = ImVec2(panel.x + side, bottom); break;
        case 3: pos = ImVec2(panel.x + panelSize.x - width - side, bottom); break;
        case 4: pos = ImVec2(panel.x + panelSize.x * 0.5f - width * 0.5f,
                              panel.y + panelSize.y * 0.5f - height * 0.5f); break;
        default: pos = ImVec2(panel.x + side, panel.y + top); break;
        }

        draw->AddRectFilled(pos, ImVec2(pos.x + width, pos.y + height), Color(0.01f, 0.05f, 0.025f, 0.94f), 9.0f * s);
        GlowFrame(draw, pos, ImVec2(pos.x + width, pos.y + height), Accent(st, 0.32f), 9.0f * s, s, st.ui.glow);
        TextAt(draw, ImVec2(pos.x + 8.0f * s, pos.y + 6.0f * s), Color(0.47f, 0.62f, 0.51f, 1.0f), "HUD · ПРИМЕР", 0.46f);
        TextAt(draw, ImVec2(pos.x + width - 42.0f * s, pos.y + 6.0f * s), Color(0.36f, 0.49f, 0.39f, 1.0f), "НЕТ ДАННЫХ", 0.40f);
        draw->AddLine(ImVec2(pos.x + 8.0f * s, pos.y + 18.0f * s), ImVec2(pos.x + width - 8.0f * s, pos.y + 18.0f * s),
                      Accent(st, 0.14f), 1.0f);

        struct HudLine { bool visible; Glyph glyph; const char* label; const char* value; };
        const HudLine lines[] = {
            { st.hud.showFps, Glyph::Activity, "FPS", "—" },
            { st.hud.showCps, Glyph::Mouse, "CPS", "—" },
            { st.hud.showClock, Glyph::Clock, "Часы", "--:--" },
            { st.hud.showCoordinates, Glyph::Grid, "Координаты", "— / —" }
        };
        float y = pos.y + 23.0f * s;
        int shown = 0;
        for (const HudLine& line : lines)
        {
            if (!line.visible || shown >= 3)
                continue;
            DrawGlyph(draw, line.glyph, ImVec2(pos.x + 13.0f * s, y + 5.0f * s), 9.0f * s, Accent(st, 0.85f));
            TextAt(draw, ImVec2(pos.x + 22.0f * s, y - 1.0f * s), Color(0.58f, 0.73f, 0.62f, 1.0f), line.label, 0.50f);
            TextAt(draw, ImVec2(pos.x + width - 30.0f * s, y - 1.0f * s), Color(0.40f, 0.53f, 0.43f, 1.0f), line.value, 0.50f);
            y += 16.0f * s;
            ++shown;
        }
        if (shown == 0)
            TextAt(draw, ImVec2(pos.x + 8.0f * s, y), Color(0.36f, 0.49f, 0.39f, 1.0f), "Элементы выключены", 0.48f);
    }

    void PreviewPanel(MenuState& st, const MenuActions& ac, float s, ImVec2 panelPos, ImVec2 panelSize)
    {
        ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 15.0f * s);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(13.0f * s, 12.0f * s));
        ImGui::PushStyleColor(ImGuiCol_ChildBg, Color(0.022f, 0.085f, 0.044f, 0.80f));
        ImGui::PushStyleColor(ImGuiCol_Border, Accent(st, 0.22f));
        ImGui::SetCursorScreenPos(panelPos);
        ImGui::BeginChild("##previewPanel", panelSize, ImGuiChildFlags_Borders,
                          ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
        ImDrawList* draw = ImGui::GetWindowDrawList();

        const ImVec2 headerPos = ImGui::GetCursorScreenPos();
        const float contentWidth = ImGui::GetContentRegionAvail().x;
        draw->AddRectFilled(headerPos, ImVec2(headerPos.x + 28.0f * s, headerPos.y + 28.0f * s), Accent(st, 0.09f), 7.0f * s);
        draw->AddRect(headerPos, ImVec2(headerPos.x + 28.0f * s, headerPos.y + 28.0f * s), Accent(st, 0.24f), 7.0f * s, 0, 1.0f);
        DrawGlyph(draw, Glyph::Eye, ImVec2(headerPos.x + 14.0f * s, headerPos.y + 14.0f * s), 16.0f * s, Accent(st));
        TextAt(draw, ImVec2(headerPos.x + 37.0f * s, headerPos.y - 1.0f * s), Color(0.94f, 1.0f, 0.96f, 1.0f),
               "Предпросмотр", 0.96f);
        TextAt(draw, ImVec2(headerPos.x + 38.0f * s, headerPos.y + 18.0f * s), Color(0.42f, 0.56f, 0.46f, 1.0f),
               st.section == Section::Graphics ? "Область для будущего рендера" : "Условное расположение элементов HUD", 0.58f);
        ImGui::SetCursorScreenPos(ImVec2(headerPos.x + contentWidth - 28.0f * s, headerPos.y + 1.0f * s));
        if (ImGui::InvisibleButton("##expandPreview", ImVec2(27.0f * s, 27.0f * s)))
        {
            if (ac.previewTextureRequested)
                ac.previewTextureRequested();
        }
        draw->AddRectFilled(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), Color(0.03f, 0.10f, 0.055f, 0.8f), 6.0f * s);
        draw->AddRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), Accent(st, 0.18f), 6.0f * s, 0, 1.0f);
        DrawGlyph(draw, Glyph::Image, ImVec2(ImGui::GetItemRectMin().x + 13.5f * s, ImGui::GetItemRectMin().y + 13.5f * s),
                  13.0f * s, Accent(st, 0.7f));

        const float tipHeight = 76.0f * s;
        const float noteHeight = 18.0f * s;
        const float stageTop = headerPos.y + 38.0f * s;
        const float stageHeight = std::max(180.0f * s,
            panelSize.y - (stageTop - panelPos.y) - tipHeight - noteHeight - 44.0f * s);
        const ImVec2 stagePos(headerPos.x, stageTop);
        const ImVec2 stageSize(contentWidth, stageHeight);
        draw->AddRectFilled(stagePos, ImVec2(stagePos.x + stageSize.x, stagePos.y + stageSize.y),
                            Color(0.002f, 0.016f, 0.008f, 1.0f), 11.0f * s);
        GlowFrame(draw, stagePos, ImVec2(stagePos.x + stageSize.x, stagePos.y + stageSize.y),
                  Accent(st, st.ui.glow ? 0.55f : 0.35f), 11.0f * s, s, st.ui.glow, st.ui.glowIntensity / 55.0f);
        draw->AddRect(ImVec2(stagePos.x + 7.0f * s, stagePos.y + 7.0f * s),
                      ImVec2(stagePos.x + stageSize.x - 7.0f * s, stagePos.y + stageSize.y - 7.0f * s), Accent(st, 0.09f),
                      7.0f * s, 0, 1.0f);

        // Мягкое декоративное свечение в центре области предпросмотра.
        {
            const ImVec2 center(stagePos.x + stageSize.x * 0.5f, stagePos.y + stageSize.y * 0.48f);
            const ImU32 halo = Accent(st, st.ui.glow ? 0.028f : 0.016f);
            for (int ring = 6; ring >= 1; --ring)
                draw->AddCircleFilled(center, ring * 23.0f * s, halo, 48);
        }

        // Угловые засечки области предпросмотра.
        {
            const float inset = 13.0f * s;
            const float tick = 15.0f * s;
            const ImU32 tickColor = Accent(st, 0.34f);
            const ImVec2 a(stagePos.x + inset, stagePos.y + inset);
            const ImVec2 b(stagePos.x + stageSize.x - inset, stagePos.y + stageSize.y - inset);
            draw->AddLine(a, ImVec2(a.x + tick, a.y), tickColor, 1.3f);
            draw->AddLine(a, ImVec2(a.x, a.y + tick), tickColor, 1.3f);
            draw->AddLine(ImVec2(b.x, a.y), ImVec2(b.x - tick, a.y), tickColor, 1.3f);
            draw->AddLine(ImVec2(b.x, a.y), ImVec2(b.x, a.y + tick), tickColor, 1.3f);
            draw->AddLine(ImVec2(a.x, b.y), ImVec2(a.x + tick, b.y), tickColor, 1.3f);
            draw->AddLine(ImVec2(a.x, b.y), ImVec2(a.x, b.y - tick), tickColor, 1.3f);
            draw->AddLine(b, ImVec2(b.x - tick, b.y), tickColor, 1.3f);
            draw->AddLine(b, ImVec2(b.x, b.y - tick), tickColor, 1.3f);
        }

        const float gridStep = 34.0f * s;
        for (float x = stagePos.x + gridStep; x < stagePos.x + stageSize.x; x += gridStep)
            draw->AddLine(ImVec2(x, stagePos.y + 8.0f * s), ImVec2(x, stagePos.y + stageSize.y - 8.0f * s),
                          Accent(st, 0.045f), 1.0f);
        for (float y = stagePos.y + gridStep; y < stagePos.y + stageSize.y; y += gridStep)
            draw->AddLine(ImVec2(stagePos.x + 8.0f * s, y), ImVec2(stagePos.x + stageSize.x - 8.0f * s, y),
                          Accent(st, 0.045f), 1.0f);

        if (HasTexture(st))
        {
            draw->AddImage(st.preview.texture, ImVec2(stagePos.x + 7.0f * s, stagePos.y + 7.0f * s),
                           ImVec2(stagePos.x + stageSize.x - 7.0f * s, stagePos.y + stageSize.y - 7.0f * s));
        }
        else
        {
            const char* title = st.section == Section::Graphics ? "Предпросмотр эффектов недоступен" : "Предпросмотр недоступен";
            const char* body = st.section == Section::Graphics
                ? "Здесь будет изображение персонажа или сцены, переданное клиентом."
                : "Область готова принять изображение или текстуру из существующего клиента.";
            const ImVec2 titleSize = Measure(title, 0.82f);
            const ImVec2 bodySize = Measure(body, 0.56f);
            const float centerX = stagePos.x + stageSize.x * 0.5f;
            const float centerY = stagePos.y + stageSize.y * 0.5f;
            const ImVec2 iconPos(centerX - 22.0f * s, centerY - 66.0f * s);
            draw->AddRectFilled(iconPos, ImVec2(iconPos.x + 44.0f * s, iconPos.y + 44.0f * s), Color(0.02f, 0.09f, 0.045f, 0.9f), 10.0f * s);
            draw->AddRect(iconPos, ImVec2(iconPos.x + 44.0f * s, iconPos.y + 44.0f * s), Accent(st, 0.25f), 10.0f * s, 0, 1.0f);
            DrawGlyph(draw, Glyph::Image, ImVec2(iconPos.x + 22.0f * s, iconPos.y + 22.0f * s), 22.0f * s, Accent(st, 0.6f));
            TextAt(draw, ImVec2(centerX - titleSize.x * 0.5f, centerY - 10.0f * s),
                   Color(0.71f, 0.85f, 0.75f, 1.0f), title, 0.82f);
            TextAt(draw, ImVec2(centerX - bodySize.x * 0.5f, centerY + 10.0f * s),
                   Color(0.44f, 0.58f, 0.47f, 1.0f), body, 0.56f);
        }

        draw->AddRectFilled(ImVec2(stagePos.x + 14.0f * s, stagePos.y + 13.0f * s),
                            ImVec2(stagePos.x + 93.0f * s, stagePos.y + 31.0f * s), Color(0.01f, 0.055f, 0.027f, 0.88f), 4.0f * s);
        draw->AddRect(ImVec2(stagePos.x + 14.0f * s, stagePos.y + 13.0f * s),
                      ImVec2(stagePos.x + 93.0f * s, stagePos.y + 31.0f * s), Accent(st, 0.25f), 4.0f * s, 0, 1.0f);
        TextAt(draw, ImVec2(stagePos.x + 22.0f * s, stagePos.y + 16.0f * s), Accent(st, 0.85f), "ДЕМОНСТРАЦИЯ", 0.46f);
        {
            const char* scaleLabel = "16:9 - ПРЕДПРОСМОТР";
            const ImVec2 scaleSize = Measure(scaleLabel, 0.44f);
            TextAt(draw, ImVec2(stagePos.x + stageSize.x - scaleSize.x - 14.0f * s, stagePos.y + 16.0f * s),
                   Color(0.35f, 0.48f, 0.38f, 1.0f), scaleLabel, 0.44f);
        }
        DrawHudMock(st, s, stagePos, stageSize);

        const ImVec2 tipPos(stagePos.x, stagePos.y + stageSize.y + 10.0f * s);
        const ImVec2 tipSize(stageSize.x, 66.0f * s);
        draw->AddRectFilled(tipPos, ImVec2(tipPos.x + tipSize.x, tipPos.y + tipSize.y),
                            Color(0.035f, 0.13f, 0.07f, 0.80f), 10.0f * s);
        draw->AddRect(tipPos, ImVec2(tipPos.x + tipSize.x, tipPos.y + tipSize.y), Accent(st, 0.25f), 10.0f * s, 0, 1.0f);
        const ImVec2 bulbPos(tipPos.x + 11.0f * s, tipPos.y + 12.0f * s);
        draw->AddRectFilled(bulbPos, ImVec2(bulbPos.x + 29.0f * s, bulbPos.y + 29.0f * s), Accent(st, 0.08f), 7.0f * s);
        draw->AddRect(bulbPos, ImVec2(bulbPos.x + 29.0f * s, bulbPos.y + 29.0f * s), Accent(st, 0.22f), 7.0f * s, 0, 1.0f);
        DrawGlyph(draw, Glyph::Bolt, ImVec2(bulbPos.x + 14.5f * s, bulbPos.y + 14.5f * s), 15.0f * s, Accent(st));
        TextAt(draw, ImVec2(tipPos.x + 49.0f * s, tipPos.y + 10.0f * s), Color(0.80f, 0.94f, 0.83f, 1.0f), "Совет", 0.68f);
        const char* tip = st.section == Section::Graphics
            ? "Модель и рендер не имитируются. Текстура передаётся через ImTextureID."
            : "Измените положение HUD, чтобы он не перекрывал важные элементы игры.";
        TextAt(draw, ImVec2(tipPos.x + 49.0f * s, tipPos.y + 28.0f * s), Color(0.44f, 0.58f, 0.47f, 1.0f), tip, 0.56f);
        if (st.ui.glow)
            DrawPixelSpray(draw, ImVec2(tipPos.x + tipSize.x - 31.0f * s, tipPos.y + 12.0f * s), s, Accent(st, 0.16f));

        TextAt(draw, ImVec2(stagePos.x, tipPos.y + 74.0f * s), Color(0.30f, 0.42f, 0.33f, 1.0f),
               st.preview.label.c_str(), 0.48f);
        const char* rightNote = "ПАНЕЛЬ HUD · БЕЗ ИГРОВЫХ ДАННЫХ";
        TextAt(draw, ImVec2(stagePos.x + stageSize.x - Measure(rightNote, 0.44f).x, tipPos.y + 74.0f * s),
               Color(0.30f, 0.42f, 0.33f, 1.0f), rightNote, 0.44f);

        ImGui::EndChild();
        ImGui::PopStyleColor(2);
        ImGui::PopStyleVar(2);
    }

    // ─────────────────────────────────────────────────────────────────────────
    // Диспетчер содержимого
    // ─────────────────────────────────────────────────────────────────────────
    void RenderContent(MenuState& st, const MenuActions& ac, float s)
    {
        switch (st.tab)
        {
        case Tab::Settings:
            switch (st.section)
            {
            case Section::Home: RenderHome(st, ac, s); break;
            case Section::Hud: RenderHud(st, ac, s); break;
            case Section::Interface: RenderInterface(st, ac, s); break;
            case Section::Graphics: RenderGraphics(st, ac, s); break;
            case Section::Sound: RenderSound(st, ac, s); break;
            case Section::Controls: RenderControls(st, ac, s); break;
            case Section::Other: RenderOther(st, ac, s); break;
            }
            break;
        case Tab::Mods: RenderMods(st, ac, s); break;
        case Tab::Profiles: RenderProfiles(st, ac, s); break;
        case Tab::Statistics: RenderStatistics(st, ac, s); break;
        }
    }

    const char* SectionTitle(Section section)
    {
        switch (section)
        {
        case Section::Home: return "Главная";
        case Section::Hud: return "HUD";
        case Section::Interface: return "Интерфейс";
        case Section::Graphics: return "Графика";
        case Section::Sound: return "Звук";
        case Section::Controls: return "Управление";
        case Section::Other: return "Прочее";
        }
        return "Главная";
    }

    const char* HeaderTitle(const MenuState& state)
    {
        switch (state.tab)
        {
        case Tab::Settings: return SectionTitle(state.section);
        case Tab::Mods: return "Моды";
        case Tab::Profiles: return "Профили";
        case Tab::Statistics: return "Статистика";
        }
        return "GAYClient";
    }

    const char* HeaderEyebrow(const MenuState& state)
    {
        switch (state.tab)
        {
        case Tab::Settings: return "GAYCLIENT / НАСТРОЙКИ";
        case Tab::Mods: return "GAYCLIENT / КАТАЛОГ";
        case Tab::Profiles: return "GAYCLIENT / ПРОФИЛИ";
        case Tab::Statistics: return "GAYCLIENT / ДАННЫЕ";
        }
        return "GAYCLIENT";
    }

    void DrawFooter(const MenuState& st, float s, ImVec2 windowPos, ImVec2 windowSize)
    {
        ImDrawList* draw = ImGui::GetWindowDrawList();
        const float y = windowPos.y + windowSize.y - 28.0f * s;
        const float x = windowPos.x + kRailWidth * s + 16.0f * s;
        draw->AddRectFilled(ImVec2(x, y + 2.0f * s), ImVec2(x + 58.0f * s, y + 18.0f * s),
                            Color(0.03f, 0.12f, 0.06f, 0.85f), 4.0f * s);
        draw->AddRect(ImVec2(x, y + 2.0f * s), ImVec2(x + 58.0f * s, y + 18.0f * s), Accent(st, 0.22f), 4.0f * s, 0, 1.0f);
        TextAt(draw, ImVec2(x + 6.0f * s, y + 5.0f * s), Color(0.58f, 0.73f, 0.62f, 1.0f), "Right Shift", 0.50f);
        TextAt(draw, ImVec2(x + 65.0f * s, y + 4.0f * s), Color(0.31f, 0.43f, 0.34f, 1.0f),
               "Открыть меню  /  ЛКМ - выбрать  /  Колесо - прокрутка", 0.50f);

        const char* meta = "GAYClient 1.1.5  /  демонстрационный интерфейс";
        const ImVec2 metaSize = Measure(meta, 0.48f);
        const float metaX = windowPos.x + windowSize.x - 16.0f * s - metaSize.x;
        draw->AddCircleFilled(ImVec2(metaX - 8.0f * s, y + 9.0f * s), 2.0f * s, Accent(st, 0.6f), 8);
        TextAt(draw, ImVec2(metaX, y + 4.0f * s), Color(0.29f, 0.41f, 0.32f, 1.0f), meta, 0.48f);
    }
} // namespace

// ─────────────────────────────────────────────────────────────────────────────
// Публичный API
// ─────────────────────────────────────────────────────────────────────────────
bool LoadCyrillicFont(ImGuiIO& io, float pixelSize)
{
    const char* candidates[] = {
        "C:/Windows/Fonts/segoeui.ttf",
        "C:/Windows/Fonts/tahoma.ttf",
        "C:/Windows/Fonts/arial.ttf",
        "/System/Library/Fonts/Supplemental/Arial.ttf",
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
        "/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf"
    };

    ImFontConfig config;
    config.OversampleH = 2;
    config.OversampleV = 1;
    config.PixelSnapH = true;
    config.GlyphRanges = io.Fonts->GetGlyphRangesCyrillic();

    for (const char* path : candidates)
    {
        FILE* file = nullptr;
#if defined(_MSC_VER)
        if (fopen_s(&file, path, "rb") != 0)
            file = nullptr;
#else
        file = std::fopen(path, "rb");
#endif
        if (file == nullptr)
            continue;
        std::fclose(file);
        if (io.Fonts->AddFontFromFileTTF(path, pixelSize, &config) != nullptr)
            return true;
    }

    io.Fonts->AddFontDefault();
    return false;
}

void ApplyMenuStyle(const MenuState& state, float scale)
{
    ApplyStyleInternal(state, std::clamp(scale, 0.5f, 2.0f));
}

void SyncModuleStates(MenuState& state)
{
    for (ModuleEntry& module : state.modules)
    {
        if (const bool* flag = EnabledFlag(state, module.id))
            module.enabled = *flag;
    }
}

bool SetModuleEnabled(MenuState& state, ModuleId id, bool enabled)
{
    bool* flag = EnabledFlag(state, id);
    if (flag == nullptr)
        return false;
    *flag = enabled;
    SyncModuleStates(state);
    return true;
}

bool RenderMenu(MenuState& state, const MenuActions& actions)
{
    if (!state.visible)
        return false;

    ImGuiIO& io = ImGui::GetIO();
    if (io.DisplaySize.x < 200.0f || io.DisplaySize.y < 200.0f)
        return false;

    SyncModuleStates(state);

    // Масштабируем окно под размер игрового экрана, но не растягиваем на весь экран.
    float fit = std::min((io.DisplaySize.x - 40.0f) / kShellWidth, (io.DisplaySize.y - 40.0f) / kShellHeight);
    fit = std::clamp(fit, 0.55f, 1.80f);  // адаптер: было 1.30 — меньшему канвасу нужен больший предел
    const float userScale = std::clamp(state.ui.menuScale / 100.0f, 0.70f, 1.50f);  // адаптер: было 0.80..1.20 — чтобы ползунок реально работал в игре
    float windowWidth = kShellWidth * fit * userScale;
    float windowHeight = kShellHeight * fit * userScale;
    windowWidth = std::min(windowWidth, io.DisplaySize.x - 24.0f);
    windowHeight = std::min(windowHeight, io.DisplaySize.y - 24.0f);
    const float s = std::min(windowWidth / kShellWidth, windowHeight / kShellHeight);
    windowWidth = kShellWidth * s;
    windowHeight = kShellHeight * s;

    const ImGuiStyle backupStyle = ImGui::GetStyle();
    const float backupFontScale = io.FontGlobalScale;
    ApplyStyleInternal(state, s);
    io.FontGlobalScale = backupFontScale * s;

    ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f), ImGuiCond_Always,
                            ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(windowWidth, windowHeight), ImGuiCond_Always);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    const ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                                   ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
                                   ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse |
                                   ImGuiWindowFlags_NoSavedSettings;
    ImGui::Begin("GAYClient Menu", nullptr, flags);

    const ImVec2 windowPos = ImGui::GetWindowPos();
    const ImVec2 windowSize = ImGui::GetWindowSize();
    ImDrawList* draw = ImGui::GetWindowDrawList();

    // ── Оболочка: один фон, рамка и мягкое свечение ─────────────────────────
    const float shellRounding = 22.0f * s;
    draw->AddRectFilled(windowPos, ImVec2(windowPos.x + windowSize.x, windowPos.y + windowSize.y),
                        Color(0.012f, 0.048f, 0.025f, 0.98f), shellRounding);
    draw->AddRectFilledMultiColor(windowPos, ImVec2(windowPos.x + windowSize.x, windowPos.y + windowSize.y),
                                  Color(0.05f, 0.17f, 0.085f, 0.42f), Color(0.02f, 0.08f, 0.04f, 0.22f),
                                  Color(0.008f, 0.028f, 0.014f, 0.92f), Color(0.012f, 0.05f, 0.024f, 0.72f));
    if (state.ui.glow)
    {
        const float intensity = state.ui.glowIntensity / 55.0f;
        for (int i = 4; i >= 1; --i)
        {
            const float spread = i * 2.5f * s;
            draw->AddRect(ImVec2(windowPos.x - spread, windowPos.y - spread),
                          ImVec2(windowPos.x + windowSize.x + spread, windowPos.y + windowSize.y + spread),
                          Accent(state, 0.035f * intensity), shellRounding + spread, 0, 1.0f + i);
        }
    }
    draw->AddRect(windowPos, ImVec2(windowPos.x + windowSize.x, windowPos.y + windowSize.y),
                  Accent(state, 0.50f), shellRounding, 0, 1.4f);
    draw->AddRect(ImVec2(windowPos.x + 4.0f * s, windowPos.y + 4.0f * s),
                  ImVec2(windowPos.x + windowSize.x - 4.0f * s, windowPos.y + windowSize.y - 4.0f * s),
                  Accent(state, 0.08f), shellRounding - 4.0f * s, 0, 1.0f);

    // Декоративные угловые скобки по краям единого окна.
    {
        const float inset = 9.0f * s;
        const float topY = windowPos.y + 8.0f * s;
        const float bottomY = windowPos.y + windowSize.y - 8.0f * s;
        const float leftX = windowPos.x + inset;
        const float rightX = windowPos.x + windowSize.x - inset;
        const float length = 15.0f * s;
        const ImU32 cornerColor = Accent(state, 0.62f);
        const float thickness = 1.5f;
        draw->AddLine(ImVec2(leftX, topY), ImVec2(leftX + length, topY), cornerColor, thickness);
        draw->AddLine(ImVec2(leftX, topY), ImVec2(leftX, topY + length), cornerColor, thickness);
        draw->AddLine(ImVec2(rightX, topY), ImVec2(rightX - length, topY), cornerColor, thickness);
        draw->AddLine(ImVec2(rightX, topY), ImVec2(rightX, topY + length), cornerColor, thickness);
        draw->AddLine(ImVec2(leftX, bottomY), ImVec2(leftX + length, bottomY), cornerColor, thickness);
        draw->AddLine(ImVec2(leftX, bottomY), ImVec2(leftX, bottomY - length), cornerColor, thickness);
        draw->AddLine(ImVec2(rightX, bottomY), ImVec2(rightX - length, bottomY), cornerColor, thickness);
        draw->AddLine(ImVec2(rightX, bottomY), ImVec2(rightX, bottomY - length), cornerColor, thickness);
    }

    draw->AddLine(ImVec2(windowPos.x + kRailWidth * s, windowPos.y + 18.0f * s),
                  ImVec2(windowPos.x + kRailWidth * s, windowPos.y + windowSize.y - 18.0f * s),
                  Accent(state, 0.13f), 1.0f);
    DrawPixelSpray(draw, ImVec2(windowPos.x + windowSize.x - 30.0f * s, windowPos.y + windowSize.y - 26.0f * s), s,
                   Accent(state, 0.20f));

    // ── Левая панель ────────────────────────────────────────────────────────
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(16.0f * s, 16.0f * s));
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
    ImGui::SetCursorScreenPos(windowPos);
    ImGui::BeginChild("##rail", ImVec2(kRailWidth * s, windowSize.y), ImGuiChildFlags_None,
                      ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

    {
        const ImVec2 brandPos = ImGui::GetCursorScreenPos();
        const float brandWidth = ImGui::GetContentRegionAvail().x;
        const float brandHeight = 82.0f * s;
        ImGui::InvisibleButton("##brand", ImVec2(brandWidth, brandHeight));
        if (ImGui::IsItemClicked())
        {
            state.tab = Tab::Settings;
            state.section = Section::Home;
            if (actions.sectionChanged)
                actions.sectionChanged(Section::Home);
        }
        const ImVec2 bp = ImGui::GetItemRectMin();
        draw->AddRectFilled(bp, ImVec2(bp.x + brandWidth, bp.y + brandHeight), Color(0.035f, 0.14f, 0.07f, 0.86f),
                            13.0f * s);
        GlowFrame(draw, bp, ImVec2(bp.x + brandWidth, bp.y + brandHeight), Accent(state, 0.32f), 13.0f * s, s,
                  state.ui.glow);
        TextAt(draw, ImVec2(bp.x + 14.0f * s, bp.y + 14.0f * s), Color(0.95f, 1.0f, 0.96f, 1.0f), "GAYCLIENT", 1.12f);
        TextAt(draw, ImVec2(bp.x + 14.0f * s, bp.y + 40.0f * s), Accent(state, 0.8f), "ИГРОВОЙ КЛИЕНТ", 0.54f);
        const char* version = "BEDROCK 1.1.5";
        const ImVec2 versionSize = Measure(version, 0.48f);
        draw->AddRectFilled(ImVec2(bp.x + brandWidth - versionSize.x - 25.0f * s, bp.y + brandHeight - 26.0f * s),
                            ImVec2(bp.x + brandWidth - 10.0f * s, bp.y + brandHeight - 10.0f * s),
                            Accent(state, 0.07f), 4.0f * s);
        TextAt(draw, ImVec2(bp.x + brandWidth - versionSize.x - 17.0f * s, bp.y + brandHeight - 22.0f * s),
               Color(0.48f, 0.63f, 0.52f, 1.0f), version, 0.48f);
        ImGui::Dummy(ImVec2(0.0f, 4.0f * s));
    }

    TextAt(draw, ImVec2(ImGui::GetCursorScreenPos().x + 4.0f * s, ImGui::GetCursorScreenPos().y),
           Color(0.31f, 0.43f, 0.34f, 1.0f), "НАВИГАЦИЯ", 0.52f);
    ImGui::Dummy(ImVec2(0.0f, 18.0f * s));

    {
        const char* labels[] = { "Главная", "HUD", "Интерфейс", "Графика", "Звук", "Управление", "Прочее" };
        const Glyph glyphs[] = { Glyph::Home, Glyph::Monitor, Glyph::Palette, Glyph::Graphics,
                                 Glyph::Volume, Glyph::Sliders, Glyph::Dots };
        const Section sections[] = { Section::Home, Section::Hud, Section::Interface, Section::Graphics,
                                     Section::Sound, Section::Controls, Section::Other };
        for (int i = 0; i < 7; ++i)
        {
            const bool active = state.tab == Tab::Settings && state.section == sections[i];
            if (NavItem(state, s, labels[i], glyphs[i], active))
            {
                state.tab = Tab::Settings;
                state.section = sections[i];
                if (actions.sectionChanged)
                    actions.sectionChanged(sections[i]);
            }
        }

        // Группа "Клиент" намеренно удалена: "Моды", "Профили" и "Статистика"
        // не дублируются в боковой навигации. Слева только разделы настроек.
    }

    // ── Блок состояния: заполняет свободное место под навигацией ────────────
    if (windowPos.y + windowSize.y - 16.0f * s - ImGui::GetCursorScreenPos().y > 150.0f * s)
    {
        int enabledModules = 0;
        for (const ModuleEntry& module : state.modules)
            enabledModules += module.enabled ? 1 : 0;

        const float railWidth = kRailWidth * s - 32.0f * s;
        const float panelTop = ImGui::GetCursorScreenPos().y + 12.0f * s;
        const float panelBottom = windowPos.y + windowSize.y - 16.0f * s;
        const ImVec2 panelPos(windowPos.x + 16.0f * s, panelTop);
        const ImVec2 panelSize(railWidth, std::max(148.0f * s, panelBottom - panelTop));
        ImDrawList* panelDraw = ImGui::GetWindowDrawList();
        panelDraw->AddRectFilled(panelPos, ImVec2(panelPos.x + panelSize.x, panelPos.y + panelSize.y),
                                 Color(0.03f, 0.115f, 0.06f, 0.86f), 10.0f * s);
        panelDraw->AddRect(panelPos, ImVec2(panelPos.x + panelSize.x, panelPos.y + panelSize.y),
                           Accent(state, 0.20f), 10.0f * s, 0, 1.0f);
        panelDraw->AddLine(ImVec2(panelPos.x + 10.0f * s, panelPos.y),
                           ImVec2(panelPos.x + panelSize.x - 10.0f * s, panelPos.y), Accent(state, 0.45f), 1.0f);
        panelDraw->AddRectFilled(ImVec2(panelPos.x + 11.0f * s, panelPos.y + 11.0f * s),
                                 ImVec2(panelPos.x + 15.0f * s, panelPos.y + 15.0f * s), Accent(state), 1.0f);
        TextAt(panelDraw, ImVec2(panelPos.x + 21.0f * s, panelPos.y + 9.0f * s),
               Color(0.62f, 0.78f, 0.66f, 1.0f), "СОСТОЯНИЕ", 0.48f);
        TextAt(panelDraw, ImVec2(panelPos.x + panelSize.x - 36.0f * s, panelPos.y + 9.0f * s),
               Accent(state, 0.75f), "ДЕМО", 0.42f);

        const int segments = 14;
        const int activeSegments = enabledModules == 0
            ? 3
            : std::max(3, static_cast<int>(std::round(enabledModules / 12.0f * segments)));
        const float meterY = panelPos.y + 28.0f * s;
        const float meterWidth = panelSize.x - 22.0f * s;
        const float segmentWidth = (meterWidth - (segments - 1) * 2.0f * s) / segments;
        for (int i = 0; i < segments; ++i)
        {
            const float x = panelPos.x + 11.0f * s + i * (segmentWidth + 2.0f * s);
            const float height = (i % 4 == 0) ? 13.0f * s : 8.0f * s;
            panelDraw->AddRectFilled(ImVec2(x, meterY + (13.0f * s - height)),
                                     ImVec2(x + segmentWidth, meterY + 13.0f * s),
                                     i < activeSegments ? Accent(state, 0.85f) : Accent(state, 0.12f), 1.0f);
        }

        char modulesValue[32];
        std::snprintf(modulesValue, sizeof(modulesValue), "%d / %d", enabledModules,
                      static_cast<int>(state.modules.size()));
        const char* labels[] = { "Активно модулей", "Версия", "Данные игры" };
        const char* values[] = { modulesValue, "1.1.5", "нет связи" };
        for (int i = 0; i < 3; ++i)
        {
            const float rowY = panelPos.y + (48.0f + i * 16.0f) * s;
            TextAt(panelDraw, ImVec2(panelPos.x + 11.0f * s, rowY), Color(0.40f, 0.54f, 0.44f, 1.0f), labels[i], 0.48f);
            const ImVec2 valueSize = Measure(values[i], 0.46f);
            TextAt(panelDraw, ImVec2(panelPos.x + panelSize.x - valueSize.x - 11.0f * s, rowY),
                   i == 0 ? Color(0.72f, 0.88f, 0.76f, 1.0f) : Color(0.50f, 0.64f, 0.53f, 1.0f), values[i], 0.46f);
        }
        DrawPixelSpray(panelDraw, ImVec2(panelPos.x + panelSize.x - 29.0f * s, panelPos.y + 30.0f * s), s,
                       Accent(state, 0.20f));

        // Декоративный мини-график активности демонстрационных модулей.
        {
            const ImVec2 chartPos(panelPos.x + 11.0f * s, panelPos.y + 84.0f * s);
            const float chartHeight = std::max(30.0f * s, panelSize.y - 84.0f * s - 52.0f * s);
            const ImVec2 chartSize(panelSize.x - 22.0f * s, std::min(chartHeight, 170.0f * s));
            panelDraw->AddRectFilled(chartPos, ImVec2(chartPos.x + chartSize.x, chartPos.y + chartSize.y),
                                     Color(0.008f, 0.045f, 0.022f, 0.55f), 4.0f * s);
            panelDraw->AddRect(chartPos, ImVec2(chartPos.x + chartSize.x, chartPos.y + chartSize.y),
                               Accent(state, 0.08f), 4.0f * s, 0, 1.0f);
            const float points[] = { 0.62f, 0.44f, 0.52f, 0.28f, 0.38f, 0.16f, 0.30f, 0.12f, 0.22f, 0.18f, 0.08f };
            const int count = IM_ARRAYSIZE(points);
            ImVec2 previous(chartPos.x, chartPos.y + chartSize.y * points[0]);
            for (int i = 1; i < count; ++i)
            {
                const ImVec2 current(chartPos.x + chartSize.x * i / (count - 1),
                                     chartPos.y + chartSize.y * points[i]);
                panelDraw->AddLine(previous, current, Accent(state, 0.55f), 1.2f);
                previous = current;
            }
            TextAt(panelDraw, ImVec2(chartPos.x + 5.0f * s, chartPos.y + chartSize.y - 9.0f * s),
                   Accent(state, 0.35f), "АКТИВНОСТЬ - ДЕМО", 0.38f);
        }

        // Компактные переходы внутри блока состояния.
        const float chipsY = panelPos.y + panelSize.y - 34.0f * s;
        panelDraw->AddLine(ImVec2(panelPos.x + 11.0f * s, chipsY - 9.0f * s),
                           ImVec2(panelPos.x + panelSize.x - 11.0f * s, chipsY - 9.0f * s),
                           Accent(state, 0.12f), 1.0f);
        const char* chipLabels[] = { "HUD", "Эффекты", "Тема" };
        const Glyph chipGlyphs[] = { Glyph::Monitor, Glyph::Sparkles, Glyph::Palette };
        const Section chipSections[] = { Section::Hud, Section::Graphics, Section::Interface };
        const float chipGap = 4.0f * s;
        const float chipWidth = (panelSize.x - 22.0f * s - chipGap * 2.0f) / 3.0f;
        for (int i = 0; i < 3; ++i)
        {
            const ImVec2 chipPos(panelPos.x + 11.0f * s + i * (chipWidth + chipGap), chipsY);
            const ImVec2 chipSize(chipWidth, 26.0f * s);
            ImGui::SetCursorScreenPos(chipPos);
            ImGui::PushID(i);
            ImGui::InvisibleButton("##sideChip", chipSize);
            const bool hovered = ImGui::IsItemHovered();
            const bool clicked = ImGui::IsItemClicked();
            ImGui::PopID();
            panelDraw->AddRectFilled(chipPos, ImVec2(chipPos.x + chipSize.x, chipPos.y + chipSize.y),
                                     hovered ? Accent(state, 0.14f) : Accent(state, 0.045f), 5.0f * s);
            panelDraw->AddRect(chipPos, ImVec2(chipPos.x + chipSize.x, chipPos.y + chipSize.y),
                               Accent(state, hovered ? 0.36f : 0.14f), 5.0f * s, 0, 1.0f);
            const float labelWidth = Measure(chipLabels[i], 0.46f).x;
            const float contentWidth = labelWidth + 15.0f * s;
            const float contentX = chipPos.x + (chipSize.x - contentWidth) * 0.5f;
            DrawGlyph(panelDraw, chipGlyphs[i], ImVec2(contentX + 5.0f * s, chipPos.y + chipSize.y * 0.5f),
                      10.0f * s, Accent(state, hovered ? 1.0f : 0.75f));
            TextAt(panelDraw, ImVec2(contentX + 15.0f * s, chipPos.y + (chipSize.y - TextLine(0.46f)) * 0.5f),
                   hovered ? Color(0.88f, 1.0f, 0.92f, 1.0f) : Color(0.55f, 0.70f, 0.59f, 1.0f), chipLabels[i], 0.46f);
            if (clicked)
            {
                state.tab = Tab::Settings;
                state.section = chipSections[i];
                if (actions.sectionChanged)
                    actions.sectionChanged(chipSections[i]);
            }
        }
    }

    ImGui::EndChild();
    ImGui::PopStyleColor();
    ImGui::PopStyleVar();

    // ── Шапка окна без верхних вкладок ──────────────────────────────────────
    {
        const float startX = windowPos.x + kRailWidth * s + 16.0f * s;
        const float headerCenterY = windowPos.y + kTopbarHeight * s * 0.5f;
        const float markSize = 30.0f * s;
        const ImVec2 markPos(startX, headerCenterY - markSize * 0.5f);

        draw->AddRectFilled(markPos, ImVec2(markPos.x + markSize, markPos.y + markSize), Accent(state, 0.10f), 8.0f * s);
        draw->AddRect(markPos, ImVec2(markPos.x + markSize, markPos.y + markSize), Accent(state, 0.32f), 8.0f * s, 0, 1.0f);
        DrawGlyph(draw, Glyph::Shield, ImVec2(markPos.x + markSize * 0.5f, markPos.y + markSize * 0.5f), 16.0f * s,
                  Accent(state));

        TextAt(draw, ImVec2(markPos.x + 40.0f * s, headerCenterY - 16.0f * s),
               Color(0.37f, 0.51f, 0.41f, 1.0f), HeaderEyebrow(state), 0.48f);
        const char* title = HeaderTitle(state);
        TextAt(draw, ImVec2(markPos.x + 40.0f * s, headerCenterY - 2.0f * s),
               Color(0.95f, 1.0f, 0.96f, 1.0f), title, 0.94f);

        const float titleEnd = markPos.x + 40.0f * s + Measure(title, 0.94f).x + 10.0f * s;
        draw->AddRectFilled(ImVec2(titleEnd, headerCenterY - 9.0f * s), ImVec2(titleEnd + 37.0f * s, headerCenterY + 7.0f * s),
                            Accent(state, 0.08f), 4.0f * s);
        draw->AddRect(ImVec2(titleEnd, headerCenterY - 9.0f * s), ImVec2(titleEnd + 37.0f * s, headerCenterY + 7.0f * s),
                      Accent(state, 0.24f), 4.0f * s, 0, 1.0f);
        TextAt(draw, ImVec2(titleEnd + 8.0f * s, headerCenterY - 6.0f * s), Accent(state, 0.85f), "ДЕМО", 0.46f);

        const float chromeWidth = 3.0f * 27.0f * s + 2.0f * 2.0f * s + 10.0f * s;
        const float chromeX = windowPos.x + windowSize.x - 16.0f * s - chromeWidth;

        // Декоративная полоска из светящихся штрихов перед кнопками окна.
        const int ornamentWidths[] = { 26, 15, 8, 4, 2 };
        float ornamentX = chromeX - 18.0f * s;
        for (int i = 4; i >= 0; --i)
        {
            const float barWidth = ornamentWidths[i] * s;
            ornamentX -= barWidth;
            const ImU32 barColor = Accent(state, i == 2 ? 0.72f : 0.24f);
            draw->AddRectFilled(ImVec2(ornamentX, headerCenterY - 1.4f * s),
                                ImVec2(ornamentX + barWidth, headerCenterY + 1.4f * s), barColor, 1.0f * s);
            ornamentX -= 4.0f * s;
        }

        ImGui::SetCursorScreenPos(ImVec2(chromeX, headerCenterY - 13.5f * s));
        if (ChromeButton(state, s, Glyph::Minus, false))
        {
            Notify(actions, "window.minimize");
        }
        if (ChromeButton(state, s, Glyph::Grid, false))
        {
            state.ui.menuScale = 100.0f;
            Notify(actions, "window.fit");
        }
        if (ChromeButton(state, s, Glyph::Cross, true))
        {
            state.visible = false;
            if (actions.closeRequested)
                actions.closeRequested();
        }

        const float lineY = windowPos.y + kTopbarHeight * s - 1.0f;
        const float middle = windowPos.x + kRailWidth * s + (windowSize.x - kRailWidth * s) * 0.5f;
        draw->AddLine(ImVec2(startX, lineY), ImVec2(middle - 8.0f * s, lineY), Accent(state, 0.20f), 1.0f);
        draw->AddLine(ImVec2(middle + 8.0f * s, lineY), ImVec2(windowPos.x + windowSize.x - 16.0f * s, lineY),
                      Accent(state, 0.20f), 1.0f);
        draw->AddRectFilled(ImVec2(middle - 2.0f * s, lineY - 2.0f * s), ImVec2(middle + 2.0f * s, lineY + 2.0f * s),
                            Accent(state, 0.65f), 0.5f * s);
    }

    // ── Контент и предпросмотр ──────────────────────────────────────────────
    const float padX = 16.0f * s;
    const float gap = kColumnGap * s;
    const float contentTop = windowPos.y + kTopbarHeight * s;
    const float bodyHeight = windowSize.y - kTopbarHeight * s - kFooterHeight * s - 8.0f * s;
    const float workWidth = windowSize.x - kRailWidth * s - padX * 2.0f;
    const float contentWidth = (workWidth - gap) * kContentRatio;
    const float previewWidth = workWidth - gap - contentWidth;
    const ImVec2 contentPos(windowPos.x + kRailWidth * s + padX, contentTop);

    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 15.0f * s);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(14.0f * s, 14.0f * s));
    ImGui::PushStyleColor(ImGuiCol_ChildBg, Color(0.020f, 0.072f, 0.038f, 0.78f));
    ImGui::PushStyleColor(ImGuiCol_Border, Accent(state, 0.21f));
    ImGui::SetCursorScreenPos(contentPos);
    ImGui::BeginChild("##content", ImVec2(contentWidth, bodyHeight), ImGuiChildFlags_Borders,
                      ImGuiWindowFlags_None);
    RenderContent(state, actions, s);
    ImGui::EndChild();
    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar(2);

    PreviewPanel(state, actions, s, ImVec2(contentPos.x + contentWidth + gap, contentTop),
                 ImVec2(previewWidth, bodyHeight));

    DrawFooter(state, s, windowPos, windowSize);

    ImGui::End();
    ImGui::PopStyleVar();
    io.FontGlobalScale = backupFontScale;
    ImGui::GetStyle() = backupStyle;
    return true;
}

} // namespace gayclient
