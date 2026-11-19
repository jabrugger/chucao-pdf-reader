#pragma once
#include <windows.h>
#include <gdiplus.h>
#include <cmath>
#include <algorithm>
#include <utility>
#include <vector>

enum class UiIcon { Folder, Previous, Next, Minus, Plus, Fit, Outline, Search, Printer, Close, Pin, Moon, Sun, Info, Gear };

// Central palette for every custom-drawn surface. PDF sheets stay white in both
// themes: the document is the content, not part of the chrome.
struct UiPalette {
    COLORREF ink, muted, line, canvas, bar, field, accent;
    COLORREF sheet, shadow, hover, active, pressed;
    COLORREF welcomeTitle, welcomeText;
};
inline const UiPalette& LightPalette() {
    static const UiPalette palette{
        RGB(39, 49, 63), RGB(111, 121, 134), RGB(228, 232, 236), RGB(242, 244, 247),
        RGB(255, 255, 255), RGB(247, 248, 250), RGB(176, 76, 26),
        RGB(255, 255, 255), RGB(223, 228, 234), RGB(241, 243, 246),
        RGB(255, 239, 224), RGB(247, 224, 204), RGB(22, 34, 49), RGB(86, 97, 112)};
    return palette;
}
inline const UiPalette& DarkPalette() {
    static const UiPalette palette{
        RGB(226, 232, 240), RGB(148, 163, 184), RGB(52, 63, 79), RGB(24, 29, 37),
        RGB(31, 38, 48), RGB(41, 50, 63), RGB(230, 126, 68),
        RGB(255, 255, 255), RGB(12, 16, 22), RGB(55, 66, 82),
        RGB(92, 60, 36), RGB(110, 72, 42), RGB(236, 241, 247), RGB(168, 181, 196)};
    return palette;
}
// Windows high contrast replaces the custom palette with system colors. The
// hover/active fills stay flat (COLOR_WINDOW) so the accent stroke (the highlight
// color) remains the only indicator and never loses contrast.
inline const UiPalette& HighContrastPalette() {
    static UiPalette palette = LightPalette();
    palette = UiPalette{
        GetSysColor(COLOR_WINDOWTEXT), GetSysColor(COLOR_WINDOWTEXT), GetSysColor(COLOR_3DSHADOW),
        GetSysColor(COLOR_WINDOW), GetSysColor(COLOR_WINDOW), GetSysColor(COLOR_WINDOW),
        GetSysColor(COLOR_HIGHLIGHT),
        GetSysColor(COLOR_WINDOW), GetSysColor(COLOR_WINDOW), GetSysColor(COLOR_WINDOW),
        GetSysColor(COLOR_WINDOW), GetSysColor(COLOR_WINDOW),
        GetSysColor(COLOR_WINDOWTEXT), GetSysColor(COLOR_WINDOWTEXT)};
    return palette;
}
inline bool HighContrastActive() {
    HIGHCONTRASTW info{sizeof(info)};
    return SystemParametersInfoW(SPI_GETHIGHCONTRAST, sizeof(info), &info, 0) &&
        (info.dwFlags & HCF_HIGHCONTRASTON) != 0;
}
inline const UiPalette*& CurrentPalette() {
    static const UiPalette* palette = &LightPalette();
    return palette;
}
inline const UiPalette& UiTheme() { return *CurrentPalette(); }
inline bool UiDark() { return CurrentPalette() == &DarkPalette(); }
inline void SetUiTheme(bool dark) { CurrentPalette() = dark ? &DarkPalette() : &LightPalette(); }

// Shared background brush for dialogs and static controls. Brushes are cached by
// color and kept alive, so light, dark and any high-contrast scheme can change at
// runtime without leaking one brush per paint.
inline HBRUSH UiBackgroundBrush() {
    static std::vector<std::pair<COLORREF, HBRUSH>> brushes;
    COLORREF color = UiTheme().bar;
    for (const auto& entry : brushes) if (entry.first == color) return entry.second;
    HBRUSH brush = CreateSolidBrush(color);
    brushes.emplace_back(color, brush);
    return brush;
}

namespace UiDraw {
inline Gdiplus::Color Color(COLORREF color) {
    return Gdiplus::Color(255, GetRValue(color), GetGValue(color), GetBValue(color));
}
inline void RoundedRectPath(Gdiplus::GraphicsPath& path, float x, float y, float width, float height, float radius) {
    float r = std::max(0.0f, std::min(radius, std::min(width, height) / 2));
    float d = r * 2;
    if (d <= 0) { path.AddRectangle(Gdiplus::RectF(x, y, width, height)); return; }
    path.AddArc(x, y, d, d, 180.0f, 90.0f);
    path.AddArc(x + width - d, y, d, d, 270.0f, 90.0f);
    path.AddArc(x + width - d, y + height - d, d, d, 0.0f, 90.0f);
    path.AddArc(x, y + height - d, d, d, 90.0f, 90.0f);
    path.CloseFigure();
}
} // namespace UiDraw

