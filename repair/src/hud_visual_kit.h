#pragma once
// Pure visual specifications for the redesigned HUD editor.
//
// VISUAL ONLY. Every struct describes how a control should look; none of them
// reads settings, draws anything or calls the game. compat.cpp converts these
// specs to ImGui style pushes and draw commands; integration stays elsewhere.
//
// Keeping the kit free of ImGui lets the developer review the look and lets
// Linux tests verify contrast and geometry without a Windows build.
#include <algorithm>
#include <cmath>
#include "hud_theme.h"

namespace hud_visual {

using hud_theme::Rgba;

struct ButtonSpec {
    Rgba background;
    Rgba border;
    Rgba text;
    float rounding;
    float height;
    float paddingX;
    bool outline;   // primary actions are filled, secondary are outlined
};

enum class ButtonTone { Primary, Secondary, Ghost, Danger };

inline ButtonSpec button(const hud_theme::Tokens& t, ButtonTone tone) {
    ButtonSpec s{t.panelSoft, t.border, t.text, t.roundingSmall, 26.f, 10.f, true};
    switch (tone) {
        case ButtonTone::Primary:
            s.background = hud_theme::withAlpha(t.accent,.18f);
            s.border = hud_theme::withAlpha(t.accent,.55f);
            s.text = t.text;
            s.outline = false;
            break;
        case ButtonTone::Secondary:
            s.background = hud_theme::withAlpha(t.accent,.08f);
            s.border = hud_theme::withAlpha(t.accent,.28f);
            break;
        case ButtonTone::Ghost:
            s.background = hud_theme::withAlpha(Rgba{1,1,1,1},.04f);
            s.border = t.borderSoft;
            s.text = t.textMuted;
            break;
        case ButtonTone::Danger:
            s.background = hud_theme::withAlpha(t.danger,.10f);
            s.border = hud_theme::withAlpha(t.danger,.32f);
            s.text = t.danger;
            break;
    }
    return s;
}

struct ToggleSpec {
    float width, height, knob, inset;
    Rgba trackOff, trackOn, knobOff, knobOn, borderOff, borderOn;
};
inline ToggleSpec toggle(const hud_theme::Tokens& t) {
    ToggleSpec s{26.f,15.f,11.f,2.f,
        hud_theme::withAlpha(Rgba{1,1,1,1},.09f),
        hud_theme::withAlpha(t.accent,.85f),
        hud_theme::rgba(225,238,228),
        hud_theme::rgba(6,28,15),
        hud_theme::withAlpha(Rgba{1,1,1,1},.14f),
        hud_theme::withAlpha(t.accent,.75f)};
    return s;
}

struct SliderSpec {
    float height, grab, rounding;
    Rgba track, fill, grabColor, grabBorder, text;
};
inline SliderSpec slider(const hud_theme::Tokens& t) {
    return {4.f,10.f,999.f,
        hud_theme::withAlpha(Rgba{1,1,1,1},.10f),
        t.accent,
        t.accent,
        hud_theme::withAlpha(t.accent,.40f),
        t.textMuted};
}

struct CardSpec {
    Rgba background, border, headerBar, accentBar;
    float rounding, borderSize, pad;
};
inline CardSpec card(const hud_theme::Tokens& t, bool nested = false) {
    return {nested ? t.panelSoft : t.panel,
        nested ? t.borderSoft : hud_theme::withAlpha(t.accent,.20f),
        hud_theme::withAlpha(Rgba{1,1,1,1},.035f),
        t.accentLine,
        t.rounding,
        t.borderSize,
        t.padSection};
}

struct SectionSpec {
    Rgba title, subtitle, accentBar;
    float gapBefore, gapAfter, barWidth, barHeight;
};
inline SectionSpec section(const hud_theme::Tokens& t) {
    return {t.text, t.textDim, t.accentLine, 8.f, 4.f, 2.f, 9.f};
}

struct PillSpec {
    Rgba background, border, text;
    float rounding, paddingX, paddingY;
};
inline PillSpec pill(const hud_theme::Tokens& t, bool accentPill = false) {
    return {accentPill ? hud_theme::withAlpha(t.accent,.12f) : hud_theme::withAlpha(Rgba{1,1,1,1},.04f),
        accentPill ? hud_theme::withAlpha(t.accent,.35f) : t.borderSoft,
        accentPill ? t.accent : t.textMuted,
        999.f, 7.f, 2.f};
}

struct StatusSpec { Rgba dot, text; float size; };
inline StatusSpec status(const hud_theme::Tokens& t, bool ok, bool warning = false) {
    if (warning) return {t.warning, t.warning, 4.f};
    return {ok ? t.success : t.textDim, ok ? t.success : t.textDim, 4.f};
}

struct SwatchSpec { float size, rounding, gap; Rgba selectedRing; };
inline SwatchSpec swatch(const hud_theme::Tokens& t) { return {14.f, 3.f, 5.f, t.accent}; }

// Readability: visual-only guard so text remains legible on transparent cards.
inline float contrastFloor() { return 4.5f; }
inline float linearChannel(float v) {
    v=hud_theme::clamp01(v);
    return v<=.04045f ? v/12.92f : std::pow((v+.055f)/1.055f,2.4f);
}
inline float luminance(const Rgba& c) {
    return .2126f*linearChannel(c.r)+.7152f*linearChannel(c.g)+.0722f*linearChannel(c.b);
}
inline bool readableOn(const Rgba& background, const Rgba& text) {
    // Composite translucent UI layers over black, a conservative reference;
    // real scene content can differ, so this is not a guarantee in game.
    const float ba=hud_theme::alpha(background.a), ta=hud_theme::alpha(text.a);
    Rgba bg{background.r*ba,background.g*ba,background.b*ba,1.f};
    Rgba fg{text.r*ta+bg.r*(1.f-ta),text.g*ta+bg.g*(1.f-ta),text.b*ta+bg.b*(1.f-ta),1.f};
    const float a=luminance(bg), b=luminance(fg);
    return (std::max(a,b)+.05f)/(std::min(a,b)+.05f)>=contrastFloor();
}

// Five editor columns and the neighbouring preview (same order as the mockup).
struct LayoutSpec {
    int columns;
    float toolbarHeight;
    float columnWidth;
    float gap;
    float previewWidth;
    bool stacked;   // narrow viewport fallback
};
inline LayoutSpec layout(float viewportWidth) {
    LayoutSpec s{5, hud_theme::editorMetrics.toolbarHeight,
        hud_theme::panelWidthFor(viewportWidth),
        hud_theme::editorMetrics.panelGap,
        hud_theme::editorMetrics.previewWidth,
        !hud_theme::columnsFit(viewportWidth)};
    return s;
}

// Palette preview card used by the Appearance column.
struct PaletteCardSpec { Rgba bg, border, accent, dot; float rounding, height; };
inline PaletteCardSpec paletteCard(int index) {
    const auto& p = hud_theme::palette(index);
    return {hud_theme::withAlpha(p.bg,.9f), hud_theme::withAlpha(p.border,.55f),
        p.accent, p.accent, 7.f, 24.f};
}

// Generic row metrics shared by toggles, sliders and palette rows.
struct RowSpec { float height, gap; float labelWidth; bool wrapped; };
inline RowSpec row() { return {22.f, 4.f, 104.f, true}; }

} // namespace hud_visual
