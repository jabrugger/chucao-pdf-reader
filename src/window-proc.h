#pragma once
// Main window procedure.
// Internal fragment of main.cpp: include it only from there, after the Windows/PDFium headers.
namespace {
LRESULT CALLBACK WindowProc(HWND window, UINT message, WPARAM wp, LPARAM lp) {
    auto app = reinterpret_cast<Reader*>(GetWindowLongPtrW(window, GWLP_USERDATA));
    if (message == WM_NCCREATE) {
        app = static_cast<Reader*>(reinterpret_cast<CREATESTRUCTW*>(lp)->lpCreateParams);
        app->window = window; SetWindowLongPtrW(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(app));
    }
    if (!app) return DefWindowProcW(window, message, wp, lp);
    switch (message) {
    case WM_CREATE: {
        app->dpi = WindowDpi(window);
        app->CreateFonts();
        SendMessageW(window, WM_SETICON, ICON_SMALL, reinterpret_cast<LPARAM>(LoadImageW(GetModuleHandleW(nullptr), MAKEINTRESOURCEW(IDI_CHUCAO), IMAGE_ICON, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), LR_SHARED)));
        app->tooltip = CreateWindowExW(0, TOOLTIPS_CLASSW, nullptr, WS_POPUP | TTS_ALWAYSTIP | TTS_NOPREFIX,
            CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, window, nullptr, nullptr, nullptr);
        SendMessageW(app->tooltip, TTM_SETMAXTIPWIDTH, 0, app->D(320));
        struct Control { const wchar_t* text; int id, x; };
        Control buttons[] = {{L"Abrir PDF (Ctrl+O)", OpenMenu, 16},
            {L"Mostrar u ocultar el índice (Ctrl+B)", Outline, 140}, {L"Buscar en el documento (Ctrl+F)", Find, 180},
            {L"Página anterior", Previous, 248}, {L"Página siguiente", Next, 396}, {L"Alejar", Smaller, 460},
            {L"Acercar", Larger, 564}, {L"Ajustar al ancho (Ctrl+0); página (Ctrl+1); alto (Ctrl+2)", Fit, 604},
            {L"Imprimir (Ctrl+P)", Print, 664}};
        for (const auto& button : buttons) app->IconButton(window, button.id, button.text);
        app->IconButton(window, Theme, L"Opciones de tema y sesión");
        app->IconButton(window, About, L"Acerca de Chucao (F1)");
        app->homeList = CreateWindowW(L"LISTBOX", L"PDF abiertos recientemente", WS_CHILD | WS_TABSTOP | WS_VSCROLL |
            LBS_NOTIFY | LBS_OWNERDRAWFIXED | LBS_HASSTRINGS | LBS_NOINTEGRALHEIGHT, 0, 0, 1, 1, window,
            reinterpret_cast<HMENU>(static_cast<INT_PTR>(HomeList)), nullptr, nullptr);
        // A single click must open the recent PDF even when the item was already
        // selected, so wait for the button-up instead of relying on LBN_SELCHANGE.
        SetWindowSubclass(app->homeList, HomeListProc, 1, 0);
        app->IconButton(window, OutlineRail, L"Desplegar el índice");
        app->outlineWindow = CreateWindowExW(WS_EX_CONTROLPARENT, L"ChucaoOutline", L"Contenido del documento", WS_CHILD | WS_CLIPCHILDREN,
            0, app->ContentTop(), app->D(260), app->D(500), window, nullptr, GetModuleHandleW(nullptr), app);
        app->query = CreateWindowExW(0, L"EDIT", L"", WS_CHILD | WS_TABSTOP | ES_AUTOHSCROLL, 0, 0, app->D(220), app->D(26), window, reinterpret_cast<HMENU>(Query), nullptr, nullptr);
        SendMessageW(app->query, EM_SETCUEBANNER, FALSE, reinterpret_cast<LPARAM>(L"Buscar en el documento"));
        SetAccessibleName(app->query, L"Buscar en el documento");
        app->pageEdit = CreateWindowExW(0, L"EDIT", L"", WS_CHILD | WS_TABSTOP | ES_NUMBER | ES_CENTER, 0, 0, app->D(98), app->D(30), window, reinterpret_cast<HMENU>(static_cast<INT_PTR>(PageEdit)), nullptr, nullptr);
        SendMessageW(app->pageEdit, EM_SETLIMITTEXT, 10, 0);
        SetAccessibleName(app->pageEdit, L"Ir a la página");
        struct Accessible { int id; const wchar_t* name; };
        const Accessible names[] = {{OpenMenu, L"Abrir PDF y recientes"}, {Outline, L"Mostrar u ocultar el índice"},
            {Find, L"Buscar en el documento"}, {Previous, L"Página anterior"}, {Next, L"Página siguiente"},
            {Smaller, L"Alejar"}, {Larger, L"Acercar"}, {Fit, L"Ajustar al ancho, a la página o al alto"}, {Print, L"Imprimir"},
            {Theme, L"Opciones de tema y sesión"}, {About, L"Acerca de Chucao"}};
        for (const auto& entry : names) SetAccessibleName(GetDlgItem(window, entry.id), entry.name);
        app->findLabel = CreateWindowW(L"STATIC", L"Buscar", WS_CHILD, 0, 0, 1, 1, window, nullptr, nullptr, nullptr);
        app->findFeedback = CreateWindowW(L"STATIC", L"", WS_CHILD, 0, 0, 1, 1, window, nullptr, nullptr, nullptr);
        app->statusBar = CreateWindowW(L"STATIC", L"", WS_CHILD | WS_VISIBLE | SS_LEFT | SS_CENTERIMAGE | SS_NOPREFIX,
            0, 0, 1, 1, window, nullptr, nullptr, nullptr);
        app->IconButton(window, FindPrevious, L"Coincidencia anterior (Mayús+Enter)");
        app->IconButton(window, FindNext, L"Coincidencia siguiente (Enter / F3)");
        app->IconButton(window, FindClose, L"Cerrar búsqueda (Escape)");
        SendMessageW(app->query, EM_SETLIMITTEXT, 256, 0);
        app->ApplyFonts();
        app->LoadTheme();
        app->SyncStatus();
        app->ArrangeChrome(); DragAcceptFiles(window, TRUE); return 0;
    }
    case WM_DPICHANGED: {
        int next = HIWORD(wp) ? HIWORD(wp) : LOWORD(wp);
        if (next >= 48 && next <= 768) app->dpi = next;
        // Windows supplies the rectangle that keeps the window on the same logical
        // area after crossing monitors with different scale factors.
        RECT* suggested = reinterpret_cast<RECT*>(lp);
        if (suggested) SetWindowPos(window, nullptr, suggested->left, suggested->top,
            suggested->right - suggested->left, suggested->bottom - suggested->top,
            SWP_NOZORDER | SWP_NOACTIVATE);
        app->CreateFonts();
        app->previousContentWidth = 0;
        app->ArrangeChrome();
        app->Reflow(); app->Render();
        InvalidateRect(window, nullptr, FALSE);
        return 0;
    }
    case WM_SETTINGCHANGE:
    case WM_SYSCOLORCHANGE:
    case WM_THEMECHANGED:
        // Re-read high contrast and system colors; the saved theme choice is kept.
        app->ApplyPalette(app->followSystemTheme ? SystemPrefersDark() : app->darkTheme, false);
        return 0;
    case WM_COMMAND:
        if (LOWORD(wp) == HomeList) {
            // Single clicks are handled by HomeListProc. Keep the notification as
            // a fallback for accessibility tools that simulate a double click.
            if (HIWORD(wp) == LBN_DBLCLK) {
                int index = static_cast<int>(SendMessageW(app->homeList, LB_GETCURSEL, 0, 0));
                if (index >= 0) app->Command(RecentBase + index);
            }
        }
        else if (LOWORD(wp) == Query && (HIWORD(wp) == EN_SETFOCUS || HIWORD(wp) == EN_KILLFOCUS)) { app->Redraw(); }
        else if (LOWORD(wp) == PageEdit && HIWORD(wp) == EN_KILLFOCUS) { app->CancelPageEdit(false); }
        else if (LOWORD(wp) == PageEdit) { /* EN_CHANGE/EN_SETFOCUS: the field manages itself. */ }
        else if (LOWORD(wp) == Query && HIWORD(wp) == EN_CHANGE) {
            app->searching = false; app->searchPage = -1; app->searchBoxes.clear();
            SetWindowTextW(app->findFeedback, L"");
            KillTimer(window, 9);
            if (app->searchVisible && app->pages && GetWindowTextLengthW(app->query)) SetTimer(window, 9, 220, nullptr);
            if (app->worker) app->worker->CancelSearch(); app->Redraw();
        } else app->Command(LOWORD(wp));
        return 0;
    case WM_DRAWITEM: app->PaintButton(*reinterpret_cast<DRAWITEMSTRUCT*>(lp)); return TRUE;
    case WM_MEASUREITEM: reinterpret_cast<MEASUREITEMSTRUCT*>(lp)->itemHeight = app->D(56); return TRUE;
    case WM_CTLCOLORLISTBOX:
        SetTextColor(reinterpret_cast<HDC>(wp), UiTheme().ink); SetBkColor(reinterpret_cast<HDC>(wp), UiTheme().bar);
        return reinterpret_cast<LRESULT>(UiBackgroundBrush());
    case WM_CTLCOLORSTATIC:
        SetTextColor(reinterpret_cast<HDC>(wp), UiTheme().muted); SetBkColor(reinterpret_cast<HDC>(wp), UiTheme().bar);
        return reinterpret_cast<LRESULT>(UiBackgroundBrush());
    case WM_CTLCOLOREDIT:
        if (reinterpret_cast<HWND>(lp) == app->query || reinterpret_cast<HWND>(lp) == app->pageEdit) {
            SetTextColor(reinterpret_cast<HDC>(wp), UiTheme().ink); SetBkColor(reinterpret_cast<HDC>(wp), UiTheme().field);
            SetDCBrushColor(reinterpret_cast<HDC>(wp), UiTheme().field);
            return reinterpret_cast<LRESULT>(GetStockObject(DC_BRUSH));
        }
        break;
    case ChromeHover: app->PeekOutline(); return 0;
    case WM_CONTEXTMENU: {
        if (!app->pages) break;
        if (lp == static_cast<LPARAM>(-1)) { app->ShowTextMenuAtSelection(); return 0; }
        POINT screen{GET_X_LPARAM(lp), GET_Y_LPARAM(lp)};
        POINT client = screen; ScreenToClient(window, &client);
        RECT rc{}; GetClientRect(window, &rc);
        // Only the document view gets the text menu; tabs, toolbar, outline and
        // status bar keep their native behavior.
        if (client.x < app->ViewLeft() || client.x >= rc.right ||
            client.y < app->ContentTop() || client.y >= app->ContentTop() + app->ViewHeight()) break;
        app->ShowTextMenu(screen); return 0;
    }
    case WM_LBUTTONDOWN: {
        int x = GET_X_LPARAM(lp), y = GET_Y_LPARAM(lp);
        if (app->TabClick(x, y)) return 0;
        if (app->PageFieldClick(x, y)) return 0;
        int count = app->TrackDocumentClick(x, y);
        if (count >= 2) { app->SelectWordOrLine(x, y, count >= 3 ? 2 : 1); return 0; }
        app->BeginSelection(x, y); return 0;
    }
    case WM_LBUTTONDBLCLK: {
        int x = GET_X_LPARAM(lp), y = GET_Y_LPARAM(lp);
        if (app->TabClick(x, y)) return 0;
        if (app->PageFieldClick(x, y)) return 0;
        int count = app->TrackDocumentClick(x, y);
        app->SelectWordOrLine(x, y, count >= 3 ? 2 : 1);
        return 0;
    }
    case WM_MBUTTONDOWN: {
        bool close = false;
        int index = app->TabHitTest(GET_X_LPARAM(lp), GET_Y_LPARAM(lp), close);
        if (index >= 0) { app->CloseTab(index); return 0; }
        return 0;
    }
    case WM_MOUSEMOVE: {
        app->MoveSelection(GET_X_LPARAM(lp), GET_Y_LPARAM(lp)); app->HoverLinks(GET_X_LPARAM(lp), GET_Y_LPARAM(lp));
        if (GET_X_LPARAM(lp) < app->D(18) && GET_Y_LPARAM(lp) >= app->ContentTop()) app->PeekOutline();
        bool changed = false;
        bool homeHover = app->HomeTabHitTest(GET_X_LPARAM(lp), GET_Y_LPARAM(lp));
        if (app->hoverHomeTab != homeHover) { app->hoverHomeTab = homeHover; changed = true; }
        bool close = false;
        int index = app->TabHitTest(GET_X_LPARAM(lp), GET_Y_LPARAM(lp), close);
        if (app->hoverTab != index || app->hoverTabClose != close) { app->hoverTab = index; app->hoverTabClose = close; changed = true; }
        if (changed) app->Redraw();
        return 0;
    }
    case WM_LBUTTONUP: app->FinishSelection(GET_X_LPARAM(lp), GET_Y_LPARAM(lp)); return 0;
    case WM_CAPTURECHANGED:
        if (reinterpret_cast<HWND>(lp) != window) app->EndSelection(false);
        return 0;
    case WM_SETCURSOR:
        if (reinterpret_cast<HWND>(wp) == window && LOWORD(lp) == HTCLIENT) {
            POINT mouse{}; GetCursorPos(&mouse); ScreenToClient(window, &mouse);
            if (app->pages) {
                RECT field = app->PageFieldRect();
                if (mouse.x >= field.left && mouse.x < field.right && mouse.y >= field.top && mouse.y < field.bottom) {
                    SetCursor(LoadCursor(nullptr, IDC_HAND)); return TRUE;
                }
                Reader::TextPoint point;
                if (app->PointOnPage(mouse.x, mouse.y, point)) { SetCursor(LoadCursor(nullptr, app->LinkAt(point) ? IDC_HAND : IDC_IBEAM)); return TRUE; }
            }
        }
        break;
    case WM_DROPFILES: {
        wchar_t path[32768]{}; HDROP drop = reinterpret_cast<HDROP>(wp);
        if (DragQueryFileW(drop, 0, path, 32768)) app->OpenDocument(path);
        DragFinish(drop); return 0;
    }
    case Ready: app->Receive(std::unique_ptr<Result>(reinterpret_cast<Result*>(lp))); return 0;
    case UpdateReady: app->OnUpdateChecked(reinterpret_cast<UpdateInfo*>(lp)); return 0;
    case WM_SIZE: app->ArrangeChrome(); if (wp != SIZE_MINIMIZED) SetTimer(window, 1, 120, nullptr); return 0;
    case WM_TIMER:
        if (wp == 4 && app->dragging) {
            POINT pointer{}; GetCursorPos(&pointer); ScreenToClient(window, &pointer);
            if (pointer.y < app->ContentTop() + app->D(20)) { app->ScrollTo(app->scroll - app->D(28)); app->MoveSelection(pointer.x, pointer.y); }
            else if (pointer.y > app->ContentTop() + app->ViewHeight() - app->D(20)) { app->ScrollTo(app->scroll + app->D(28)); app->MoveSelection(pointer.x, pointer.y); }
            if (pointer.x < app->ViewLeft() + app->D(20)) { app->ScrollHorizontal(app->horizontal - app->D(28)); app->MoveSelection(pointer.x, pointer.y); }
            else if (pointer.x > app->ViewLeft() + app->ViewWidth() - app->D(20)) { app->ScrollHorizontal(app->horizontal + app->D(28)); app->MoveSelection(pointer.x, pointer.y); }
        }
        else if (wp == 9) { KillTimer(window, 9); if (app->searchVisible) app->Search(); }
        else if (wp == 7) app->CheckOutlinePeek();
        else if (wp == 6) app->CheckReadingState();
        else if (wp == 3) app->CheckNavigation();
        else if (wp == 5) app->TickLoading();
        else if (wp == 10) { KillTimer(window, 10); app->CheckOutlineFilterNavigation(); }
        else if (wp == 2) { KillTimer(window, 2); app->BenchmarkResult(false); }
        else { KillTimer(window, 1); app->Render(); }
        return 0;
    case WM_GETMINMAXINFO:
        reinterpret_cast<MINMAXINFO*>(lp)->ptMinTrackSize = {app->D(820), app->D(400)}; return 0;
    case WM_MOUSEWHEEL: {
        app->pendingReadingState.reset();
        ++app->linkEpoch;
        int delta = app->D(GET_WHEEL_DELTA_WPARAM(wp));
        if (GetKeyState(VK_CONTROL) < 0) {
            POINT screen{GET_X_LPARAM(lp), GET_Y_LPARAM(lp)};
            POINT client = screen; ScreenToClient(window, &client);
            app->Zoom(GET_WHEEL_DELTA_WPARAM(wp) > 0, client.x, client.y);
        }
        else if (GetKeyState(VK_SHIFT) < 0) app->ScrollHorizontal(app->horizontal - delta);
        else app->ScrollTo(app->scroll - delta); return 0;
    }
    case WM_MOUSEHWHEEL:
        app->pendingReadingState.reset(); ++app->linkEpoch;
        app->ScrollHorizontal(app->horizontal + app->D(GET_WHEEL_DELTA_WPARAM(wp))); return 0;
    case WM_HSCROLL: {
        app->pendingReadingState.reset();
        ++app->linkEpoch;
        SCROLLINFO info{sizeof(info), SIF_ALL}; GetScrollInfo(window, SB_HORZ, &info);
        int value = app->horizontal;
        switch (LOWORD(wp)) {
        case SB_LINELEFT: value -= app->D(40); break; case SB_LINERIGHT: value += app->D(40); break;
        case SB_PAGELEFT: value -= info.nPage; break; case SB_PAGERIGHT: value += info.nPage; break;
        case SB_LEFT: value = 0; break; case SB_RIGHT: value = app->layout.MaxHorizontal(app->ViewWidth()); break;
        case SB_THUMBPOSITION: case SB_THUMBTRACK: value = info.nTrackPos; break;
        }
        app->ScrollHorizontal(value); return 0;
    }
    case WM_VSCROLL: {
        app->pendingReadingState.reset();
        ++app->linkEpoch;
        SCROLLINFO info{sizeof(info), SIF_ALL}; GetScrollInfo(window, SB_VERT, &info);
        std::int64_t value = app->scroll;
        switch (LOWORD(wp)) {
        case SB_LINEUP: value -= app->D(40); break; case SB_LINEDOWN: value += app->D(40); break;
        case SB_PAGEUP: value -= info.nPage; break; case SB_PAGEDOWN: value += info.nPage; break;
        case SB_TOP: value = 0; break; case SB_BOTTOM: value = app->MaxScroll(); break;
        case SB_THUMBPOSITION: case SB_THUMBTRACK:
            value = app->layout.Total() < 1000000000 ? info.nTrackPos : static_cast<std::int64_t>(app->MaxScroll() * (info.nTrackPos / (info.nMax - info.nPage + 1.0))); break;
        }
        app->ScrollTo(value); return 0;
    }
    case WM_ERASEBKGND: return 1;
    case WM_CLOSE:
        app->SaveReadingState(); app->SaveSession(); app->SaveWindowPlacement();
        app->savedOnClose = true; DestroyWindow(window); return 0;
    case WM_ENDSESSION:
        if (wp) { app->SaveReadingState(); app->SaveSession(); app->SaveWindowPlacement(); } return 0;
    case WM_PAINT: app->Paint(); return 0;
    case WM_DESTROY: {
        if (!app->savedOnClose) { app->SaveReadingState(); app->SaveWindowPlacement(); }
        app->worker.reset();
        if (app->outlineWindow) DestroyWindow(app->outlineWindow);
        MSG pending{}; while (PeekMessageW(&pending, window, Ready, Ready, PM_REMOVE)) delete reinterpret_cast<Result*>(pending.lParam);
        while (PeekMessageW(&pending, window, UpdateReady, UpdateReady, PM_REMOVE)) delete reinterpret_cast<UpdateInfo*>(pending.lParam);
        DeleteObject(app->font); DeleteObject(app->strongFont); DeleteObject(app->welcomeFont);
        if (app->welcomeImage) DeleteObject(app->welcomeImage);
        delete app->welcomeLogo;
        if (app->backDC) DeleteDC(app->backDC);
        if (app->backBitmap) DeleteObject(app->backBitmap);
        app->statusBar = nullptr;
        PostQuitMessage(0); return 0;
    }
    }
    return DefWindowProcW(window, message, wp, lp);
}
} // namespace
