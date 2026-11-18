#pragma once
#include <windows.h>

enum class UiIcon { Folder, Previous, Next, Minus, Plus, Fit, Outline, Search, Printer, Close, Pin, Moon, Sun };

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
inline const UiPalette*& CurrentPalette() {
    static const UiPalette* palette = &LightPalette();
    return palette;
}
inline const UiPalette& UiTheme() { return *CurrentPalette(); }
inline bool UiDark() { return CurrentPalette() == &DarkPalette(); }
inline void SetUiTheme(bool dark) { CurrentPalette() = dark ? &DarkPalette() : &LightPalette(); }

// Shared background brush for dialogs and static controls, replaced only when
// the theme changes. The two possible brushes are intentionally kept alive.
inline HBRUSH UiBackgroundBrush() {
    static HBRUSH light = CreateSolidBrush(LightPalette().bar);
    static HBRUSH dark = CreateSolidBrush(DarkPalette().bar);
    return UiDark() ? dark : light;
}

inline void UiRoundRect(HDC dc, RECT bounds, COLORREF fill, COLORREF border, int radius = 10) {
    int saved = SaveDC(dc);
    HBRUSH brush = CreateSolidBrush(fill); HPEN pen = CreatePen(PS_SOLID, 1, border);
    SelectObject(dc, brush); SelectObject(dc, pen);
    RoundRect(dc, bounds.left, bounds.top, bounds.right, bounds.bottom, radius, radius);
    RestoreDC(dc, saved); DeleteObject(brush); DeleteObject(pen);
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

// Small native vector glyphs: no fonts, image dependencies or engine work.
inline void DrawUiIcon(HDC dc, RECT bounds, UiIcon icon, bool active, bool hovered, bool pressed, bool focused) {
    const auto& palette = UiTheme();
    int saved = SaveDC(dc);
    IntersectClipRect(dc, bounds.left, bounds.top, bounds.right, bounds.bottom);
    SetDCBrushColor(dc, palette.bar);
    FillRect(dc, &bounds, reinterpret_cast<HBRUSH>(GetStockObject(DC_BRUSH)));
    COLORREF background = pressed ? palette.pressed : active ? palette.active : hovered ? palette.hover : palette.bar;
    HBRUSH brush = CreateSolidBrush(background);
    SelectObject(dc, brush); SelectObject(dc, GetStockObject(NULL_PEN));
    RoundRect(dc, bounds.left + 2, bounds.top + 2, bounds.right - 2, bounds.bottom - 2, 8, 8);
    HPEN pen = CreatePen(PS_SOLID, 2, active ? palette.accent : palette.ink);
    SelectObject(dc, pen); SelectObject(dc, GetStockObject(NULL_BRUSH));
    int x = (bounds.left + bounds.right - 20) / 2, y = (bounds.top + bounds.bottom - 20) / 2;
    auto line = [&](int a, int b, int c, int d) { MoveToEx(dc, x + a, y + b, nullptr); LineTo(dc, x + c, y + d); };
    auto rect = [&](int a, int b, int c, int d) { Rectangle(dc, x + a, y + b, x + c, y + d); };
    switch (icon) {
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
    case UiIcon::Search: Ellipse(dc, x + 3, y + 2, x + 14, y + 13); line(12, 12, 18, 18); break;
    case UiIcon::Printer:
        rect(5, 2, 15, 7); line(3, 14, 2, 14); line(2, 14, 2, 7); line(2, 7, 18, 7);
        line(18, 7, 18, 14); line(18, 14, 16, 14); rect(5, 11, 15, 18); break;
    case UiIcon::Close: line(5, 5, 15, 15); line(15, 5, 5, 15); break;
    case UiIcon::Pin: rect(6, 3, 14, 10); line(4, 11, 16, 11); line(10, 11, 10, 18); break;
    case UiIcon::Moon: {
        // Filled crescent: an ink circle with a background-colored bite taken out.
        HBRUSH inkBrush = CreateSolidBrush(active ? palette.accent : palette.ink);
        SelectObject(dc, inkBrush); SelectObject(dc, GetStockObject(NULL_PEN));
        Ellipse(dc, x + 2, y + 2, x + 18, y + 18);
        HBRUSH bite = CreateSolidBrush(background);
        SelectObject(dc, bite); Ellipse(dc, x + 9, y, x + 24, y + 16);
        DeleteObject(bite); DeleteObject(inkBrush); break;
    }
    case UiIcon::Sun:
        Ellipse(dc, x + 5, y + 5, x + 15, y + 15);
        line(10, 0, 10, 3); line(10, 17, 10, 20); line(0, 10, 3, 10); line(17, 10, 20, 10);
        line(3, 3, 6, 6); line(14, 14, 17, 17); line(17, 3, 14, 6); line(6, 14, 3, 17); break;
    }
    if (focused) { RECT focus{bounds.left + 4, bounds.top + 4, bounds.right - 4, bounds.bottom - 4}; DrawFocusRect(dc, &focus); }
    RestoreDC(dc, saved); DeleteObject(pen); DeleteObject(brush);
}
