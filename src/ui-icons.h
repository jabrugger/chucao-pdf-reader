#pragma once
#include <windows.h>

enum class UiIcon { Folder, Previous, Next, Minus, Plus, Fit, Outline, Search, Printer, Close, Pin };

namespace UiColor {
constexpr COLORREF Ink = RGB(39, 49, 63);
constexpr COLORREF Muted = RGB(111, 121, 134);
constexpr COLORREF Line = RGB(228, 232, 236);
constexpr COLORREF Canvas = RGB(242, 244, 247);
constexpr COLORREF Field = RGB(247, 248, 250);
constexpr COLORREF Accent = RGB(176, 76, 26);
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
    int saved = SaveDC(dc);
    FillRect(dc, &bounds, reinterpret_cast<HBRUSH>(GetStockObject(WHITE_BRUSH)));
    COLORREF background = pressed ? RGB(247, 224, 204) : active ? RGB(255, 239, 224) : hovered ? RGB(241, 243, 246) : RGB(255, 255, 255);
    HBRUSH brush = CreateSolidBrush(background);
    SelectObject(dc, brush); SelectObject(dc, GetStockObject(NULL_PEN));
    RoundRect(dc, bounds.left + 2, bounds.top + 2, bounds.right - 2, bounds.bottom - 2, 8, 8);
    HPEN pen = CreatePen(PS_SOLID, 2, active ? UiColor::Accent : UiColor::Ink);
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
    }
    if (focused) { RECT focus{bounds.left + 4, bounds.top + 4, bounds.right - 4, bounds.bottom - 4}; DrawFocusRect(dc, &focus); }
    RestoreDC(dc, saved); DeleteObject(pen); DeleteObject(brush);
}