// Rounded fields and cards are drawn with GDI+ so their corners stay smooth at
// every scale. The pen width follows the radius, which callers already scale
// with the monitor DPI.
inline void UiRoundRect(HDC dc, RECT bounds, COLORREF fill, COLORREF border, int radius = 10) {
    if (bounds.right <= bounds.left || bounds.bottom <= bounds.top) return;
    Gdiplus::Graphics graphics(dc);
    if (graphics.GetLastStatus() != Gdiplus::Ok) return;
    graphics.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
    Gdiplus::GraphicsPath path;
    UiDraw::RoundedRectPath(path, static_cast<float>(bounds.left), static_cast<float>(bounds.top),
        static_cast<float>(bounds.right - bounds.left), static_cast<float>(bounds.bottom - bounds.top),
        static_cast<float>(radius));
    Gdiplus::SolidBrush brush(UiDraw::Color(fill));
    graphics.FillPath(&brush, &path);
    if (border != fill) {
        Gdiplus::Pen pen(UiDraw::Color(border), std::max(1.0f, std::round(radius / 10.0f)));
        graphics.DrawPath(&pen, &path);
    }
}

inline void PaintUiChildren(HDC dc, HWND parent) {
    for (HWND child = GetWindow(parent, GW_CHILD); child; child = GetWindow(child, GW_HWNDNEXT)) {
        if (!(GetWindowLongPtrW(child, GWL_STYLE) & WS_VISIBLE)) continue;
        RECT bounds{}; GetWindowRect(child, &bounds);
        MapWindowPoints(nullptr, parent, reinterpret_cast<POINT*>(&bounds), 2);
        int saved = SaveDC(dc); OffsetViewportOrgEx(dc, bounds.left, bounds.top, nullptr);
        IntersectClipRect(dc, 0, 0, bounds.right - bounds.left, bounds.bottom - bounds.top);
        SendMessageW(child, WM_PRINT, reinterpret_cast<WPARAM>(dc), PRF_CLIENT | PRF_NONCLIENT | PRF_ERASEBKGND);
        PaintUiChildren(dc, child); RestoreDC(dc, saved);
    }
}

