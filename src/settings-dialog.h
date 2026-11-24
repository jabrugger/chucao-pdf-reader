#pragma once
// Options dialog procedure.
// Internal fragment of main.cpp: include it only from there, after the Windows/PDFium headers.
namespace {
INT_PTR CALLBACK SettingsProc(HWND dialog, UINT message, WPARAM wp, LPARAM lp) {
    auto app = reinterpret_cast<Reader*>(GetWindowLongPtrW(dialog, DWLP_USER));
    if (message == WM_INITDIALOG) {
        app = reinterpret_cast<Reader*>(lp); SetWindowLongPtrW(dialog, DWLP_USER, lp);
        SetSettingChecked(dialog, IDC_SETTINGS_RESTORE, app->restoreLastSession);
        SetSettingChecked(dialog, IDC_SETTINGS_OUTLINE, app->autoShowOutline);
        SetSettingChecked(dialog, IDC_SETTINGS_UPDATES, app->checkUpdates);
        SetSettingChecked(dialog, IDC_SETTINGS_ASSOCIATE,
            AssociationState(FileAssociationTarget{}) == FileAssociationState::Default);
        SetAccessibleName(GetDlgItem(dialog, IDC_SETTINGS_RESTORE), L"Reabrir los PDF de la última sesión");
        SetAccessibleName(GetDlgItem(dialog, IDC_SETTINGS_OUTLINE), L"Mostrar automáticamente el índice");
        SetAccessibleName(GetDlgItem(dialog, IDC_SETTINGS_UPDATES), L"Buscar actualizaciones al abrir");
        SetAccessibleName(GetDlgItem(dialog, IDC_SETTINGS_ASSOCIATE), L"Abrir los PDF con Chucao");
        SetAccessibleName(GetDlgItem(dialog, IDC_SETTINGS_THEME_SYSTEM), L"Tema igual que el sistema");
        SetAccessibleName(GetDlgItem(dialog, IDC_SETTINGS_THEME_LIGHT), L"Tema claro");
        SetAccessibleName(GetDlgItem(dialog, IDC_SETTINGS_THEME_DARK), L"Tema oscuro");
        for (int id : {IDC_SETTINGS_THEME_SYSTEM, IDC_SETTINGS_THEME_LIGHT, IDC_SETTINGS_THEME_DARK,
                IDC_SETTINGS_RESTORE, IDC_SETTINGS_OUTLINE, IDC_SETTINGS_UPDATES, IDC_SETTINGS_ASSOCIATE,
                IDC_SETTINGS_DEFAULTAPPS, IDC_SETTINGS_CLEAR}) {
            HWND button = GetDlgItem(dialog, id);
            SetWindowTheme(button, L"", L"");
            SetWindowSubclass(button, FlatButtonProc, 1, 0);
            InvalidateRect(button, nullptr, FALSE);
        }
        ApplyWindowTheme(dialog, UiDark());
        RECT owner{}, bounds{}; GetWindowRect(app->window, &owner); GetWindowRect(dialog, &bounds);
        SetWindowPos(dialog, nullptr, owner.left + ((owner.right - owner.left) - (bounds.right - bounds.left)) / 2,
            owner.top + ((owner.bottom - owner.top) - (bounds.bottom - bounds.top)) / 2, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
        return TRUE;
    }
    if (!app) return FALSE;
    // The dialog paints its own background with the current palette. Relying on the
    // default erase made the dialog keep the old (dark) background after switching
    // to light, while its children repainted light.
    if (message == WM_ERASEBKGND) {
        RECT client{}; GetClientRect(dialog, &client);
        FillRect(reinterpret_cast<HDC>(wp), &client, UiBackgroundBrush());
        SetWindowLongPtrW(dialog, DWLP_MSGRESULT, 1); return TRUE;
    }
    if (message == WM_CTLCOLORDLG || message == WM_CTLCOLORSTATIC || message == WM_CTLCOLORBTN || message == WM_CTLCOLORLISTBOX) {
        if (message != WM_CTLCOLORDLG) { SetTextColor(reinterpret_cast<HDC>(wp), UiTheme().ink); SetBkColor(reinterpret_cast<HDC>(wp), UiTheme().bar); }
        return reinterpret_cast<INT_PTR>(UiBackgroundBrush());
    }
    if (message == WM_DRAWITEM) {
        auto item = reinterpret_cast<DRAWITEMSTRUCT*>(lp);
        if (!item) return FALSE;
        if (item->CtlID == IDC_SETTINGS_RESTORE || item->CtlID == IDC_SETTINGS_OUTLINE ||
            item->CtlID == IDC_SETTINGS_UPDATES || item->CtlID == IDC_SETTINGS_ASSOCIATE) {
            HDC dc = item->hDC;
            RECT rect = item->rcItem;
            SetDCBrushColor(dc, UiTheme().bar);
            FillRect(dc, &rect, reinterpret_cast<HBRUSH>(GetStockObject(DC_BRUSH)));
            bool checked = SettingChecked(dialog, static_cast<int>(item->CtlID));
            int dpi = WindowDpi(dialog);
            int boxSize = MulDiv(15, dpi, 96);
            int boxTop = rect.top + (rect.bottom - rect.top - boxSize) / 2;
            RECT box{rect.left + MulDiv(2, dpi, 96), boxTop,
                rect.left + MulDiv(2, dpi, 96) + boxSize, boxTop + boxSize};
            COLORREF boxFill = checked ? UiTheme().accent :
                (UiDark() ? RGB(0x47, 0x50, 0x63) : UiTheme().field);
            COLORREF boxBorder = checked ? UiTheme().accent :
                (UiDark() ? RGB(0x4C, 0x56, 0x6A) : UiTheme().line);
            UiRoundRect(dc, box, boxFill, boxBorder, MulDiv(3, dpi, 96));
            if (checked) {
                Gdiplus::Graphics graphics(dc);
                if (graphics.GetLastStatus() == Gdiplus::Ok) {
                    graphics.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
                    float scale = static_cast<float>(dpi) / 96.0f;
                    Gdiplus::Pen check(UiDraw::Color(UiDark() ? UiTheme().bar : RGB(255, 255, 255)),
                        std::max(1.6f, 2.0f * scale));
                    check.SetLineCap(Gdiplus::LineCapRound, Gdiplus::LineCapRound, Gdiplus::DashCapRound);
                    float left = static_cast<float>(box.left), top = static_cast<float>(box.top);
                    float w = static_cast<float>(box.right - box.left), h = static_cast<float>(box.bottom - box.top);
                    graphics.DrawLine(&check, left + w * 0.23f, top + h * 0.52f, left + w * 0.43f, top + h * 0.72f);
                    graphics.DrawLine(&check, left + w * 0.43f, top + h * 0.72f, left + w * 0.78f, top + h * 0.29f);
                }
            }
            if (HFONT font = reinterpret_cast<HFONT>(SendMessageW(item->hwndItem, WM_GETFONT, 0, 0))) SelectObject(dc, font);
            SetBkMode(dc, TRANSPARENT);
            SetTextColor(dc, UiTheme().ink);
            std::wstring label = Text(item->hwndItem);
            RECT text{box.right + MulDiv(8, dpi, 96), rect.top, rect.right - MulDiv(4, dpi, 96), rect.bottom};
            DrawTextW(dc, label.c_str(), -1, &text, DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX | DT_END_ELLIPSIS);
            if ((item->itemState & ODS_FOCUS) &&
                !(SendMessageW(GetAncestor(dialog, GA_ROOT), WM_QUERYUISTATE, 0, 0) & UISF_HIDEFOCUS)) {
                RECT ring = box; InflateRect(&ring, MulDiv(2, dpi, 96), MulDiv(2, dpi, 96));
                DrawFocusRect(dc, &ring);
            }
            return TRUE;
        }
        if (item->CtlID == IDC_SETTINGS_THEME_SYSTEM || item->CtlID == IDC_SETTINGS_THEME_LIGHT ||
            item->CtlID == IDC_SETTINGS_THEME_DARK) {
            int choice = item->CtlID == IDC_SETTINGS_THEME_SYSTEM ? 0 :
                item->CtlID == IDC_SETTINGS_THEME_LIGHT ? 1 : 2;
            int current = app->followSystemTheme ? 0 : app->darkTheme ? 2 : 1;
            bool selected = choice == current;
            POINT cursor{}; RECT windowRect{};
            GetCursorPos(&cursor); GetWindowRect(item->hwndItem, &windowRect);
            bool hovered = PtInRect(&windowRect, cursor) != FALSE;
            HDC dc = item->hDC;
            RECT rect = item->rcItem;
            COLORREF fill = selected ? UiTheme().accent : hovered ? UiTheme().hover : UiTheme().field;
            UiRoundRect(dc, rect, fill, UiTheme().line, MulDiv(6, WindowDpi(dialog), 96));
            if (HFONT font = reinterpret_cast<HFONT>(SendMessageW(item->hwndItem, WM_GETFONT, 0, 0))) SelectObject(dc, font);
            SetBkMode(dc, TRANSPARENT);
            SetTextColor(dc, selected ? RGB(255, 255, 255) : UiTheme().ink);
            std::wstring label = Text(item->hwndItem);
            DrawTextW(dc, label.c_str(), -1, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX | DT_END_ELLIPSIS);
            if ((item->itemState & ODS_FOCUS) &&
                !(SendMessageW(GetAncestor(dialog, GA_ROOT), WM_QUERYUISTATE, 0, 0) & UISF_HIDEFOCUS))
                DrawFocusRect(dc, &rect);
            return TRUE;
        }
        if (item->CtlID != IDC_SETTINGS_DEFAULTAPPS && item->CtlID != IDC_SETTINGS_CLEAR) return FALSE;
        HDC dc = item->hDC;
        RECT rect = item->rcItem;
        SetDCBrushColor(dc, UiTheme().bar);
        FillRect(dc, &rect, reinterpret_cast<HBRUSH>(GetStockObject(DC_BRUSH)));
        if (HFONT font = reinterpret_cast<HFONT>(SendMessageW(item->hwndItem, WM_GETFONT, 0, 0))) SelectObject(dc, font);
        SetBkMode(dc, TRANSPARENT);
        SetTextColor(dc, UiTheme().accent);
        std::wstring label = Text(item->hwndItem);
        RECT text = rect; text.left += app->D(2); text.right -= app->D(2);
        DrawTextW(dc, label.c_str(), -1, &text, DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX | DT_END_ELLIPSIS);
        SIZE extent{};
        if (GetTextExtentPoint32W(dc, label.c_str(), static_cast<int>(label.size()), &extent)) {
            int y = rect.top + (rect.bottom - rect.top + extent.cy) / 2 + app->D(1);
            HPEN pen = CreatePen(PS_SOLID, 1, UiTheme().accent);
            HGDIOBJ old = SelectObject(dc, pen);
            MoveToEx(dc, text.left, y, nullptr);
            LineTo(dc, std::min<int>(text.left + extent.cx, text.right), y);
            SelectObject(dc, old); DeleteObject(pen);
        }
        if ((item->itemState & ODS_FOCUS) &&
            !(SendMessageW(GetAncestor(dialog, GA_ROOT), WM_QUERYUISTATE, 0, 0) & UISF_HIDEFOCUS))
            DrawFocusRect(dc, &rect);
        return TRUE;
    }
    if (message == WM_COMMAND) {
        int id = LOWORD(wp);
        if (id == IDOK || id == IDCANCEL) { EndDialog(dialog, id); return TRUE; }
        if (id == IDC_SETTINGS_DEFAULTAPPS && HIWORD(wp) == BN_CLICKED) {
            if (reinterpret_cast<INT_PTR>(ShellExecuteW(dialog, L"open", L"ms-settings:defaultapps", nullptr, nullptr, SW_SHOWNORMAL)) <= 32)
                SetDlgItemTextW(dialog, IDC_SETTINGS_STATUS, L"No se pudo abrir la configuración de Windows.");
            return TRUE;
        }
        if (id == IDC_SETTINGS_CLEAR && HIWORD(wp) == BN_CLICKED) {
            // Confirm over the Options dialog, not over the main window that this
            // modal dialog has disabled.
            app->ClearReadingHistory(true, dialog); return TRUE;
        }
        if (id == IDC_SETTINGS_ASSOCIATE && HIWORD(wp) == BN_CLICKED) {
            bool want = !SettingChecked(dialog, IDC_SETTINGS_ASSOCIATE);
            SetSettingChecked(dialog, IDC_SETTINGS_ASSOCIATE, want);
            std::wstring error;
            bool ok = want ? RegisterFileAssociation(ExecutablePath(), FileAssociationTarget{}, error) :
                UnregisterFileAssociation(FileAssociationTarget{}, error);
            auto state = AssociationState(FileAssociationTarget{});
            SetSettingChecked(dialog, IDC_SETTINGS_ASSOCIATE,
                state == FileAssociationState::Default);
            SetDlgItemTextW(dialog, IDC_SETTINGS_STATUS, !ok ? L"No se pudo cambiar la asociación de PDF." :
                want && state != FileAssociationState::Default ?
                L"Chucao quedó registrado. Si Windows mantiene otra app, elegí Chucao en Configuración > Aplicaciones predeterminadas." :
                want ? L"Chucao quedó asociado a los PDF." : L"Asociación quitada.");
            return TRUE;
        }
        auto saveOptions = [&]() {
            AppSettings value = app->CollectSettings();
            value.restoreSession = SettingChecked(dialog, IDC_SETTINGS_RESTORE);
            value.autoShowOutline = SettingChecked(dialog, IDC_SETTINGS_OUTLINE);
            value.checkUpdates = SettingChecked(dialog, IDC_SETTINGS_UPDATES);
            if (!app->ConfigureSettings() || !SettingsStore(app->settingsFile).Write(value)) {
                SetSettingChecked(dialog, IDC_SETTINGS_RESTORE, app->restoreLastSession);
                SetSettingChecked(dialog, IDC_SETTINGS_OUTLINE, app->autoShowOutline);
                SetSettingChecked(dialog, IDC_SETTINGS_UPDATES, app->checkUpdates);
                SetDlgItemTextW(dialog, IDC_SETTINGS_STATUS, L"No se pudieron guardar las opciones."); return;
            }
            bool wasRestoring = app->restoreLastSession;
            app->restoreLastSession = value.restoreSession;
            app->autoShowOutline = value.autoShowOutline; // applies from the next document on
            app->checkUpdates = value.checkUpdates;
            bool saved = value.restoreSession ? app->SaveSession() : !wasRestoring || SessionStore(app->SessionFile()).Write({});
            SetDlgItemTextW(dialog, IDC_SETTINGS_STATUS, saved ? L"" : L"No se pudo actualizar la sesión guardada.");
            // No palette work and no reader redraw: these flags do not alter the
            // current document, so repainting the main window only caused a flash.
        };
        auto applyThemeChoice = [&](int choice) {
            bool system = choice == 0;
            // Keep the current window placement while changing the other options.
            AppSettings value = app->CollectSettings();
            value.dark = system ? SystemPrefersDark() : choice == 2;
            value.followSystem = system;
            value.restoreSession = SettingChecked(dialog, IDC_SETTINGS_RESTORE);
            value.autoShowOutline = SettingChecked(dialog, IDC_SETTINGS_OUTLINE);
            value.checkUpdates = SettingChecked(dialog, IDC_SETTINGS_UPDATES);
            if (!app->ConfigureSettings() || !SettingsStore(app->settingsFile).Write(value)) {
                for (int themeId : {IDC_SETTINGS_THEME_SYSTEM, IDC_SETTINGS_THEME_LIGHT, IDC_SETTINGS_THEME_DARK})
                    InvalidateRect(GetDlgItem(dialog, themeId), nullptr, FALSE);
                SetSettingChecked(dialog, IDC_SETTINGS_RESTORE, app->restoreLastSession);
                SetSettingChecked(dialog, IDC_SETTINGS_OUTLINE, app->autoShowOutline);
                SetSettingChecked(dialog, IDC_SETTINGS_UPDATES, app->checkUpdates);
                SetDlgItemTextW(dialog, IDC_SETTINGS_STATUS, L"No se pudieron guardar las opciones."); return;
            }
            bool wasRestoring = app->restoreLastSession;
            app->restoreLastSession = value.restoreSession; app->followSystemTheme = value.followSystem;
            app->autoShowOutline = value.autoShowOutline; // applies from the next document on
            app->checkUpdates = value.checkUpdates;
            SendMessageW(dialog, WM_SETREDRAW, FALSE, 0);
            app->ApplyPalette(value.dark, false);
            SendMessageW(dialog, WM_SETREDRAW, TRUE, 0);
            bool saved = value.restoreSession ? app->SaveSession() : !wasRestoring || SessionStore(app->SessionFile()).Write({});
            SetDlgItemTextW(dialog, IDC_SETTINGS_STATUS, saved ? L"" : L"No se pudo actualizar la sesión guardada.");
            ApplyWindowTheme(dialog, UiDark());
            // This small modal dialog does need its background erased with the new palette;
            // the main window's redraw (which must not erase) is handled in ApplyPalette.
            RedrawWindow(dialog, nullptr, nullptr, RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN);
        };
        if (id == IDC_SETTINGS_THEME_SYSTEM || id == IDC_SETTINGS_THEME_LIGHT || id == IDC_SETTINGS_THEME_DARK) {
            if (HIWORD(wp) == BN_CLICKED)
                applyThemeChoice(id == IDC_SETTINGS_THEME_SYSTEM ? 0 : id == IDC_SETTINGS_THEME_LIGHT ? 1 : 2);
            return TRUE;
        }
        if ((id == IDC_SETTINGS_RESTORE || id == IDC_SETTINGS_OUTLINE || id == IDC_SETTINGS_UPDATES) && HIWORD(wp) == BN_CLICKED) {
            SetSettingChecked(dialog, id, !SettingChecked(dialog, id));
            saveOptions();
            return TRUE;
        }
    }
    return FALSE;
}
} // namespace
