#pragma once
// Window procedures of the outline, home list and toolbar buttons.
// Internal fragment of main.cpp: include it only from there, after the Windows/PDFium headers.
namespace {
LRESULT CALLBACK OutlineProc(HWND window, UINT message, WPARAM wp, LPARAM lp) {
    auto app = reinterpret_cast<Reader*>(GetWindowLongPtrW(window, GWLP_USERDATA));
    if (message == WM_NCCREATE) {
        app = static_cast<Reader*>(reinterpret_cast<CREATESTRUCTW*>(lp)->lpCreateParams);
        SetWindowLongPtrW(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(app));
    }
    if (!app) return DefWindowProcW(window, message, wp, lp);
    auto paint = [&](HDC dc) {
        RECT rc{}; GetClientRect(window, &rc);
        SetDCBrushColor(dc, UiTheme().bar); FillRect(dc, &rc, reinterpret_cast<HBRUSH>(GetStockObject(DC_BRUSH)));
        SelectObject(dc, app->strongFont); SetBkMode(dc, TRANSPARENT); SetTextColor(dc, UiTheme().ink);
        RECT title{app->D(20), 0, rc.right - app->D(80), app->D(48)}; DrawTextW(dc, L"Contenido", -1, &title, DT_SINGLELINE | DT_VCENTER);
        RECT rule{rc.right - 1, 0, rc.right, rc.bottom}; SetDCBrushColor(dc, UiTheme().line);
        FillRect(dc, &rule, reinterpret_cast<HBRUSH>(GetStockObject(DC_BRUSH)));
    };
    switch (message) {
    case WM_CREATE:
        app->IconButton(window, OutlinePin, L"Fijar el índice abierto");
        app->IconButton(window, OutlineClose, L"Plegar el índice (Ctrl+B)");
        app->outlineFilter = CreateWindowExW(0, L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
            0, 0, 1, 1, window, reinterpret_cast<HMENU>(static_cast<INT_PTR>(OutlineFilterId)), nullptr, nullptr);
        SendMessageW(app->outlineFilter, EM_SETCUEBANNER, FALSE, reinterpret_cast<LPARAM>(L"Filtrar el índice"));
        SendMessageW(app->outlineFilter, EM_SETLIMITTEXT, 128, 0);
        SetAccessibleName(app->outlineFilter, L"Filtrar el índice");
        app->outlineFilterClear = CreateWindowW(L"BUTTON", L"Limpiar el filtro", WS_CHILD | WS_TABSTOP | BS_OWNERDRAW | BS_FLAT,
            0, 0, app->D(28), app->D(28), window, reinterpret_cast<HMENU>(static_cast<INT_PTR>(OutlineFilterClear)), nullptr, nullptr);
        SetWindowTheme(app->outlineFilterClear, L"", L"");
        SetWindowSubclass(app->outlineFilterClear, IconButtonProc, 1, 0);
        SetAccessibleName(app->outlineFilterClear, L"Limpiar el filtro");
        app->outlineTree = CreateWindowExW(0, WC_TREEVIEWW, L"Índice del documento", WS_CHILD | WS_VISIBLE | WS_TABSTOP |
            TVS_LINESATROOT | TVS_HASBUTTONS | TVS_FULLROWSELECT, 0, 0, 1, 1, window, nullptr, nullptr, nullptr);
        SetWindowSubclass(app->outlineTree, OutlineTreeProc, 1, 0);
        app->outlineLabel = CreateWindowW(L"STATIC", L"", WS_CHILD | WS_VISIBLE, 8, 0, 1, 1, window, nullptr, nullptr, nullptr);
        app->StyleOutline(); return 0;
    case WM_SIZE: {
        RECT rc{}; GetClientRect(window, &rc);
        MoveWindow(GetDlgItem(window, OutlinePin), rc.right - app->D(80), app->D(6), app->D(36), app->D(36), FALSE);
        MoveWindow(GetDlgItem(window, OutlineClose), rc.right - app->D(42), app->D(6), app->D(36), app->D(36), FALSE);
        MoveWindow(app->outlineFilter, app->D(12), app->D(52), std::max<LONG>(1, rc.right - app->D(24) - app->D(32)), app->D(28), FALSE);
        MoveWindow(GetDlgItem(window, OutlineFilterClear), rc.right - app->D(40), app->D(52), app->D(28), app->D(28), FALSE);
        MoveWindow(app->outlineTree, app->D(12), app->D(88), std::max<LONG>(1, rc.right - app->D(24)), std::max<LONG>(1, rc.bottom - app->D(132)), FALSE);
        MoveWindow(app->outlineLabel, app->D(20), std::max<LONG>(0, rc.bottom - app->D(36)), std::max<LONG>(1, rc.right - app->D(32)), app->D(28), FALSE); return 0;
    }
    case WM_COMMAND:
        if (LOWORD(wp) == OutlineFilterId && HIWORD(wp) == EN_CHANGE) { app->ApplyOutlineFilter(); return 0; }
        app->Command(LOWORD(wp)); return 0;
    case WM_DRAWITEM: app->PaintButton(*reinterpret_cast<DRAWITEMSTRUCT*>(lp)); return TRUE;
    case WM_CTLCOLORSTATIC:
        SetTextColor(reinterpret_cast<HDC>(wp), UiTheme().muted); SetBkColor(reinterpret_cast<HDC>(wp), UiTheme().bar);
        return reinterpret_cast<LRESULT>(UiBackgroundBrush());
    case WM_CTLCOLOREDIT:
        if (reinterpret_cast<HWND>(lp) == app->outlineFilter) {
            SetTextColor(reinterpret_cast<HDC>(wp), UiTheme().ink); SetBkColor(reinterpret_cast<HDC>(wp), UiTheme().field);
            SetDCBrushColor(reinterpret_cast<HDC>(wp), UiTheme().field);
            return reinterpret_cast<LRESULT>(GetStockObject(DC_BRUSH));
        }
        break;
    case WM_ERASEBKGND: return 1;
    case WM_PAINT: { PAINTSTRUCT ps{}; HDC dc = BeginPaint(window, &ps); paint(dc); EndPaint(window, &ps); return 0; }
    case WM_PRINTCLIENT: paint(reinterpret_cast<HDC>(wp)); return 0;
    case WM_NOTIFY: {
        auto notification = reinterpret_cast<NMHDR*>(lp);
        if (notification->hwndFrom == app->outlineTree && notification->code == NM_CUSTOMDRAW) {
            auto draw = reinterpret_cast<NMTVCUSTOMDRAW*>(lp);
            if (draw->nmcd.dwDrawStage == CDDS_PREPAINT) {
                if (app->outlineHighlight) app->PaintOutlineHighlight(draw->nmcd.hdc, app->outlineHighlight);
                return CDRF_NOTIFYITEMDRAW;
            }
            if (draw->nmcd.dwDrawStage == CDDS_ITEMPREPAINT && app->outlineHighlight &&
                reinterpret_cast<HTREEITEM>(draw->nmcd.dwItemSpec) == app->outlineHighlight) {
                draw->clrTextBk = app->OutlineTint(); return CDRF_NEWFONT;
            }
            return CDRF_DODEFAULT;
        }
        if (notification->hwndFrom == app->outlineTree && notification->code == TVN_ITEMEXPANDEDW) {
            app->UpdateOutlineHighlight(false); return 0;
        }
        if (notification->hwndFrom == app->outlineTree && notification->code == TVN_KEYDOWN &&
            reinterpret_cast<NMTVKEYDOWN*>(lp)->wVKey == VK_RETURN) app->NavigateOutline();
        return 0;
    }
    case WM_CLOSE: app->SetOutlinePinned(false); SetFocus(app->window); return 0;
    case WM_DESTROY: app->outlineWindow = app->outlineTree = app->outlineLabel = app->outlineFilter = nullptr; return 0;
    }
    return DefWindowProcW(window, message, wp, lp);
}

// The outline tree navigates on a click. Taking the position from the mouse
// message itself (lParam) is reliable: GetMessagePos can point at a stale
// location while the tree pumps messages, and a miss left filtered rows inert.
// The outline tree navigates on a click. It reacts on the mouse-down (the earliest
// event, so the first click works even while the window is being activated) and
// also on the mouse-up as a fallback; outlineClickHandled keeps it to one
// navigation per click. Taking the position from the message itself (lParam) is
// reliable: GetMessagePos can point at a stale location while the tree pumps
// messages, and a miss left filtered rows inert.
LRESULT CALLBACK OutlineTreeProc(HWND window, UINT message, WPARAM wp, LPARAM lp, UINT_PTR, DWORD_PTR) {
    if (message == WM_NCDESTROY) RemoveWindowSubclass(window, OutlineTreeProc, 1);
    if (message == WM_LBUTTONDOWN || message == WM_LBUTTONUP) {
        LRESULT result = DefSubclassProc(window, message, wp, lp);
        auto app = reinterpret_cast<Reader*>(GetWindowLongPtrW(GetAncestor(window, GA_ROOT), GWLP_USERDATA));
        if (app && message == WM_LBUTTONDOWN) app->outlineClickHandled = false;
        TVHITTESTINFO hit{};
        hit.pt = {GET_X_LPARAM(lp), GET_Y_LPARAM(lp)};
        HTREEITEM item = TreeView_HitTest(window, &hit);
        // TVS_FULLROWSELECT makes the whole row clickable; accept every part of the
        // row except the expand/collapse button and a state icon.
        if (app && !app->outlineClickHandled && item &&
            (hit.flags & (TVHT_ONITEMLABEL | TVHT_ONITEMICON | TVHT_ONITEMRIGHT | TVHT_ONITEMINDENT)) &&
            !(hit.flags & (TVHT_ONITEMBUTTON | TVHT_ONITEMSTATEICON))) {
            app->outlineClickHandled = true;
            app->ActivateOutlineItem(item);
        }
        return result;
    }
    return DefSubclassProc(window, message, wp, lp);
}

LRESULT CALLBACK FlatButtonProc(HWND window, UINT message, WPARAM wp, LPARAM lp, UINT_PTR, DWORD_PTR) {
    if (message == WM_NCCALCSIZE || message == WM_NCPAINT) return 0;
    if (message == WM_ERASEBKGND) return 1;
    if (message == WM_NCDESTROY) RemoveWindowSubclass(window, FlatButtonProc, 1);
    return DefSubclassProc(window, message, wp, lp);
}

LRESULT CALLBACK HomeListProc(HWND window, UINT message, WPARAM wp, LPARAM lp, UINT_PTR, DWORD_PTR) {
    if (message == WM_ERASEBKGND) {
        HDC dc = reinterpret_cast<HDC>(wp);
        RECT rc{}; GetClientRect(window, &rc);
        int count = static_cast<int>(SendMessageW(window, LB_GETCOUNT, 0, 0));
        int top = 0;
        if (count > 0) {
            RECT last{};
            if (SendMessageW(window, LB_GETITEMRECT, count - 1, reinterpret_cast<LPARAM>(&last)) != LB_ERR)
                top = std::max<int>(0, static_cast<int>(last.bottom));
        }
        if (top < rc.bottom) {
            RECT empty{rc.left, top, rc.right, rc.bottom};
            SetDCBrushColor(dc, UiTheme().bar);
            FillRect(dc, &empty, reinterpret_cast<HBRUSH>(GetStockObject(DC_BRUSH)));
        }
        return 1;
    }
    if (message == WM_LBUTTONUP) {
        // Let the list box finish its own selection/notification processing first.
        LRESULT result = DefSubclassProc(window, message, wp, lp);
        LRESULT item = SendMessageW(window, LB_ITEMFROMPOINT, 0, lp);
        if (item == LB_ERR || HIWORD(item) != 0) return result;
        int index = static_cast<int>(LOWORD(item));
        RECT bounds{};
        if (!SendMessageW(window, LB_GETITEMRECT, index, reinterpret_cast<LPARAM>(&bounds))) return result;
        POINT point{GET_X_LPARAM(lp), GET_Y_LPARAM(lp)};
        // LB_ITEMFROMPOINT can return the nearest row for an empty click below the
        // list; only open when the pointer actually landed on the item.
        if (!PtInRect(&bounds, point)) return result;
        if (auto app = reinterpret_cast<Reader*>(GetWindowLongPtrW(GetAncestor(window, GA_ROOT), GWLP_USERDATA)))
            app->Command(RecentBase + index);
        return result;
    }
    if (message == WM_NCDESTROY) RemoveWindowSubclass(window, HomeListProc, 1);
    return DefSubclassProc(window, message, wp, lp);
}

LRESULT CALLBACK IconButtonProc(HWND window, UINT message, WPARAM wp, LPARAM lp, UINT_PTR, DWORD_PTR) {
    auto paint = [&](HDC dc) {
        auto app = reinterpret_cast<Reader*>(GetWindowLongPtrW(GetAncestor(window, GA_ROOT), GWLP_USERDATA));
        if (!app || !dc) return;
        DRAWITEMSTRUCT item{}; item.CtlID = GetDlgCtrlID(window); item.hwndItem = window; item.hDC = dc;
        GetClientRect(window, &item.rcItem);
        HWND root = GetAncestor(window, GA_ROOT);
        bool keyboardFocus = GetFocus() == window && !(SendMessageW(root, WM_QUERYUISTATE, 0, 0) & UISF_HIDEFOCUS);
        bool pressed = GetCapture() == window && (GetKeyState(VK_LBUTTON) & 0x8000) != 0;
        item.itemState = (keyboardFocus ? ODS_FOCUS : 0) | (pressed ? ODS_SELECTED : 0);
        app->PaintButton(item);
    };
    // Owner-drawn buttons still get a classic 3D highlight from the button
    // class. Remove the non-client inset and paint the whole control ourselves.
    if (message == WM_NCCALCSIZE) return 0;
    if (message == WM_NCPAINT) return 0;
    if (message == WM_ERASEBKGND) return 1;
    if (message == WM_THEMECHANGED) {
        // Windows may re-enable the native button theme after a theme change,
        // which would draw a hot border on hover. Only clear it when a theme is
        // actually attached, so SetWindowTheme does not recurse forever.
        if (GetWindowTheme(window) != nullptr) {
            SetWindowTheme(window, L"", L"");
            InvalidateRect(window, nullptr, FALSE);
        }
        return 0;
    }
    if (message == WM_PAINT) {
        PAINTSTRUCT ps{}; HDC dc = BeginPaint(window, &ps); paint(dc); EndPaint(window, &ps); return 0;
    }
    if (message == WM_PRINTCLIENT) { paint(reinterpret_cast<HDC>(wp)); return 0; }
    if (message == WM_SETFOCUS || message == WM_KILLFOCUS) {
        InvalidateRect(window, nullptr, FALSE);
        return DefSubclassProc(window, message, wp, lp);
    }
    if (message == WM_LBUTTONDOWN)
        SendMessageW(GetAncestor(window, GA_ROOT), WM_CHANGEUISTATE, MAKEWPARAM(UIS_SET, UISF_HIDEFOCUS), 0);
    if (message == WM_MOUSEMOVE) {
        if (!GetPropW(window, L"ChucaoHover")) {
            SetPropW(window, L"ChucaoHover", reinterpret_cast<HANDLE>(1));
            TRACKMOUSEEVENT track{sizeof(track), TME_LEAVE, window, 0};
            TrackMouseEvent(&track);
            InvalidateRect(window, nullptr, FALSE);
        }
        if (GetDlgCtrlID(window) == OutlineRail) SendMessageW(GetParent(window), ChromeHover, 0, 0);
    } else if (message == WM_MOUSELEAVE) {
        if (GetPropW(window, L"ChucaoHover")) {
            RemovePropW(window, L"ChucaoHover");
            InvalidateRect(window, nullptr, FALSE);
        }
    } else if (message == WM_NCDESTROY) {
        RemovePropW(window, L"ChucaoHover");
        RemoveWindowSubclass(window, IconButtonProc, 1);
    }
    return DefSubclassProc(window, message, wp, lp);
}
} // namespace