// Small native vector glyphs drawn with GDI+ antialiasing: no fonts or image
// dependencies, and smooth diagonals at every DPI.
inline void DrawUiIcon(HDC dc, RECT bounds, UiIcon icon, bool active, bool hovered, bool pressed, bool focused) {
    const auto& palette = UiTheme();
    Gdiplus::Graphics graphics(dc);
    if (graphics.GetLastStatus() != Gdiplus::Ok) return;
    graphics.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
    float left = static_cast<float>(bounds.left), top = static_cast<float>(bounds.top);
    float width = static_cast<float>(bounds.right - bounds.left), height = static_cast<float>(bounds.bottom - bounds.top);
    if (width <= 0 || height <= 0) return;
    float scale = width / 36.0f;
    Gdiplus::SolidBrush barBrush(UiDraw::Color(palette.bar));
    graphics.FillRectangle(&barBrush, left, top, width, height);
    COLORREF background = pressed ? palette.pressed : active ? palette.active : hovered ? palette.hover : palette.bar;
    float inset = std::max(1.0f, std::round(2 * scale));
    Gdiplus::GraphicsPath backgroundPath;
    UiDraw::RoundedRectPath(backgroundPath, left + inset, top + inset, width - 2 * inset, height - 2 * inset,
        std::max(2.0f, std::round(8 * scale)));
    Gdiplus::SolidBrush backgroundBrush(UiDraw::Color(background));
    graphics.FillPath(&backgroundBrush, &backgroundPath);
    Gdiplus::Pen pen(UiDraw::Color(active ? palette.accent : palette.ink), std::max(1.4f, 2.0f * scale));
    pen.SetLineCap(Gdiplus::LineCapRound, Gdiplus::LineCapRound, Gdiplus::DashCapRound);
    pen.SetLineJoin(Gdiplus::LineJoinRound);
    float glyph = std::max(12.0f, std::round(20 * scale));
    float x = left + (width - glyph) / 2, y = top + (height - glyph) / 2;
    auto sx = [&](float value) { return x + value * scale; };
    auto sy = [&](float value) { return y + value * scale; };
    auto line = [&](float a, float b, float c, float d) { graphics.DrawLine(&pen, sx(a), sy(b), sx(c), sy(d)); };
    auto rect = [&](float a, float b, float c, float d) {
        graphics.DrawRectangle(&pen, sx(a), sy(b), (c - a) * scale, (d - b) * scale);
    };
    auto ellipse = [&](float a, float b, float c, float d) {
        graphics.DrawEllipse(&pen, sx(a), sy(b), (c - a) * scale, (d - b) * scale);
    };
    switch (icon) {
    case UiIcon::Gear:
        ellipse(4, 4, 16, 16); ellipse(8, 8, 12, 12);
        for (int i = 0; i < 8; ++i) {
            float angle = static_cast<float>(i * 3.141592653589793 / 4);
            line(10 + 6 * std::cos(angle), 10 + 6 * std::sin(angle), 10 + 9 * std::cos(angle), 10 + 9 * std::sin(angle));
        }
        break;
    case UiIcon::Folder:
        line(2, 6, 2, 3); line(2, 3, 8, 3); line(8, 3, 10, 6); line(10, 6, 18, 6);
        line(18, 6, 16, 17); line(16, 17, 2, 17); line(2, 17, 2, 6); line(2, 8, 17, 8); break;
    case UiIcon::Previous: line(12, 4, 6, 10); line(6, 10, 12, 16); break;
    case UiIcon::Next: line(7, 4, 13, 10); line(13, 10, 7, 16); break;
    case UiIcon::Plus: line(10, 4, 10, 16); [[fallthrough]];
    case UiIcon::Minus: line(4, 10, 16, 10); break;
    case UiIcon::Fit:
        rect(5, 3, 15, 17); line(1, 6, 1, 14); line(19, 6, 19, 14);
        line(1, 10, 5, 10); line(15, 10, 19, 10); break;
    case UiIcon::Outline: rect(2, 3, 18, 17); line(7, 3, 7, 17); line(10, 7, 15, 7); line(10, 11, 15, 11); break;
    case UiIcon::Search: ellipse(3, 2, 14, 13); line(12, 12, 18, 18); break;
    case UiIcon::Printer:
        rect(5, 2, 15, 7); line(3, 14, 2, 14); line(2, 14, 2, 7); line(2, 7, 18, 7);
        line(18, 7, 18, 14); line(18, 14, 16, 14); rect(5, 11, 15, 18); break;
    case UiIcon::Close: line(5, 5, 15, 15); line(15, 5, 5, 15); break;
    case UiIcon::Pin: rect(6, 3, 14, 10); line(4, 11, 16, 11); line(10, 11, 10, 18); break;
    case UiIcon::Moon: {
        // Crescent: an ink circle with a background bite taken out, opening right.
        Gdiplus::SolidBrush inkBrush(UiDraw::Color(active ? palette.accent : palette.ink));
        graphics.FillEllipse(&inkBrush, sx(2), sy(2), 16 * scale, 16 * scale);
        Gdiplus::SolidBrush bite(UiDraw::Color(background));
        graphics.FillEllipse(&bite, sx(9), sy(0), 15 * scale, 16 * scale);
        break;
    }
    case UiIcon::Sun:
        ellipse(7, 7, 13, 13);
        line(10, 2, 10, 4.5f); line(10, 15.5f, 10, 18); line(2, 10, 4.5f, 10); line(15.5f, 10, 18, 10);
        line(6.5f, 6.5f, 4.5f, 4.5f); line(13.5f, 6.5f, 15.5f, 4.5f);
        line(6.5f, 13.5f, 4.5f, 15.5f); line(13.5f, 13.5f, 15.5f, 15.5f); break;
    case UiIcon::Info: {
        ellipse(4, 4, 16, 16);
        Gdiplus::SolidBrush dot(UiDraw::Color(active ? palette.accent : palette.ink));
        graphics.FillEllipse(&dot, sx(9.0f), sy(6.0f), 2 * scale, 2 * scale);
        line(10, 9, 10, 14);
        break;
    }
    }
    if (focused) {
        int margin = std::max(2, static_cast<int>(std::lround(4 * scale)));
        RECT focus{bounds.left + margin, bounds.top + margin, bounds.right - margin, bounds.bottom - margin};
        DrawFocusRect(dc, &focus);
    }
}

// Folder and dropdown chevron for the labelled Open button.
inline void DrawOpenGlyph(HDC dc, RECT box, COLORREF color, float scale) {
    Gdiplus::Graphics graphics(dc);
    if (graphics.GetLastStatus() != Gdiplus::Ok) return;
    graphics.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
    Gdiplus::Pen pen(UiDraw::Color(color), std::max(1.4f, 2.0f * scale));
    pen.SetLineCap(Gdiplus::LineCapRound, Gdiplus::LineCapRound, Gdiplus::DashCapRound);
    pen.SetLineJoin(Gdiplus::LineJoinRound);
    float x = static_cast<float>(box.left) + 8 * scale, y = static_cast<float>(box.top) + 8 * scale;
    auto line = [&](float a, float b, float c, float d) {
        graphics.DrawLine(&pen, x + a * scale, y + b * scale, x + c * scale, y + d * scale);
    };
    line(0, 3, 6, 3); line(6, 3, 9, 6); line(9, 6, 17, 6); line(17, 6, 15, 17);
    line(15, 17, 0, 17); line(0, 17, 0, 3); line(0, 8, 16, 8);
    float chevronX = static_cast<float>(box.right) - 18 * scale;
    graphics.DrawLine(&pen, chevronX, y + 8 * scale, chevronX + 4 * scale, y + 12 * scale);
    graphics.DrawLine(&pen, chevronX + 4 * scale, y + 12 * scale, chevronX + 8 * scale, y + 8 * scale);
}
