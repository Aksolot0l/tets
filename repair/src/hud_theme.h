#pragma once
// GAYClient HUD visual language (ports the redesign mockup).
//
// VISUAL LAYER ONLY: this header contains colours, spacing, radii and geometry.
// It owns no settings, writes no game state and has no ImGui dependency, so the
// developer can wire it into the adapter without any behavioural change.
//
// The closed core controls the real HUD values and may paint its own card
// backgrounds; tokens here are used by the scoped card theme and by the
// adapter-owned editor windows.
#include <algorithm>
#include <cmath>
#include <cstring>

namespace hud_theme {

// --- Scoped legacy HUD cards (contract preserved) ---------------------------
inline bool card(const char* name) {
    if (!name) return false;
    constexpr const char prefix[] = "##notification_";
    if (std::strncmp(name, prefix, sizeof(prefix) - 1) == 0) {
        const char* p = name + sizeof(prefix) - 1;
        if (!*p) return false;
        for (; *p; ++p) if (*p < '0' || *p > '9') return false;
        return true;
    }
    constexpr const char* names[] = {"##hud_fps","##hud_cps","##hud_clock","##hud_cpu","##hud_ram",
        "##hud_resolution","##hud_watermark","##hud_info","##hud_active","##hud_hotkeys",
        "##hud_coordinates","##hud_keystrokes"};
    for (const auto* n : names) if (std::strcmp(name, n) == 0) return true;
    return false;
}

// --- Numeric guards (no behaviour, only sane visuals) ----------------------
inline float alpha(float v) { return std::isfinite(v) ? std::clamp(v, 0.f, 1.f) : .72f; }
inline float clamp01(float v) { return std::isfinite(v) ? std::clamp(v, 0.f, 1.f) : 0.f; }
inline float brightness(float v) { return std::isfinite(v) ? std::clamp(v, .5f, 1.6f) : 1.f; }
inline float rounding(float v) { return std::isfinite(v) ? std::clamp(v, 0.f, 20.f) : 8.f; }
inline float scale(float v) { return std::isfinite(v) ? std::clamp(v, .7f, 1.6f) : 1.f; }
inline float opacity(float v) { return std::isfinite(v) ? std::clamp(v, 0.f, 1.f) : .78f; }

struct Rgba { float r, g, b, a; };

inline Rgba rgba(int r, int g, int b, float a = 1.f) {
    return {r / 255.f, g / 255.f, b / 255.f, alpha(a)};
}
inline Rgba mix(const Rgba& a, const Rgba& b, float t) {
    const float k = clamp01(t);
    return {a.r + (b.r - a.r) * k, a.g + (b.g - a.g) * k,
            a.b + (b.b - a.b) * k, a.a + (b.a - a.a) * k};
}
inline Rgba withAlpha(Rgba c, float a) { c.a = alpha(a); return c; }

// --- Palettes from the mockup (Aogiri Green / Emerald / Graphite) ----------
struct Palette {
    const char* name;
    Rgba bg;
    Rgba border;
    Rgba accent;
    Rgba text;
};
inline constexpr int kPaletteCount = 3;
inline constexpr Palette palettes[kPaletteCount] = {
    {"Aogiri Green", {8.f/255,19.f/255,13.f/255,1}, {47.f/255,122.f/255,74.f/255,1},
        {77.f/255,1,136.f/255,1}, {234.f/255,248.f/255,238.f/255,1}},
    {"Emerald", {7.f/255,19.f/255,15.f/255,1}, {43.f/255,110.f/255,90.f/255,1},
        {46.f/255,230.f/255,160.f/255,1}, {226.f/255,246.f/255,232.f/255,1}},
    {"Graphite", {13.f/255,17.f/255,16.f/255,1}, {58.f/255,84.f/255,68.f/255,1},
        {157.f/255,1,180.f/255,1}, {236.f/255,244.f/255,238.f/255,1}},
};
inline const Palette& palette(int i) { return palettes[i >= 0 && i < kPaletteCount ? i : 0]; }

// Backward-compatible helpers used by the scoped card hook.
inline Rgba background(float opacityValue, int theme = 0) {
    auto c = palette(theme).bg; c.a = alpha(opacityValue); return c;
}
inline Rgba border(float opacityValue, int theme = 0) {
    auto c = palette(theme).border; c.a = alpha(opacityValue) * .75f; return c;
}
inline Rgba text(int theme = 0) { return palette(theme).text; }
inline Rgba accent(int theme = 0) { return palette(theme).accent; }

// --- Full visual token set for adapter-owned windows ----------------------
struct Tokens {
    Rgba window, panel, panelSoft, inset;
    Rgba border, borderSoft, highlight;
    Rgba accent, accentSoft, accentLine;
    Rgba text, textMuted, textDim;
    Rgba success, warning, danger;
    float rounding, roundingSmall, roundingPill, borderSize, opacity;
    float padWindow, padSection, gapRow, indent;
};

inline Tokens tokens(int theme = 0, float opacityValue = .94f,
                     float roundingValue = 10.f, float brightnessValue = 1.f) {
    const auto& p = palette(theme);
    const float o = opacity(opacityValue);
    const auto b = brightness(brightnessValue);
    auto boost=[&](Rgba c) { return Rgba{clamp01(c.r*b),clamp01(c.g*b),clamp01(c.b*b),c.a}; };
    Tokens t{};
    t.window     = boost(withAlpha(p.bg, o*.96f));
    t.panel      = boost(withAlpha(mix(p.bg,p.accent,.04f), std::min(1.f,o+.02f)));
    t.panelSoft  = boost(withAlpha(mix(p.bg,p.accent,.075f), o));
    t.inset      = boost(withAlpha(mix(p.bg,Rgba{0,0,0,1},.35f), std::min(1.f,o+.02f)));
    t.border     = withAlpha(p.border,.62f);
    t.borderSoft = withAlpha(mix(p.border,p.bg,.58f),.52f);
    t.highlight  = withAlpha(Rgba{1,1,1,1},.05f);
    t.accent     = p.accent;
    t.accentSoft = withAlpha(p.accent,.12f);
    t.accentLine = withAlpha(p.accent,.72f);
    t.text       = boost(p.text);
    t.textMuted  = withAlpha(mix(p.text,p.bg,.40f),.94f);
    t.textDim    = withAlpha(mix(p.text,p.bg,.62f),.85f);
    t.success    = rgba(110,245,140);
    t.warning    = rgba(242,199,106);
    t.danger     = rgba(248,113,113);
    t.rounding      = rounding(roundingValue);
    t.roundingSmall = std::min(t.rounding,6.f);
    t.roundingPill  = 999.f;
    t.borderSize    = 1.f;
    t.opacity       = o;
    // Spacing scale from the mockup (8px grid, 10/12/16px sections).
    t.padWindow  = 12.f;
    t.padSection = 10.f;
    t.gapRow     = 6.f;
    t.indent     = 8.f;
    return t;
}

// --- Editor geometry (visual layout only) --------------------------------
struct EditorMetrics {
    float panelWidth, panelGap, panelTop, panelLeft, toolbarHeight, previewWidth, previewGap;
};
inline constexpr EditorMetrics editorMetrics{190.f, 8.f, 88.f, 16.f, 32.f, 208.f, 10.f};

// Wide displays use five columns and a separate preview. On smaller windows
// the toolbar selects one panel; fixed-width coordinates must not overlap it.
inline float panelWidthFor(float viewportWidth) {
    const float reserved = editorMetrics.panelLeft*2.f + editorMetrics.previewWidth + editorMetrics.previewGap*2.f;
    const float five = (viewportWidth - reserved - editorMetrics.panelGap*4.f) / 5.f;
    return std::max(150.f, std::min(editorMetrics.panelWidth, five));
}
inline bool columnsFit(float viewportWidth) {
    const float required = editorMetrics.panelLeft*2.f + editorMetrics.previewWidth +
        editorMetrics.previewGap*2.f + editorMetrics.panelGap*4.f + 5.f*150.f;
    return std::isfinite(viewportWidth) && viewportWidth >= required;
}
inline float panelX(int index, float viewportWidth = 1920.f) {
    const int i = index < 0 ? 0 : (index > 4 ? 4 : index);
    return editorMetrics.panelLeft + float(i) * (panelWidthFor(viewportWidth) + editorMetrics.panelGap);
}
inline float previewX(float viewportWidth) {
    return std::max(editorMetrics.panelLeft, viewportWidth - editorMetrics.previewWidth - editorMetrics.panelLeft);
}
inline float compactPanelWidth(float viewportWidth) {
    const float available = viewportWidth >= 600.f
        ? previewX(viewportWidth)-editorMetrics.panelLeft-editorMetrics.previewGap
        : viewportWidth-editorMetrics.panelLeft*2.f;
    return std::max(100.f, std::min(260.f,available));
}
inline float panelTop(float viewportWidth) {
    return columnsFit(viewportWidth) ? editorMetrics.panelTop : (viewportWidth < 480.f ? 128.f : 106.f);
}
inline float panelHeight(int index, float viewportWidth, float viewportHeight) {
    constexpr float desired[5]={210.f,420.f,280.f,360.f,290.f};
    const int i=index < 0 ? 0 : (index > 4 ? 4 : index);
    return std::max(48.f,std::min(desired[i],viewportHeight-panelTop(viewportWidth)-16.f));
}

// The editor chrome never fades below a readable level; the adjustable card
// opacity applies to the real HUD cards and the preview, not to the controls.
inline float readableOpacity(float requested) {
    const float v = opacity(requested);
    return v < .18f ? .18f : v;
}
inline float editorOpacity(float) { return .94f; }

// Text hierarchy used by the kit (visual only).
inline float fontSizeTitle()   { return 13.f; }
inline float fontSizeSection() { return 10.f; }
inline float fontSizeBody()    { return 11.f; }
inline float fontSizeHint()    { return 9.f; }
inline float fontSizePill()    { return 8.f; }

} // namespace hud_theme
