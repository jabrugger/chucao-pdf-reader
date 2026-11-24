#pragma once
// Shared constants and small Win32/text helpers.
// Internal fragment of main.cpp: include it only from there, after the Windows/PDFium headers.
namespace {
constexpr UINT Ready = WM_APP + 1;
constexpr UINT ChromeHover = WM_APP + 2;
constexpr UINT UpdateReady = WM_APP + 3;
constexpr UINT SettingsUpdateReady = WM_APP + 4;

constexpr int RecentBase = 4000;
constexpr int ZoomPresetBase = 4100;
constexpr int OutlineFilterId = 3100;
constexpr int HomeList = 4200;
enum { Open = 100, Previous, Next, Smaller, Larger, Fit, Find, Print, Query, FindNext, FindPrevious, Copy, Outline, OpenMenu, ForgetHistory,
    FindClose, OutlineClose, OutlinePin, OutlineRail, About, Theme, NewTabCmd, CloseTabCmd, NextTabCmd, PrevTabCmd,
    FitPageCmd, FitHeightCmd, PageEdit, OutlineFilterClear };

// Best-effort dark title bar. Older Windows builds ignore both attributes.
void ApplyWindowTheme(HWND window, bool dark) {
    // The frame change must happen before the color attributes; otherwise DWM
    // repaints the native accent title bar and discards the custom colors.
    if (window) SetWindowPos(window, nullptr, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
    BOOL value = dark ? TRUE : FALSE;
    DwmSetWindowAttribute(window, 20, &value, sizeof(value));
    DwmSetWindowAttribute(window, 19, &value, sizeof(value));
    // Windows 11 caption colors. In light/high-contrast mode COLOR_DEFAULT lets
    // Windows draw its native title bar again.
    DWORD caption = dark ? UiTheme().bar : 0xFFFFFFFF;
    DWORD captionText = dark ? UiTheme().ink : 0xFFFFFFFF;
    DWORD border = dark ? UiTheme().line : 0xFFFFFFFF;
    DwmSetWindowAttribute(window, 35, &caption, sizeof(caption));      // DWMWA_CAPTION_COLOR
    DwmSetWindowAttribute(window, 36, &captionText, sizeof(captionText)); // DWMWA_TEXT_COLOR
    DwmSetWindowAttribute(window, 34, &border, sizeof(border));        // DWMWA_BORDER_COLOR
}

// Owner-drawn buttons paint their own chrome. A theme change (including the
// parent's SetWindowTheme) can make Windows draw a native hot border again;
// keep every ownerdraw child on the empty theme after each palette update.
// Decode the transparent welcome logo from the module's RCDATA resource. The
// asset keeps its alpha channel, unlike the legacy BMP.
Gdiplus::Bitmap* LoadPngResource(UINT id) {
    HRSRC resource = FindResourceW(GetModuleHandleW(nullptr), MAKEINTRESOURCEW(id), RT_RCDATA);
    if (!resource) return nullptr;
    HGLOBAL loaded = LoadResource(GetModuleHandleW(nullptr), resource);
    DWORD size = SizeofResource(GetModuleHandleW(nullptr), resource);
    void* data = loaded ? LockResource(loaded) : nullptr;
    if (!data || !size) return nullptr;
    HGLOBAL memory = GlobalAlloc(GMEM_MOVEABLE, size);
    if (!memory) return nullptr;
    void* destination = GlobalLock(memory);
    if (!destination) { GlobalFree(memory); return nullptr; }
    memcpy(destination, data, size);
    GlobalUnlock(memory);
    IStream* stream = nullptr;
    if (FAILED(CreateStreamOnHGlobal(memory, TRUE, &stream))) { GlobalFree(memory); return nullptr; }
    Gdiplus::Bitmap* bitmap = Gdiplus::Bitmap::FromStream(stream);
    stream->Release();
    if (bitmap && bitmap->GetLastStatus() == Gdiplus::Ok) return bitmap;
    delete bitmap;
    return nullptr;
}

void DisableOwnerDrawThemes(HWND root) {
    if (!root) return;
    EnumChildWindows(root, [](HWND child, LPARAM) -> BOOL {
        LONG type = static_cast<LONG>(GetWindowLongPtrW(child, GWL_STYLE)) & BS_TYPEMASK;
        if (type == BS_OWNERDRAW) {
            SetWindowTheme(child, L"", L"");
            InvalidateRect(child, nullptr, FALSE);
        }
        return TRUE;
    }, 0);
}

// Per-monitor v2 when available, then per-monitor v1, then system DPI awareness.
void EnablePerMonitorDpi() {
    auto user32 = GetModuleHandleW(L"user32.dll");
    // Avoid depending on the DPI_AWARENESS_CONTEXT typedef so older SDKs still build.
    using SetContextFn = BOOL(WINAPI*)(HANDLE);
    auto setContext = user32 ? reinterpret_cast<SetContextFn>(GetProcAddress(user32, "SetProcessDpiAwarenessContext")) : nullptr;
    constexpr LONG_PTR PerMonitorAwareV2 = -4;
    if (setContext && setContext(reinterpret_cast<HANDLE>(PerMonitorAwareV2))) return;
    using SetAwarenessFn = HRESULT(WINAPI*)(int);
    if (auto shcore = LoadLibraryW(L"shcore.dll")) {
        auto setAwareness = reinterpret_cast<SetAwarenessFn>(GetProcAddress(shcore, "SetProcessDpiAwareness"));
        if (setAwareness && SUCCEEDED(setAwareness(2))) return;
        FreeLibrary(shcore);
    }
    SetProcessDPIAware();
}

// Screen readers use MSAA names. Owner-drawn buttons already expose their
// window text; edits need an explicit property because the cue banner is not a
// label. Failures are silent: the reader keeps working without a screen reader.
void SetAccessibleName(HWND control, const wchar_t* name) {
    if (!control) return;
    IAccPropServices* service = nullptr;
    if (SUCCEEDED(CoCreateInstance(CLSID_AccPropServices, nullptr, CLSCTX_INPROC_SERVER,
        IID_IAccPropServices, reinterpret_cast<void**>(&service)))) {
        service->SetHwndPropStr(control, static_cast<DWORD>(OBJID_CLIENT), CHILDID_SELF, PROPID_ACC_NAME, name);
        service->Release();
    }
}

int SystemDpi() {
    using GetDpiFn = UINT(WINAPI*)();
    static auto getDpi = reinterpret_cast<GetDpiFn>(GetProcAddress(GetModuleHandleW(L"user32.dll"), "GetDpiForSystem"));
    UINT value = getDpi ? getDpi() : 0;
    if (value < 48 || value > 768) {
        HDC dc = GetDC(nullptr);
        value = dc ? static_cast<UINT>(GetDeviceCaps(dc, LOGPIXELSX)) : 96;
        if (dc) ReleaseDC(nullptr, dc);
    }
    return value >= 48 && value <= 768 ? static_cast<int>(value) : 96;
}

int WindowDpi(HWND window) {
    using GetDpiFn = UINT(WINAPI*)(HWND);
    static auto getDpi = reinterpret_cast<GetDpiFn>(GetProcAddress(GetModuleHandleW(L"user32.dll"), "GetDpiForWindow"));
    UINT value = getDpi && window ? getDpi(window) : 0;
    if (value < 48 || value > 768) {
        HDC dc = window ? GetDC(window) : nullptr;
        value = dc ? static_cast<UINT>(GetDeviceCaps(dc, LOGPIXELSX)) : 96;
        if (dc) ReleaseDC(window, dc);
    }
    return value >= 48 && value <= 768 ? static_cast<int>(value) : 96;
}

#pragma comment(lib, "advapi32.lib")
// Windows "app mode": 0 = dark, 1 = light. Missing or unreadable means light.
wchar_t FoldChar(wchar_t c) {
    switch (c) {
    case L'à': case L'á': case L'â': case L'ã': case L'ä': case L'å': return L'a';
    case L'ç': return L'c';
    case L'è': case L'é': case L'ê': case L'ë': return L'e';
    case L'ì': case L'í': case L'î': case L'ï': return L'i';
    case L'ñ': return L'n';
    case L'ò': case L'ó': case L'ô': case L'õ': case L'ö': return L'o';
    case L'ù': case L'ú': case L'û': case L'ü': return L'u';
    case L'ý': case L'ÿ': return L'y';
    default: return c;
    }
}
// Case- and accent-insensitive folding for the outline filter.
std::wstring FoldText(const std::wstring& text) {
    if (text.empty()) return {};
    std::wstring lower(text.size(), 0);
    int written = LCMapStringW(LOCALE_INVARIANT, LCMAP_LOWERCASE, text.data(), static_cast<int>(text.size()),
        &lower[0], static_cast<int>(lower.size()));
    if (written <= 0) lower = text;
    std::wstring result; result.reserve(lower.size());
    for (wchar_t c : lower) result.push_back(FoldChar(c));
    return result;
}

std::wstring ExecutablePath() {
    wchar_t buffer[32768]{};
    DWORD length = GetModuleFileNameW(nullptr, buffer, 32768);
    return length > 0 && length < 32768 ? std::wstring(buffer, length) : std::wstring{};
}

bool SystemPrefersDark() {
    DWORD value = 1, size = sizeof(value);
    if (RegGetValueW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",
        L"AppsUseLightTheme", RRF_RT_REG_DWORD, nullptr, &value, &size) != ERROR_SUCCESS) return false;
    return value == 0;
}

void Highlight(HDC dc, int x, int y, int width, int height, const std::vector<TextBox>& boxes, COLORREF color) {
    if (boxes.empty()) return;
    HDC tint = CreateCompatibleDC(dc); HBITMAP pixel = CreateCompatibleBitmap(dc, 1, 1);
    if (!tint || !pixel) { if (tint) DeleteDC(tint); if (pixel) DeleteObject(pixel); return; }
    auto old = SelectObject(tint, pixel); SetPixel(tint, 0, 0, color);
    BLENDFUNCTION blend{AC_SRC_OVER, 0, 85, 0};
    for (const auto& box : boxes) {
        if (!box.Valid()) continue;
        int left = x + static_cast<int>(box.left * width), top = y + static_cast<int>(box.top * height);
        int right = x + static_cast<int>(std::ceil(box.right * width)), bottom = y + static_cast<int>(std::ceil(box.bottom * height));
        AlphaBlend(dc, left, top, std::max(1, right - left), std::max(1, bottom - top), tint, 0, 0, 1, 1, blend);
    }
    SelectObject(tint, old); DeleteObject(pixel); DeleteDC(tint);
}

std::string Utf8(const std::wstring& value) {
    int size = WideCharToMultiByte(CP_UTF8, 0, value.data(), static_cast<int>(value.size()), nullptr, 0, nullptr, nullptr);
    std::string result(size, 0);
    if (size) WideCharToMultiByte(CP_UTF8, 0, value.data(), static_cast<int>(value.size()), &result[0], size, nullptr, nullptr);
    return result;
}
template<class Char> struct EraseSecretOnExit {
    std::basic_string<Char>& value;
    ~EraseSecretOnExit() { if (!value.empty()) SecureZeroMemory(&value[0], value.size() * sizeof(Char)); value.clear(); }
};
std::wstring Text(HWND window) {
    std::wstring value(GetWindowTextLengthW(window) + 1, 0);
    GetWindowTextW(window, &value[0], static_cast<int>(value.size()));
    value.resize(wcslen(value.c_str()));
    return value;
}
// Owner-drawn checkboxes do not keep the native BM_SETCHECK state; store it per
// control so drawing, settings and the dialog tests agree.
bool SettingChecked(HWND dialog, int id) {
    HWND button = GetDlgItem(dialog, id);
    return button && GetWindowLongPtrW(button, GWLP_USERDATA) != 0;
}
void SetSettingChecked(HWND dialog, int id, bool checked) {
    if (HWND button = GetDlgItem(dialog, id)) {
        SetWindowLongPtrW(button, GWLP_USERDATA, checked ? 1 : 0);
        InvalidateRect(button, nullptr, FALSE);
    }
}
} // namespace
