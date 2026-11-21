#pragma once
// Modal password prompt for protected PDFs.
// Internal fragment of main.cpp: include it only from there, after the Windows/PDFium headers.
namespace {
struct PasswordState { HWND edit = nullptr; bool done = false, accepted = false; std::wstring value; };
struct PasswordScript {
    enum Action { Submit, Cancel, Escape, Close } action = Submit;
    std::wstring value;
    bool expectIncorrect = false, verified = false;
};
LRESULT CALLBACK PasswordProc(HWND window, UINT msg, WPARAM wp, LPARAM lp) {
    auto state = reinterpret_cast<PasswordState*>(GetWindowLongPtrW(window, GWLP_USERDATA));
    if (msg == WM_NCCREATE) {
        state = static_cast<PasswordState*>(reinterpret_cast<CREATESTRUCTW*>(lp)->lpCreateParams);
        SetWindowLongPtrW(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(state));
    }
    if (msg == WM_COMMAND && (LOWORD(wp) == IDOK || LOWORD(wp) == IDCANCEL)) {
        state->accepted = LOWORD(wp) == IDOK;
        if (state->accepted) state->value = Text(state->edit);
        state->done = true; return 0;
    }
    if (msg == WM_KEYDOWN && (wp == VK_RETURN || wp == VK_ESCAPE)) {
        SendMessageW(window, WM_COMMAND, wp == VK_RETURN ? IDOK : IDCANCEL, 0); return 0;
    }
    if (msg == WM_CLOSE) { state->done = true; return 0; }
    return DefWindowProcW(window, msg, wp, lp);
}
bool PasswordDialog(HWND owner, std::wstring& password, const std::wstring& prompt, HFONT font, PasswordScript* script = nullptr) {
    PasswordState state;
    RECT bounds{}; GetWindowRect(owner, &bounds);
    int dpi = WindowDpi(owner);
    auto d = [&](int value) { return MulDiv(value, dpi, 96); };
    HWND dialog = CreateWindowExW(WS_EX_DLGMODALFRAME, L"ChucaoPassword", L"Contraseña del PDF", WS_CAPTION | WS_SYSMENU | WS_POPUP,
        bounds.left + d(100), bounds.top + d(120), d(410), d(210), owner, nullptr, GetModuleHandleW(nullptr), &state);
    if (!dialog) return false;
    HWND label = CreateWindowW(L"STATIC", prompt.c_str(), WS_CHILD | WS_VISIBLE, d(18), d(14), d(360), d(42), dialog, nullptr, nullptr, nullptr);
    state.edit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_PASSWORD | ES_AUTOHSCROLL, d(18), d(64), d(360), d(28), dialog, nullptr, nullptr, nullptr);
    SendMessageW(state.edit, EM_SETLIMITTEXT, 256, 0);
    CreateWindowW(L"BUTTON", L"Abrir", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_DEFPUSHBUTTON, d(198), d(112), d(85), d(30), dialog, reinterpret_cast<HMENU>(IDOK), nullptr, nullptr);
    CreateWindowW(L"BUTTON", L"Cancelar", WS_CHILD | WS_VISIBLE | WS_TABSTOP, d(293), d(112), d(85), d(30), dialog, reinterpret_cast<HMENU>(IDCANCEL), nullptr, nullptr);
    for (HWND child = GetWindow(dialog, GW_CHILD); child; child = GetWindow(child, GW_HWNDNEXT)) SendMessageW(child, WM_SETFONT, reinterpret_cast<WPARAM>(font), FALSE);
    bool wasEnabled = IsWindowEnabled(owner) != 0;
    if (!script) { EnableWindow(owner, FALSE); ShowWindow(dialog, SW_SHOW); SetFocus(state.edit); }
    else {
        SetWindowTextW(state.edit, script->value.c_str());
        script->verified = (GetWindowLongPtrW(state.edit, GWL_STYLE) & ES_PASSWORD) && Text(label) == prompt &&
            ((prompt.find(L"incorrecta") != std::wstring::npos) == script->expectIncorrect);
        if (script->action == PasswordScript::Submit) SendMessageW(dialog, WM_KEYDOWN, VK_RETURN, 0);
        else if (script->action == PasswordScript::Escape) SendMessageW(dialog, WM_KEYDOWN, VK_ESCAPE, 0);
        else if (script->action == PasswordScript::Close) SendMessageW(dialog, WM_CLOSE, 0, 0);
        else SendMessageW(dialog, WM_COMMAND, IDCANCEL, 0);
    }
    MSG message{};
    while (!state.done) {
        int status = GetMessageW(&message, nullptr, 0, 0);
        if (status <= 0) { if (!status) PostQuitMessage(static_cast<int>(message.wParam)); break; }
        if (message.message == WM_KEYDOWN && message.wParam == VK_RETURN) SendMessageW(dialog, WM_COMMAND, IDOK, 0);
        else if (message.message == WM_KEYDOWN && message.wParam == VK_ESCAPE) SendMessageW(dialog, WM_COMMAND, IDCANCEL, 0);
        else if (!IsDialogMessageW(dialog, &message)) { TranslateMessage(&message); DispatchMessageW(&message); }
    }
    password = std::move(state.value);
    SetWindowTextW(state.edit, L""); DestroyWindow(dialog);
    if (!script && IsWindow(owner)) { if (wasEnabled) EnableWindow(owner, TRUE); SetActiveWindow(owner); }
    return state.accepted;
}

} // namespace
