// Pure window-layout math for the windowed in-game menu. No engine includes:
// the adapter (menu_rml.cpp) and the Linux pixel harness both call this, so
// the tiling that ships is exactly the tiling that was verified.
//
// Contract: computeMenuWindows() packs the windows left-to-right into rows
// (a new row starts whenever the next window would not fit the viewport
// width), rebalances a lonely last row, centers every row and the block, and
// scales row heights down proportionally (window bodies scroll) when the
// natural heights do not fit. Windows in their default positions never
// overlap each other and never leave the viewport.
#pragma once

namespace menu_layout {

struct WindowRect {
    float x;
    float y;
    float w;
    float h;  // unfolded height actually shown (<= natural height)
};

constexpr float kMargin = 28.0f;
constexpr float kGap = 14.0f;
constexpr float kMinRowHeight = 200.0f;
constexpr int kMaxRows = 8;
constexpr int kMaxWindows = 16;

// row_of[i] receives the row index of window i; row_count receives the number
// of rows. Greedy first-fit in window order: a window only joins a row when
// the row's width with it still fits the viewport, so rows never overflow
// horizontally (except a single window wider than the viewport itself).
inline void packRows(const float* natural_w, int count, float avail_w,
                     int* row_of, int& row_count) {
    row_count = 1;
    float row_w = 0.0f;
    for (int i = 0; i < count; ++i) {
        if (row_w == 0.0f) {
            row_w = natural_w[i];
        } else if (row_w + kGap + natural_w[i] <= avail_w) {
            row_w += kGap + natural_w[i];
        } else {
            row_count++;
            row_w = natural_w[i];
        }
        row_of[i] = row_count - 1;
    }
}

// Move trailing windows down while the last row is much emptier than the one
// before it, so a 6+1 split becomes 5+2 and 4+3 instead of an orphan.
inline void rebalanceLastRow(const float* natural_w, int count, float avail_w,
                             int* row_of, int& row_count) {
    while (row_count >= 2) {
        int last = 0, prev = 0, last_start = -1, prev_last = -1;
        float last_w = 0.0f, prev_w = 0.0f;
        for (int i = 0; i < count; ++i) {
            if (row_of[i] == row_count - 1) {
                last++;
                if (last_start < 0) last_start = i;
                last_w = (last == 1) ? natural_w[i] : last_w + kGap + natural_w[i];
            } else if (row_of[i] == row_count - 2) {
                prev++;
                prev_last = i;
                prev_w = (prev == 1) ? natural_w[i] : prev_w + kGap + natural_w[i];
            }
        }
        if (last_start < 0 || prev_last < 0 || last >= prev - 1 || prev <= 1) break;
        if (last_w + kGap + natural_w[prev_last] > avail_w) break;
        row_of[prev_last] = row_count - 1;
    }
}

inline void computeMenuWindows(const float* natural_w, const float* natural_h, int count,
                               float view_w, float view_h, WindowRect* out) {
    const float avail_w = view_w - 2.0f * kMargin;
    const float avail_h = view_h - 2.0f * kMargin;

    int row_of[kMaxWindows] = {};
    int rows = 1;
    packRows(natural_w, count, avail_w, row_of, rows);
    if (rows > kMaxRows) rows = kMaxRows;
    rebalanceLastRow(natural_w, count, avail_w, row_of, rows);

    float row_natural[kMaxRows] = {};
    for (int i = 0; i < count; ++i) {
        const int row = row_of[i];
        if (row < kMaxRows && row_natural[row] < natural_h[i]) row_natural[row] = natural_h[i];
    }

    // Hand out the vertical space: every row keeps at least kMinRowHeight
    // while space lasts; above that, rows grow proportionally to their
    // natural height. If even the minimums do not fit (tiny viewport), the
    // rows scale purely proportionally so the block always fits exactly.
    const float room = avail_h - (rows - 1) * kGap;
    float row_height[kMaxRows] = {};
    const float min_total = rows * kMinRowHeight;
    if (room >= min_total) {
        float excess_total = 0.0f;
        float excess[kMaxRows] = {};
        for (int r = 0; r < rows; ++r) {
            excess[r] = (row_natural[r] > kMinRowHeight) ? row_natural[r] - kMinRowHeight : 0.0f;
            excess_total += excess[r];
        }
        const float room_excess = room - min_total;
        const float scale = (excess_total > 0.0f && room_excess < excess_total)
                                ? room_excess / excess_total
                                : 1.0f;
        for (int r = 0; r < rows; ++r) row_height[r] = kMinRowHeight + excess[r] * scale;
    } else if (room > 0.0f) {
        float natural_total = 0.0f;
        for (int r = 0; r < rows; ++r) natural_total += row_natural[r];
        const float scale = natural_total > 0.0f ? room / natural_total : 1.0f;
        for (int r = 0; r < rows; ++r) row_height[r] = row_natural[r] * scale;
    } else {
        for (int r = 0; r < rows; ++r) row_height[r] = kMinRowHeight;
    }

    float block_h = (rows - 1) * kGap;
    for (int r = 0; r < rows; ++r) block_h += row_height[r];
    float y = kMargin + (avail_h - block_h) * 0.5f;
    if (y < kMargin) y = kMargin;

    for (int r = 0; r < rows; ++r) {
        int in_row = 0;
        float row_w = 0.0f;
        for (int i = 0; i < count; ++i) {
            if (row_of[i] != r) continue;
            row_w = (in_row == 0) ? natural_w[i] : row_w + kGap + natural_w[i];
            in_row++;
        }
        float x = kMargin + (avail_w - row_w) * 0.5f;
        for (int i = 0; i < count; ++i) {
            if (row_of[i] != r) continue;
            out[i].x = x;
            out[i].y = y;
            out[i].w = natural_w[i];
            out[i].h = (natural_h[i] < row_height[r]) ? natural_h[i] : row_height[r];
            x += natural_w[i] + kGap;
        }
        y += row_height[r] + kGap;
    }
}

// Clamp a dragged window so its title bar always stays reachable.
inline void clampDraggedWindow(WindowRect& rect, float view_w, float view_h) {
    const float keep_w = 90.0f;
    if (rect.x < keep_w - rect.w) rect.x = keep_w - rect.w;
    if (rect.x > view_w - keep_w) rect.x = view_w - keep_w;
    if (rect.y < 0.0f) rect.y = 0.0f;
    if (rect.y > view_h - 46.0f) rect.y = view_h - 46.0f;
}

// True when no pair of the first count rects overlaps. Windows are compared
// on their shown (unfolded) height; the caller passes folded windows with
// their title-bar-only height (46px).
inline bool rectsOverlap(const WindowRect* rects, int count) {
    for (int i = 0; i < count; ++i)
        for (int j = i + 1; j < count; ++j) {
            const bool apart_x = rects[i].x + rects[i].w <= rects[j].x + 0.01f ||
                                 rects[j].x + rects[j].w <= rects[i].x + 0.01f;
            const bool apart_y = rects[i].y + rects[i].h <= rects[j].y + 0.01f ||
                                 rects[j].y + rects[j].h <= rects[i].y + 0.01f;
            if (!apart_x && !apart_y) return true;
        }
    return false;
}

}  // namespace menu_layout
