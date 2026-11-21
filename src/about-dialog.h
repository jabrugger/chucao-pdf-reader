#pragma once
// About dialog procedure.
// Internal fragment of main.cpp: include it only from there, after the Windows/PDFium headers.
namespace {
INT_PTR CALLBACK AboutProc(HWND dialog, UINT message, WPARAM wp, LPARAM lp) {
    if (message == WM_CTLCOLORDLG) return reinterpret_cast<INT_PTR>(UiBackgroundBrush());
    if (message == WM_CTLCOLORSTATIC) {
        SetBkMode(reinterpret_cast<HDC>(wp), TRANSPARENT);
        SetTextColor(reinterpret_cast<HDC>(wp), UiTheme().ink);
        return reinterpret_cast<INT_PTR>(UiBackgroundBrush());
    }
    if (message == WM_INITDIALOG) {
        ApplyWindowTheme(dialog, UiDark());
        SetDlgItemTextW(dialog, IDC_ABOUT_VERSION, L"Versión " CHUCAO_WIDE(CHUCAO_VERSION_STRING));
        SetDlgItemTextW(dialog, IDC_ABOUT_AUTHOR, CHUCAO_WIDE(CHUCAO_AUTHOR));
        SetDlgItemTextW(dialog, IDC_ABOUT_EMAIL, CHUCAO_WIDE(CHUCAO_EMAIL));
        SetDlgItemTextW(dialog, IDC_ABOUT_RELEASE, L"Fecha de release: " CHUCAO_WIDE(CHUCAO_RELEASE_DATE));
        {   // Make the bird as tall as the text beside it, square, whatever the DPI.
            auto clientRect = [&](int id) { RECT r{}; GetWindowRect(GetDlgItem(dialog, id), &r); MapWindowPoints(nullptr, dialog, reinterpret_cast<POINT*>(&r), 2); return r; };
            RECT title = clientRect(IDC_ABOUT_TITLE), release = clientRect(IDC_ABOUT_RELEASE), link = clientRect(IDC_ABOUT_REPOSITORY), icon = clientRect(IDC_ABOUT_ICON);
            int side = std::max<LONG>(32, link.top + (release.bottom - release.top) - title.top);
            if (HICON bird = static_cast<HICON>(LoadImageW(GetModuleHandleW(nullptr), MAKEINTRESOURCEW(IDI_CHUCAO), IMAGE_ICON, side, side, LR_DEFAULTCOLOR))) {
                SetWindowPos(GetDlgItem(dialog, IDC_ABOUT_ICON), nullptr, icon.left, title.top, side, side, SWP_NOZORDER);
                SendDlgItemMessageW(dialog, IDC_ABOUT_ICON, STM_SETICON, reinterpret_cast<WPARAM>(bird), 0);
            }
        }
        if (HWND list = GetDlgItem(dialog, IDC_ABOUT_HOTKEYS)) {
            ListView_SetExtendedListViewStyle(list, LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);
            ListView_SetBkColor(list, UiTheme().field);
            ListView_SetTextBkColor(list, UiTheme().field);
            ListView_SetTextColor(list, UiTheme().ink);
            SetWindowTheme(list, HighContrastActive() ? L"" : (UiDark() ? L"DarkMode_Explorer" : L"Explorer"), nullptr);
            SendMessageW(list, WM_SETFONT, SendMessageW(dialog, WM_GETFONT, 0, 0), TRUE);
            RECT area{}; GetClientRect(list, &area);
            LVCOLUMNW column{}; column.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
            int firstWidth = area.right * 28 / 100;
            int spare = MulDiv(20, WindowDpi(dialog), 96);
            column.pszText = const_cast<wchar_t*>(L"Atajo"); column.cx = firstWidth; column.iSubItem = 0;
            ListView_InsertColumn(list, 0, &column);
            column.pszText = const_cast<wchar_t*>(L"Acción"); column.cx = static_cast<int>(std::max<LONG>(1, area.right - firstWidth - spare)); column.iSubItem = 1;
            ListView_InsertColumn(list, 1, &column);
            struct Row { const wchar_t* keys; const wchar_t* action; };
            static const Row rows[] = {
                {L"Ctrl+O", L"Abrir un PDF"},
                {L"Ctrl+T", L"Abrir un PDF en una pestaña nueva"},
                {L"Ctrl+W", L"Cerrar la pestaña activa (en Inicio, volver al documento)"},
                {L"Ctrl+Tab", L"Pestaña siguiente"},
                {L"Ctrl+Mayús+Tab", L"Pestaña anterior"},
                {L"Alt+Home", L"Volver a la pestaña de recientes"},
                {L"Ctrl+B", L"Mostrar u ocultar el índice"},
                {L"Ctrl+F", L"Buscar en el documento"},
                {L"F3 / Mayús+F3", L"Siguiente / anterior coincidencia"},
                {L"Enter", L"Abrir el reciente o capítulo enfocado; en Buscar, siguiente"},
                {L"Ctrl+C", L"Copiar el texto seleccionado"},
                {L"Tecla Menú / Mayús+F10", L"Menú contextual de copia"},
                {L"Ctrl+P", L"Imprimir"},
                {L"Ctrl+0", L"Ajustar al ancho"},
                {L"Ctrl+1", L"Ajustar a la página"},
                {L"Ctrl+2", L"Ajustar al alto"},
                {L"+ / -", L"Acercar / alejar"},
                {L"Ctrl+rueda", L"Zoom con la rueda"},
                {L"Espacio / Mayús+Espacio", L"Avanzar / retroceder una pantalla"},
                {L"Home / End", L"Principio / final del documento"},
                {L"Page Up / Page Down", L"Página anterior / siguiente"},
                {L"Flechas", L"Navegación continua"},
                {L"Alt+Izquierda", L"Volver al punto anterior"},
                {L"Tab / Mayús+Tab", L"Recorrer los controles de la barra"},
                {L"F6", L"Alternar entre documento, índice y barra"},
                {L"Escape", L"Cerrar la búsqueda o devolver el foco"},
                {L"Ctrl+D", L"Cambiar entre tema claro y oscuro"},
                {L"F1", L"Acerca de Chucao"},
            };
            for (size_t i = 0; i < sizeof(rows) / sizeof(rows[0]); ++i) {
                LVITEMW item{}; item.mask = LVIF_TEXT; item.iItem = static_cast<int>(i);
                item.pszText = const_cast<wchar_t*>(rows[i].keys);
                ListView_InsertItem(list, &item);
                ListView_SetItemText(list, static_cast<int>(i), 1, const_cast<wchar_t*>(rows[i].action));
            }
        }
        HWND ownerWindow = GetParent(dialog);
        RECT owner{}, bounds{};
        if (ownerWindow) GetWindowRect(ownerWindow, &owner); else GetWindowRect(dialog, &owner);
        GetWindowRect(dialog, &bounds);
        int dialogWidth = bounds.right - bounds.left, dialogHeight = bounds.bottom - bounds.top;
        int x = owner.left + ((owner.right - owner.left) - dialogWidth) / 2;
        int y = owner.top + ((owner.bottom - owner.top) - dialogHeight) / 2;
        // The hotkey table makes the dialog tall; keep it inside the monitor work
        // area even when the reader sits near the bottom or on a small display.
        MONITORINFO monitor{sizeof(monitor)};
        if (GetMonitorInfoW(MonitorFromWindow(ownerWindow ? ownerWindow : dialog, MONITOR_DEFAULTTONEAREST), &monitor)) {
            x = std::max<int>(monitor.rcWork.left, std::min<int>(x, monitor.rcWork.right - dialogWidth));
            y = std::max<int>(monitor.rcWork.top, std::min<int>(y, monitor.rcWork.bottom - dialogHeight));
        }
        SetWindowPos(dialog, nullptr, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
        // Start on informational text; Tab can still move to the repository link
        // and then to the hotkey table.
        SetFocus(GetDlgItem(dialog, IDC_ABOUT_TITLE));
        LITEM link{}; link.mask = LIF_ITEMINDEX | LIF_STATE; link.iLink = 0; link.stateMask = LIS_FOCUSED;
        SendDlgItemMessageW(dialog, IDC_ABOUT_REPOSITORY, LM_SETITEM, 0, reinterpret_cast<LPARAM>(&link));
        return FALSE;
    }
    if (message == WM_DESTROY) {
        if (HICON bird = reinterpret_cast<HICON>(SendDlgItemMessageW(dialog, IDC_ABOUT_ICON, STM_GETICON, 0, 0))) DestroyIcon(bird);
        return FALSE;
    }
    if (message == WM_COMMAND) {
        if (LOWORD(wp) == IDOK || LOWORD(wp) == IDCANCEL) { EndDialog(dialog, LOWORD(wp)); return TRUE; }
    }
    if (message == WM_NOTIFY) {
        auto notification = reinterpret_cast<NMHDR*>(lp);
        if (notification->idFrom == IDC_ABOUT_REPOSITORY && (notification->code == NM_CLICK || notification->code == NM_RETURN)) {
            if (reinterpret_cast<INT_PTR>(ShellExecuteW(dialog, L"open", CHUCAO_WIDE(CHUCAO_REPOSITORY), nullptr, nullptr, SW_SHOWNORMAL)) <= 32)
                MessageBoxW(dialog, L"No se pudo abrir el navegador para visitar el repositorio.", L"Chucao", MB_OK | MB_ICONINFORMATION);
            return TRUE;
        }
    }
    return FALSE;
}
} // namespace
