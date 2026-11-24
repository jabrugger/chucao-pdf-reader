#pragma once
// The Reader class: window state, tabs, rendering, input and chrome.
// Internal fragment of main.cpp: include it only from there, after the Windows/PDFium headers.
namespace {
LRESULT CALLBACK OutlineProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK IconButtonProc(HWND, UINT, WPARAM, LPARAM, UINT_PTR, DWORD_PTR);
LRESULT CALLBACK HomeListProc(HWND, UINT, WPARAM, LPARAM, UINT_PTR, DWORD_PTR);
LRESULT CALLBACK FlatButtonProc(HWND, UINT, WPARAM, LPARAM, UINT_PTR, DWORD_PTR);
INT_PTR CALLBACK SettingsProc(HWND, UINT, WPARAM, LPARAM);

class Reader {
public:
    HWND window = nullptr, query = nullptr, outlineWindow = nullptr, outlineTree = nullptr, outlineLabel = nullptr;
    HWND tooltip = nullptr, findLabel = nullptr, findFeedback = nullptr, pageEdit = nullptr;
    bool searchVisible = false, outlinePinned = false, outlinePeek = false, outlineAutoShow = true, arranging = false;
    bool interfaceTest = false;
    int interfaceStep = 0;
    bool pageEditActive = false;
    int previousContentWidth = 0;
    int dpi = 96;
    int D(int value) const { return MulDiv(value, dpi, 96); }
    int BarPixels() const { return D(52); }
    int PageGap() const { return D(DocumentLayout::Gap); }
    int TabBarHeight() const { return tabs.empty() ? 0 : D(34); }
    int ContentTop() const { return BarPixels() + TabBarHeight(); }
    std::wstring TabKey(const std::wstring& file) const { return ReadingStateStore::NormalizePath(file); }
    bool SameTab(const std::wstring& a, const std::wstring& b) const {
        auto na = TabKey(a), nb = TabKey(b);
        return !na.empty() && na == nb;
    }
    std::wstring TabTitle(const std::wstring& file) const {
        auto slash = file.find_last_of(L"\\/");
        return slash == std::wstring::npos ? file : file.substr(slash + 1);
    }
    DocumentOutline outline;
    bool outlineLoaded = false, outlinePending = false, outlineTest = false;
    std::vector<HTREEITEM> outlineItems;   // tree item of each outline entry
    HTREEITEM outlineHighlight = nullptr;  // row shaded for the section being read
    int outlineChecks = 0, outlineStep = 0;
    HWND outlineFilter = nullptr;
    std::wstring outlineFilterQuery;
    bool outlineFilterTest = false;
    int outlineFilterChecks = 0;
    bool destinationsTest = false;
    double testedDestinationZoom = 3;
    bool linksTest = false;
    int linkChecks = 0, linkStep = 0;
    int webOpenAttempts = 0;
    std::int64_t linkTestScroll = 0;
    int linkTestHorizontal = 0;
    bool passwordTest = false, canCopy = false, canPrint = false;
    int passwordStage = 0, passwordAttempts = 0, passwordChecks = 0;
    std::wstring passwordTestRoot;
    HFONT font = nullptr;
    HFONT strongFont = nullptr;
    HFONT welcomeFont = nullptr;
    HBITMAP welcomeImage = nullptr;
    Gdiplus::Bitmap* welcomeLogo = nullptr;
    HDC backDC = nullptr;
    HBITMAP backBitmap = nullptr;
    int backWidth = 0, backHeight = 0;

    bool readingStateEnabled = true, readingStateWrites = true, stateConfigured = false, viewReady = false;
    std::wstring readingStateFile;
    bool settingsEnabled = true, settingsConfigured = false, forceDarkTheme = false;
    bool forceHighContrast = false, darkTheme = false, applyingPalette = false, paletteReady = false;
bool followSystemTheme = false, autoShowOutline = true, checkUpdates = false;
    std::wstring settingsFile;
    bool updatesTest = false;
    std::wstring updatesTestFile;
    std::optional<ReadingState> pendingReadingState;
    int readingStateTest = 0, readingStateStep = 0, readingStateChecks = 0;
    bool readingStateFound = false;
    int stateAuthenticationAttempts = 0;
    bool savedOnClose = false;
    bool suppressCurrentState = false;
    std::vector<ReadingState> recentEntries;
    HWND homeList = nullptr;
    HWND statusBar = nullptr;
    std::wstring announcedStatus;
    bool restoreLastSession = false, sessionWrites = true;
    int sessionTest = 0, sessionStep = 0, sessionChecks = 0;
    std::wstring sessionTestA, sessionTestB;
    bool recentTest = false;
    bool themeTest = false;
    bool startupTest = false;
    bool dpiTest = false;
    bool keysTest = false;
    bool hcTest = false;
    bool rapidTest = false;
    int rapidPages = 0;
    bool fitTest = false;
    int fitStep = 0;
    bool tabsTest = false;
    bool windowTest = false;
    bool tabsTestFailed = false;
    std::wstring tabsTestA, tabsTestB, tabsTestProtected, tabsTestMissing;
    int tabsStep = 0, tabsChecks = 0, tabsPrompts = 0, tabsClosed = 0;
    ReadingState tabsExpectedA, tabsExpectedB;
    bool openingDocument = false;
    bool loadingFailed = false, loadingTimer = false;
    int spinAngle = 0;
    int startupStage = 0, startupPage = -1;
    double startupRenderMs = 0, startupBitmapMs = 0, startupComposeMs = 0;
    int recentStep = 0, recentChecks = 0;
    std::wstring recentOther, recentMissing;
    std::unique_ptr<PdfWorker> worker;
    struct CachedPage { std::unique_ptr<Result> image; unsigned long long version = 0, used = 0; };
    std::map<int, CachedPage> cache;
    DocumentLayout layout;
    std::vector<int> planned;
    unsigned long long viewVersion = 0, plannedVersion = 0, useClock = 0;
    int renderWidth = 0, renderHeight = 0;
    double renderZoom = -1;
    ZoomMode renderMode = ZoomMode::FitWidth;
    size_t cacheBytes = 0;
    static constexpr size_t CacheLimit = 96u * 1024u * 1024u;
    unsigned long long document = 0, revision = 0;
    int page = 0, pages = 0;
    std::int64_t scroll = 0;
    int horizontal = 0;
    double zoom = 0;
    ZoomMode zoomMode = ZoomMode::FitWidth;
    bool searching = false, printing = false;
    struct TextPoint { int page = 0; double x = 0, y = 0; };
    struct Endpoint { int page = 0, character = -1; };
    std::map<int, std::vector<TextBox>> textPages;
    std::set<int> pendingText;
    std::map<int, std::vector<PageLink>> pageLinks;
    std::set<int> pendingLinks;
    struct ReadingPosition { int page; double fraction, horizontal; double zoom; };
    std::vector<ReadingPosition> linkHistory;
    unsigned long long linkEpoch = 0;
    TextPoint selectionStart, selectionEnd;
    POINT dragStart{};
    bool dragging = false, selected = false, copyWhenReady = false;
    unsigned long long selectionRevision = 0;
    int searchPage = -1;
    int searchMatch = -1;
    int testWordStart[2]{0, 0};
    std::vector<TextBox> searchBoxes;
    std::wstring path, status = L"Abrí un PDF para empezar · Ctrl+O";
    struct DocumentTab { std::wstring path; std::optional<ReadingState> reading; };
    std::vector<DocumentTab> tabs;
    int activeTab = -1, hoverTab = -1;
    bool hoverTabClose = false, homeActive = false, hoverHomeTab = false;
    struct TabLayout { RECT bounds{}, close{}; };
    std::vector<TabLayout> tabLayout;
    RECT homeTabBounds{};
    std::wstring benchmarkReport;
    bool navigationTest = false;
    bool textTest = false;
    int textStep = 0, textChecks = 0;
    std::wstring crossPageExpected;
    int navigationStep = 0;
    size_t navigationHits = 0;
    std::chrono::steady_clock::time_point started;
    double shellMs = 0, openedMs = 0;
    int OutlineWidth() const { RECT rc{}; GetClientRect(window, &rc); return std::min(D(280), std::max(D(220), static_cast<int>(rc.right / 4))); }
    int ViewLeft() const { return outlinePinned ? OutlineWidth() : outlineLoaded && !outline.entries.empty() ? D(18) : 0; }
    HWND IconButton(HWND parent, int id, const wchar_t* title) {
        HWND button = CreateWindowW(L"BUTTON", title, WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_OWNERDRAW | BS_FLAT,
            0, 0, D(36), D(36), parent, reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)), nullptr, nullptr);
        // The native button theme would leave a light frame around the custom
        // painting in the dark theme. The button draws its own background.
        SetWindowTheme(button, L"", L"");
        SetWindowSubclass(button, IconButtonProc, 1, 0);
        TOOLINFOW tip{sizeof(tip)}; tip.uFlags = TTF_IDISHWND | TTF_SUBCLASS;
        tip.hwnd = parent; tip.uId = reinterpret_cast<UINT_PTR>(button); tip.lpszText = const_cast<wchar_t*>(title);
        SendMessageW(tooltip, TTM_ADDTOOLW, 0, reinterpret_cast<LPARAM>(&tip));
        return button;
    }
    void PaintButton(const DRAWITEMSTRUCT& item) {
        if (item.CtlID == HomeList) {
            auto paintItem = [&](HDC dc, RECT bounds) {
                SetDCBrushColor(dc, UiTheme().bar); FillRect(dc, &bounds, reinterpret_cast<HBRUSH>(GetStockObject(DC_BRUSH)));
                if (item.itemID >= recentEntries.size()) return;
                RECT row = bounds; InflateRect(&row, -D(2), -D(2));
                if (item.itemState & ODS_SELECTED) UiRoundRect(dc, row, UiTheme().hover, UiTheme().hover, D(10));
                const auto& file = recentEntries[item.itemID].path;
                auto slash = file.find_last_of(L"\\/");
                auto name = file.substr(slash == std::wstring::npos ? 0 : slash + 1);
                auto folder = slash == std::wstring::npos ? L"" : file.substr(0, slash);
                int saved = SaveDC(dc); SetBkMode(dc, TRANSPARENT);
                RECT title{row.left + D(16), row.top + D(6), row.right - D(16), row.top + D(27)};
                SelectObject(dc, strongFont); SetTextColor(dc, UiTheme().ink);
                DrawTextW(dc, name.c_str(), -1, &title, DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS | DT_NOPREFIX);
                RECT pathBox{title.left, row.top + D(27), title.right, row.bottom - D(4)};
                SelectObject(dc, font); SetTextColor(dc, UiTheme().muted);
                DrawTextW(dc, folder.c_str(), -1, &pathBox, DT_SINGLELINE | DT_VCENTER | DT_PATH_ELLIPSIS | DT_NOPREFIX);
                if ((item.itemState & (ODS_FOCUS | ODS_NOFOCUSRECT)) == ODS_FOCUS) DrawFocusRect(dc, &row);
                RestoreDC(dc, saved);
            };
            // Owner-drawn list rows are composed in a memory DC and presented in
            // one blit, so the list background never flashes between rows.
            int width = std::max(1L, item.rcItem.right - item.rcItem.left);
            int height = std::max(1L, item.rcItem.bottom - item.rcItem.top);
            HDC memory = CreateCompatibleDC(item.hDC);
            HBITMAP bitmap = memory ? CreateCompatibleBitmap(item.hDC, width, height) : nullptr;
            if (memory && bitmap) {
                HGDIOBJ old = SelectObject(memory, bitmap);
                paintItem(memory, {0, 0, width, height});
                BitBlt(item.hDC, item.rcItem.left, item.rcItem.top, width, height, memory, 0, 0, SRCCOPY);
                SelectObject(memory, old); DeleteObject(bitmap); DeleteDC(memory);
            } else {
                if (bitmap) DeleteObject(bitmap);
                if (memory) DeleteDC(memory);
                paintItem(item.hDC, item.rcItem);
            }
            return;
        }
        auto px = [&](int value) { return D(value); };
        UiIcon icon = UiIcon::Folder; bool active = false;
        switch (item.CtlID) {
        case Previous: case FindPrevious: case OutlineClose: icon = UiIcon::Previous; break;
        case Next: case FindNext: case OutlineRail: icon = UiIcon::Next; break;
        case Smaller: icon = UiIcon::Minus; break;
        case Larger: icon = UiIcon::Plus; break;
        case Fit: icon = UiIcon::Fit; break;
        case Outline: icon = UiIcon::Outline; active = outlinePinned; break;
        case Find: icon = UiIcon::Search; active = searchVisible; break;
        case Print: icon = UiIcon::Printer; break;
        case FindClose: icon = UiIcon::Close; break;
        case OutlinePin: icon = UiIcon::Pin; active = outlinePinned; break;
        case Theme: icon = UiIcon::Gear; break;
        case About: icon = UiIcon::Info; break;
        }
        POINT mouse{}; GetCursorPos(&mouse); RECT rect{}; GetWindowRect(item.hwndItem, &rect);
        DrawUiIcon(item.hDC, item.rcItem, icon, active, PtInRect(&rect, mouse) != FALSE,
            (item.itemState & ODS_SELECTED) != 0,
            (item.itemState & (ODS_FOCUS | ODS_NOFOCUSRECT)) == ODS_FOCUS);
    }
    void ArrangeChrome() {
        if (arranging) return;
        arranging = true;
        RECT rc{}; GetClientRect(window, &rc);
        HDWP hdwp = BeginDeferWindowPos(20);
        auto place = [&](HWND target, int x, int y, int width, int height) {
            if (!target) return;
            if (hdwp) {
                HDWP next = DeferWindowPos(hdwp, target, nullptr, x, y, width, height,
                    SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOREDRAW | SWP_NOCOPYBITS);
                if (next) { hdwp = next; return; }
                hdwp = nullptr;
            }
            MoveWindow(target, x, y, width, height, FALSE);
        };
        struct ToolbarItem { int id, x, width; };
        const ToolbarItem toolbar[] = {{OpenMenu, 16, 36}, {Outline, 76, 36}, {Find, 116, 36},
            {Previous, 184, 36}, {Next, 332, 36}, {Smaller, 400, 36}, {Larger, 508, 36},
            {Fit, 548, 36}, {Print, 612, 36}, {Theme, 664, 36}};
        for (const auto& item : toolbar)
            place(GetDlgItem(window, item.id), D(item.x), D(8), D(item.width), D(36));
        place(GetDlgItem(window, About), rc.right - D(54), D(8), D(36), D(36));
        RECT pageField = PageFieldRect();
        place(pageEdit, pageField.left, pageField.top, pageField.right - pageField.left, pageField.bottom - pageField.top);
        int left = ViewLeft(), bottom = static_cast<int>(rc.bottom) - D(26);
        int panelBottom = outlinePinned ? bottom : BarPixels() + ViewHeight();
        place(outlineWindow, 0, ContentTop(), OutlineWidth(), std::max(1, panelBottom - ContentTop()));
        HWND rail = GetDlgItem(window, OutlineRail);
        place(rail, 0, ContentTop(), D(18), ViewHeight());
        int y = bottom - D(44), width = std::max(D(120), std::min(D(320), static_cast<int>(rc.right) - left - D(320)));
        place(query, left + D(50), y + D(12), width, D(22));
        int x = left + D(78) + width;
        for (int id : {FindPrevious, FindNext, FindClose}) { place(GetDlgItem(window, id), x, y + D(4), D(36), D(36)); x += D(36); }
        place(findFeedback, x + D(8), y + D(13), std::max(1, static_cast<int>(rc.right) - x - D(16)), D(22));
        place(statusBar, D(16), bottom + D(2), std::max(1, static_cast<int>(rc.right) - D(32)), D(22));
        if (hdwp) EndDeferWindowPos(hdwp);
        ShowWindow(outlineWindow, outlinePinned || outlinePeek ? SW_SHOWNA : SW_HIDE);
        ShowWindow(rail, !outlinePinned && outlineLoaded && !outline.entries.empty() && !outlinePeek ? SW_SHOWNA : SW_HIDE);
        if (outlinePeek) SetWindowPos(outlineWindow, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
        for (HWND control : {query, findLabel, findFeedback, GetDlgItem(window, FindPrevious), GetDlgItem(window, FindNext), GetDlgItem(window, FindClose)})
            ShowWindow(control, searchVisible ? SW_SHOWNA : SW_HIDE);
        ShowWindow(findLabel, SW_HIDE);
        Reflow(true, previousContentWidth); previousContentWidth = ViewWidth();
        InvalidateRect(GetDlgItem(window, Outline), nullptr, FALSE);
        InvalidateRect(GetDlgItem(window, Find), nullptr, FALSE);
        ArrangeHome();
        RedrawWindow(window, nullptr, nullptr, RDW_INVALIDATE | RDW_ALLCHILDREN);
        arranging = false;
    }
    void ShowSearch(bool focus = true) {
        if (!searchVisible) { searchVisible = true; ArrangeChrome(); Render(); }
        if (focus) { SetFocus(query); SendMessageW(query, EM_SETSEL, 0, -1); }
    }
    void HideSearch() {
        if (!searchVisible) return;
        searchVisible = false; searching = false; searchPage = -1; searchBoxes.clear();
        KillTimer(window, 9);
        if (worker) worker->CancelSearch();
        SetFocus(window); PageStatus(); ArrangeChrome(); Render();
    }
    void SetOutlinePinned(bool value) {
        outlinePinned = value; outlinePeek = false; outlineAutoShow = false; KillTimer(window, 7);
        ArrangeChrome(); Render();
    }
    bool UsingHighContrast() const { return forceHighContrast || HighContrastActive(); }
    void StyleOutline() {
        if (!outlineTree) return;
        SendMessageW(outlineTree, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);
        if (outlineLabel) SendMessageW(outlineLabel, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);
        if (outlineFilter) SendMessageW(outlineFilter, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);
        TreeView_SetBkColor(outlineTree, UiTheme().bar);
        TreeView_SetTextColor(outlineTree, UiTheme().ink);
        TreeView_SetExtendedStyle(outlineTree, TVS_EX_DOUBLEBUFFER, TVS_EX_DOUBLEBUFFER);
        SetWindowTheme(outlineTree, UsingHighContrast() ? L"" : (UiDark() ? L"DarkMode_Explorer" : L"Explorer"), nullptr);
        TreeView_SetItemHeight(outlineTree, D(32));
        TreeView_SetIndent(outlineTree, D(16));
    }
    void StyleNativeBars() {
        const wchar_t* theme = UsingHighContrast() ? L"" : (darkTheme ? L"DarkMode_Explorer" : L"Explorer");
        if (window) SetWindowTheme(window, theme, nullptr);
        if (homeList) SetWindowTheme(homeList, theme, nullptr);
        if (outlineTree) SetWindowTheme(outlineTree, theme, nullptr);
        // SetWindowTheme can reset the DWM caption colors; apply them last.
        if (window) ApplyWindowTheme(window, UsingHighContrast() ? false : darkTheme);
        if (outlineWindow) ApplyWindowTheme(outlineWindow, UsingHighContrast() ? false : darkTheme);
    }
    void ApplyFonts() {
        for (HWND control : {query, findLabel, findFeedback, statusBar, pageEdit})
            if (control) SendMessageW(control, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);
        StyleOutline();
    }
    void CreateFonts() {
        auto make = [&](int points, int weight) {
            return CreateFontW(-D(points), 0, 0, 0, weight, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
        };
        HFONT regular = make(14, FW_NORMAL), strong = make(14, FW_SEMIBOLD), welcome = make(28, FW_SEMIBOLD);
        if (!regular || !strong || !welcome) {
            if (regular) DeleteObject(regular);
            if (strong) DeleteObject(strong);
            if (welcome) DeleteObject(welcome);
            return;
        }
        HFONT oldRegular = font, oldStrong = strongFont, oldWelcome = welcomeFont;
        font = regular; strongFont = strong; welcomeFont = welcome;
        ApplyFonts();
        if (oldRegular) DeleteObject(oldRegular);
        if (oldStrong) DeleteObject(oldStrong);
        if (oldWelcome) DeleteObject(oldWelcome);
    }
    bool ConfigureSettings() {
        if (!settingsEnabled) return false;
        if (settingsConfigured) return !settingsFile.empty();
        settingsConfigured = true;
        if (!settingsFile.empty()) return true;
        PWSTR folder = nullptr;
        if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, nullptr, &folder))) {
            settingsFile = std::wstring(folder) + L"\\ChucaoPDFReader\\settings.dat"; CoTaskMemFree(folder);
        }
        return !settingsFile.empty();
    }
    AppSettings CollectSettings() const {
        AppSettings value{darkTheme, restoreLastSession, followSystemTheme, autoShowOutline};
        value.checkUpdates = checkUpdates;
        if (window) {
            WINDOWPLACEMENT placement{}; placement.length = sizeof(placement);
            if (GetWindowPlacement(window, &placement)) {
                value.hasWindow = true;
                value.windowRect = placement.rcNormalPosition;
                value.maximized = placement.showCmd == SW_SHOWMAXIMIZED || IsZoomed(window) != 0 ||
                    (placement.flags & WPF_RESTORETOMAXIMIZED) != 0;
            }
        }
        return value;
    }
    void SaveWindowPlacement() {
        if (!settingsEnabled || !ConfigureSettings()) return;
        SettingsStore(settingsFile).Write(CollectSettings());
    }
    bool PrepareWindowPlacement() {
        if (!settingsEnabled || !ConfigureSettings()) return false;
        auto settings = SettingsStore(settingsFile).Read();
        if (!settings || !settings->hasWindow) return false;
        RECT bounds = settings->windowRect;
        if (bounds.right - bounds.left < 200 || bounds.bottom - bounds.top < 150) return false;
        if (!MonitorFromRect(&bounds, MONITOR_DEFAULTTONULL)) return false;
        SetWindowPos(window, nullptr, bounds.left, bounds.top, bounds.right - bounds.left, bounds.bottom - bounds.top,
            SWP_NOZORDER | SWP_NOACTIVATE);
        return settings->maximized;
    }
    void ApplyPalette(bool dark, bool persist) {
        if (applyingPalette) return;
        bool highContrast = UsingHighContrast();
        const UiPalette* desired = highContrast ? &HighContrastPalette() : dark ? &DarkPalette() : &LightPalette();
        // Checkbox changes only update flags; they must not repaint the reader.
        if (paletteReady && CurrentPalette() == desired && darkTheme == dark) return;
        applyingPalette = true;
        if (window) SendMessageW(window, WM_SETREDRAW, FALSE, 0);
        if (outlineWindow) SendMessageW(outlineWindow, WM_SETREDRAW, FALSE, 0);
        darkTheme = dark;
        if (highContrast) CurrentPalette() = &HighContrastPalette(); else SetUiTheme(dark);
        if (window) {
            // A light/high-contrast window resets the DWM caption colors.
            ApplyWindowTheme(window, highContrast ? false : dark);
            SetWindowTheme(window, highContrast ? L"" : (dark ? L"DarkMode_Explorer" : L"Explorer"), nullptr);
        }
        if (outlineWindow) {
            ApplyWindowTheme(outlineWindow, highContrast ? false : dark);
            SetWindowTheme(outlineWindow, highContrast ? L"" : (dark ? L"DarkMode_Explorer" : L"Explorer"), nullptr);
        }
        DisableOwnerDrawThemes(window);
        if (outlineWindow) DisableOwnerDrawThemes(outlineWindow);
        if (outlineTree) StyleOutline();
        StyleNativeBars();
        if (persist && ConfigureSettings()) SettingsStore(settingsFile).Write(CollectSettings());
        if (window) SendMessageW(window, WM_SETREDRAW, TRUE, 0);
        if (outlineWindow) SendMessageW(outlineWindow, WM_SETREDRAW, TRUE, 0);
        paletteReady = true;
        if (!window) { applyingPalette = false; return; }
        // One final invalidation; no RDW_ERASE, which is what produced the flash.
        RedrawWindow(window, nullptr, nullptr, RDW_INVALIDATE | RDW_ALLCHILDREN);
        applyingPalette = false;
    }
    void LoadTheme() {
        restoreLastSession = false; followSystemTheme = false; autoShowOutline = outlineAutoShow = true; checkUpdates = false;
        if (forceHighContrast) { ApplyPalette(false, false); return; }
        if (forceDarkTheme) { ApplyPalette(true, false); return; }
        if (ConfigureSettings()) {
            if (auto settings = SettingsStore(settingsFile).Read()) {
                restoreLastSession = settings->restoreSession; followSystemTheme = settings->followSystem;
                autoShowOutline = outlineAutoShow = settings->autoShowOutline;
                checkUpdates = settings->checkUpdates;
                ApplyPalette(followSystemTheme ? SystemPrefersDark() : settings->dark, false); return;
            }
        }
        ApplyPalette(false, false);
    }
    // Ctrl+D is an explicit choice, so it stops following the system.
    void ToggleTheme() { followSystemTheme = false; ApplyPalette(!darkTheme, true); }
    // Run the opt-in release check on a detached thread and deliver the result
    // back through UpdateReady. The thread only touches the window handle and free
    // functions, so it stays safe if the window is destroyed while it runs.
    void StartUpdateCheck() {
        if (!checkUpdates || !IsWindow(window) || !benchmarkReport.empty()) return;
        HWND target = window;
        std::thread([target]() {
            UpdateInfo info = CheckForUpdate();
            auto payload = new UpdateInfo(std::move(info));
            if (!PostMessageW(target, UpdateReady, 0, reinterpret_cast<LPARAM>(payload))) delete payload;
        }).detach();
    }
    void OnUpdateChecked(UpdateInfo* payload) {
        std::unique_ptr<UpdateInfo> info(payload);
        if (!info || !info->checked || !info->newer || !IsWindow(window)) return;
        std::wstring message = L"Hay una nueva versión de Chucao disponible (" + info->latest +
            L").\n\n¿Abrir la página de descargas en el navegador?";
        if (MessageBoxW(window, message.c_str(), L"Chucao — actualización disponible",
            MB_YESNO | MB_ICONINFORMATION) == IDYES)
            OpenWebLink(info->url);
    }
    // Offline diagnostic: evaluate a local releases/latest payload with the same
    // parser and version comparison the live check uses, without any network.
    void RunUpdatesTest() {
        std::string body;
        HANDLE file = CreateFileW(updatesTestFile.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr,
            OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (file != INVALID_HANDLE_VALUE) {
            LARGE_INTEGER size{}; DWORD read = 0;
            if (GetFileSizeEx(file, &size) && size.QuadPart > 0 && size.QuadPart <= 65536) {
                body.resize(static_cast<size_t>(size.QuadPart));
                if (!ReadFile(file, &body[0], static_cast<DWORD>(body.size()), &read, nullptr)) body.clear();
                body.resize(read);
            }
            CloseHandle(file);
        }
        UpdateInfo info = FromReleaseJson(body);
        char data[512]{};
        int size = sprintf_s(data, "{\"success\":%s,\"newer\":%s,\"latest\":\"%ls\"}\n",
            info.checked ? "true" : "false", info.newer ? "true" : "false", info.latest.c_str());
        HANDLE output = CreateFileW(benchmarkReport.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (output != INVALID_HANDLE_VALUE) { DWORD written = 0; WriteFile(output, data, size, &written, nullptr); CloseHandle(output); }
        PostMessageW(window, WM_CLOSE, 0, 0);
    }
    std::wstring SessionFile() {
        if (!ConfigureSettings()) return {};
        return settingsFile.substr(0, settingsFile.find_last_of(L"\\/") + 1) + L"last-session.dat";
    }
    bool SaveSession() {
        if (!sessionWrites || !restoreLastSession || !settingsEnabled) return true;
        SavedSession saved;
        for (size_t i = 0; i < tabs.size() && i < 64; ++i) saved.paths.push_back(tabs[i].path);
        saved.active = std::max(0, activeTab);
        if (saved.active >= static_cast<int>(saved.paths.size()) && !saved.paths.empty()) {
            saved.paths.back() = tabs[activeTab].path; saved.active = static_cast<int>(saved.paths.size()) - 1;
        }
        return SessionStore(SessionFile()).Write(saved);
    }
    bool RestoreSession() {
        if (!settingsEnabled || !restoreLastSession || !tabs.empty()) return false;
        auto saved = SessionStore(SessionFile()).Read();
        if (saved.paths.empty()) return false;
        for (const auto& file : saved.paths) tabs.push_back({file, {}});
        activeTab = saved.active; Load(tabs[activeTab].path); return true;
    }
    void RefreshHomeRecents() {
        std::optional<ReadingState> current;
        if (pages && viewReady && !pendingReadingState && !suppressCurrentState) current = CurrentReadingState();
        recentEntries = ConfigureReadingState() ? RecentFiles(ReadingStateStore(readingStateFile).Read(), current) : std::vector<ReadingState>{};
        if (homeList) {
            SendMessageW(homeList, LB_RESETCONTENT, 0, 0);
            for (const auto& entry : recentEntries) SendMessageW(homeList, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(entry.path.c_str()));
        }
        ArrangeHome(); Redraw();
    }
    // The bird grows with the window but always leaves room for the heading and at
    // least three rows of recent files (up to 200 dip, never below the old 96).
    int HomeImageSize() const {
        RECT rc{}; GetClientRect(window, &rc);
        int reserved = D(16) + D(78) + D(56) * 3 + D(50);
        return std::clamp(static_cast<int>(rc.bottom) - ContentTop() - reserved, D(96), D(200));
    }
    int HomeListTop() const { return ContentTop() + D(16) + HomeImageSize() + D(78); }
    void ArrangeHome() {
        if (!homeList) return;
        RECT rc{}; GetClientRect(window, &rc);
        int width = std::min(D(660), std::max(D(100), static_cast<int>(rc.right) - D(48)));
        MoveWindow(homeList, (rc.right - width) / 2, HomeListTop(), width, std::max(D(56), static_cast<int>(rc.bottom) - D(50) - HomeListTop()), FALSE);
        SendMessageW(homeList, LB_SETITEMHEIGHT, 0, D(56)); SendMessageW(homeList, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);
        ShowWindow(homeList, !pages && !openingDocument && !recentEntries.empty() ? SW_SHOWNA : SW_HIDE);
    }
    void RunSessionTest() {
        if (sessionTest == 1) OpenDocument(sessionTestA);
        else if (sessionTest == 2) { if (!RestoreSession()) BenchmarkResult(false); }
        else {
            bool ok = !RestoreSession() && tabs.empty() && !worker;
            RefreshHomeRecents(); sessionChecks = 2;
            SaveTestImage(benchmarkReport + L".home.bmp", window);
            BenchmarkResult(ok && recentEntries.size() == 2);
        }
    }
    void CheckSessionImage() {
        if (!pages || !viewReady || pendingReadingState) return;
        if (sessionTest == 1) {
            if (sessionStep == 0) { sessionChecks = 1; sessionStep = 1; OpenDocument(sessionTestB); }
            else {
                bool ok = tabs.size() == 2 && activeTab == 1 && pages == 4 && SaveSession();
                auto saved = SessionStore(SessionFile()).Read(); sessionChecks = 3;
                BenchmarkResult(ok && saved.paths.size() == 2 && saved.active == 1);
            }
        } else if (sessionTest == 2) {
            bool ok = restoreLastSession && tabs.size() == 2 && activeTab == 1 && pages == 4;
            ++sessionChecks;
            while (!tabs.empty()) CloseTab(activeTab);
            RefreshHomeRecents();
            ok = ok && recentEntries.size() == 2 && SendMessageW(homeList, LB_GETCOUNT, 0, 0) == 2;
            ++sessionChecks; SaveTestImage(benchmarkReport + L".home.bmp", window);
            HWND options = CreateDialogParamW(GetModuleHandleW(nullptr), MAKEINTRESOURCEW(IDD_SETTINGS), window, SettingsProc, reinterpret_cast<LPARAM>(this));
            ok = ok && options && SettingChecked(options, IDC_SETTINGS_RESTORE);
            if (options) { SaveTestImage(benchmarkReport + L".settings.bmp", options); DestroyWindow(options); }
            ++sessionChecks; BenchmarkResult(ok);
        }
    }
    void RunHcTest() {
        forceHighContrast = true;
        ApplyPalette(false, false);
        const auto& palette = UiTheme();
        bool ok = palette.bar == GetSysColor(COLOR_WINDOW) && palette.ink == GetSysColor(COLOR_WINDOWTEXT) &&
            palette.accent == GetSysColor(COLOR_HIGHLIGHT) && palette.canvas == GetSysColor(COLOR_WINDOW) &&
            palette.sheet == GetSysColor(COLOR_WINDOW) && !UiDark();
        SaveTestImage(benchmarkReport + L".hc.bmp");
        BenchmarkResult(ok);
    }
    void CycleFocus() {
        if (GetFocus() == window) {
            if (outlinePinned && outlineTree) SetFocus(outlineTree);
            else if (HWND first = GetDlgItem(window, OpenMenu)) SetFocus(first);
        } else SetFocus(window);
    }
    void RunKeysTest() {
        bool ok = true;
        std::wstring debug;
        const int expected[] = {OpenMenu, Outline, Find, Previous, Next, Smaller, Larger, Fit, Print, Theme, About};
        HWND item = GetDlgItem(window, OpenMenu);
        for (int id : expected) {
            debug += L"expect=" + std::to_wstring(id) + L" item=" + std::to_wstring(item ? GetDlgCtrlID(item) : -1) +
                L" tab=" + std::to_wstring(item ? (GetWindowLongPtrW(item, GWL_STYLE) & WS_TABSTOP) != 0 : 0) + L"\n";
            ok = ok && item == GetDlgItem(window, id);
            ok = ok && item && (GetWindowLongPtrW(item, GWL_STYLE) & WS_TABSTOP);
            if (!item) break;
            item = GetNextDlgTabItem(window, item, FALSE);
        }
        debug += L"cycle=" + std::to_wstring(item ? GetDlgCtrlID(item) : -1) + L"\n";
        ok = ok && item == GetDlgItem(window, OpenMenu);
        auto accessibleName = [](HWND control) {
            IAccessible* accessible = nullptr;
            std::wstring result;
            if (SUCCEEDED(AccessibleObjectFromWindow(control, static_cast<DWORD>(OBJID_CLIENT), IID_IAccessible,
                reinterpret_cast<void**>(&accessible)))) {
                VARIANT self{}; self.vt = VT_I4; self.lVal = CHILDID_SELF;
                BSTR name = nullptr;
                if (SUCCEEDED(accessible->get_accName(self, &name)) && name) {
                    result = name; SysFreeString(name);
                }
                accessible->Release();
            }
            return result;
        };
        auto queryName = accessibleName(query), themeName = accessibleName(GetDlgItem(window, Theme));
        auto previousName = accessibleName(GetDlgItem(window, Previous));
        debug += L"query=" + queryName + L"\ntheme=" + themeName + L"\nprevious=" + previousName + L"\n";
        ok = ok && queryName.find(L"Buscar") != std::wstring::npos;
        ok = ok && themeName.find(L"Opciones") != std::wstring::npos;
        ok = ok && previousName.find(L"Página anterior") != std::wstring::npos;
        status = L"Estado accesible de prueba"; Redraw();
        auto statusName = accessibleName(statusBar);
        debug += L"status=" + statusName + L"\n";
        ok = ok && statusBar && (GetWindowLongPtrW(statusBar, GWL_STYLE) & WS_VISIBLE) &&
            statusName == L"Estado accesible de prueba";
        SetFocus(window); CycleFocus();
        debug += L"focus1=" + std::to_wstring(GetFocus() ? GetDlgCtrlID(GetFocus()) : -1) + L"\n";
        ok = ok && GetFocus() == GetDlgItem(window, OpenMenu);
        CycleFocus();
        debug += L"focus2=" + std::to_wstring(GetFocus() ? GetDlgCtrlID(GetFocus()) : -1) + L"\n";
        ok = ok && GetFocus() == window;
        // Tab must move focus among the custom toolbar buttons through the dialog manager.
        SetFocus(GetDlgItem(window, OpenMenu));
        MSG key{}; key.hwnd = GetDlgItem(window, OpenMenu); key.message = WM_KEYDOWN; key.wParam = VK_TAB;
        IsDialogMessageW(window, &key);
        debug += L"tab=" + std::to_wstring(GetFocus() ? GetDlgCtrlID(GetFocus()) : -1) + L"\n";
        ok = ok && GetFocus() == GetDlgItem(window, Outline);
        SetFocus(window);
        if (!ok) {
            HANDLE log = CreateFileW((benchmarkReport + L".keys.txt").c_str(), GENERIC_WRITE, 0, nullptr,
                CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
            if (log != INVALID_HANDLE_VALUE) {
                DWORD written = 0;
                WriteFile(log, debug.data(), static_cast<DWORD>(debug.size() * sizeof(wchar_t)), &written, nullptr);
                CloseHandle(log);
            }
        }
        SaveTestImage(benchmarkReport + L".keys.bmp");
        BenchmarkResult(ok);
    }
    void RunDpiTest() {
        bool ok = dpi >= 48;
        SaveTestImage(benchmarkReport + L".initial.bmp");
        auto apply = [&](int target, const wchar_t* suffix) {
            RECT rc{}; GetWindowRect(window, &rc);
            int width = rc.right - rc.left, height = rc.bottom - rc.top;
            RECT suggested{rc.left, rc.top, rc.left + MulDiv(width, target, dpi), rc.top + MulDiv(height, target, dpi)};
            SendMessageW(window, WM_DPICHANGED, MAKELONG(target, target), reinterpret_cast<LPARAM>(&suggested));
            LOGFONTW logFont{}; HFONT current = font;
            ok = ok && dpi == target && current && GetObjectW(current, sizeof(logFont), &logFont) == sizeof(logFont) &&
                logFont.lfHeight == -D(14) && BarPixels() == MulDiv(52, target, 96) &&
                ViewHeight() > 0 && ViewWidth() > 0;
            RECT about{}; GetWindowRect(GetDlgItem(window, About), &about);
            ok = ok && about.right - about.left == D(36);
            if (outlineTree) ok = ok && TreeView_GetItemHeight(outlineTree) == D(32);
            SaveTestImage(benchmarkReport + suffix);
        };
        apply(144, L".144.bmp");
        apply(192, L".192.bmp");
        BenchmarkResult(ok);
    }
    void RunThemeTest() {
        bool ok = true;
        auto writeRaw = [&](const std::string& content) {
            HANDLE file = CreateFileW(settingsFile.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
            if (file == INVALID_HANDLE_VALUE) return false;
            DWORD written = 0;
            bool result = WriteFile(file, content.data(), static_cast<DWORD>(content.size()), &written, nullptr) != 0 &&
                written == content.size();
            CloseHandle(file);
            return result;
        };
        ok = ok && writeRaw("not a settings file\n");
        LoadTheme(); ok = ok && !UiDark(); // Corrupt profile falls back to light.
        // A UTF-8 BOM and CRLF line endings from another editor must still load dark.
        ok = ok && writeRaw(std::string("\xEF\xBB\xBF") + "ChucaoSettings 1\r\ntheme=dark\r\n");
        LoadTheme(); ok = ok && UiDark();
        ToggleTheme(); ok = ok && !UiDark();
        auto settings = SettingsStore(settingsFile).Read();
        ok = ok && settings.has_value() && !settings->dark;
        ToggleTheme(); ok = ok && UiDark();
        settings = SettingsStore(settingsFile).Read();
        ok = ok && settings.has_value() && settings->dark;
        SaveTestImage(benchmarkReport + L".dark.bmp");
        BenchmarkResult(ok);
    }
    void PeekOutline() {
        if (outlinePinned || outlinePeek || dragging || GetCapture() == window || outline.entries.empty()) return;
        outlinePeek = true; ArrangeChrome(); SetTimer(window, 7, 200, nullptr);
    }
    void CheckOutlinePeek() {
        if (!outlinePeek) { KillTimer(window, 7); return; }
        POINT point{}; GetCursorPos(&point); ScreenToClient(window, &point);
        bool inside = point.x >= 0 && point.x < OutlineWidth() + D(24) && point.y >= ContentTop() && point.y < ContentTop() + ViewHeight();
        UpdateOutlinePeek(inside);
    }
    void UpdateOutlinePeek(bool inside) {
        if (!inside && !IsChild(outlineWindow, GetFocus())) { outlinePeek = false; KillTimer(window, 7); ArrangeChrome(); }
    }
    void CheckInterface(bool searched = false) {
        if (interfaceStep == 2) {
            if (outlinePinned || outlinePeek || ViewLeft() != 0 || searchVisible || !outline.entries.empty()) { BenchmarkResult(false); return; }
            SaveTestImage(benchmarkReport + L".without-index.bmp", window); BenchmarkResult(true); return;
        }
        if (interfaceStep == 0) {
            if (!outlinePinned || outline.entries.empty() || GetParent(outlineWindow) != window ||
                !(GetWindowLongPtrW(outlineWindow, GWL_STYLE) & WS_CHILD) || searchVisible ||
                (GetWindowLongPtrW(query, GWL_STYLE) & WS_VISIBLE)) { BenchmarkResult(false); return; }
            int pinnedWidth = ViewWidth(); double oldZoom = zoom; ZoomMode oldMode = zoomMode;
            zoom = 3; zoomMode = ZoomMode::Manual; Reflow(); Render(); ScrollHorizontal(layout.MaxHorizontal(ViewWidth()) / 2);
            int anchor = layout.PageAt(scroll);
            double beforeCenter = (ViewWidth() / 2.0 - layout.Left(anchor, ViewWidth(), horizontal)) / layout.Width(anchor);
            SetOutlinePinned(false);
            double afterCenter = (ViewWidth() / 2.0 - layout.Left(anchor, ViewWidth(), horizontal)) / layout.Width(anchor);
            if (std::abs(beforeCenter - afterCenter) > .003) { BenchmarkResult(false); return; }
            int collapsedWidth = ViewWidth(); auto before = revision;
            PeekOutline();
            if (!outlinePeek || outlinePinned || ViewWidth() != collapsedWidth || revision != before || collapsedWidth <= pinnedWidth) { BenchmarkResult(false); return; }
            ActivateOutlineItem(TreeView_GetChild(outlineTree, TreeView_GetRoot(outlineTree)));
            if (outlinePinned || !outlinePeek) { BenchmarkResult(false); return; }
            UpdateOutlinePeek(false); // Simulate leaving the overlay without moving the desktop pointer.
            if (outlinePeek || outlinePinned || ViewWidth() != collapsedWidth || revision != before) { BenchmarkResult(false); return; }
            ++linkEpoch; // Navigation itself is covered by the outline diagnostic; ignore this pending reply.
            SetOutlinePinned(true);
            if (ViewWidth() != pinnedWidth) { BenchmarkResult(false); return; }
            zoom = oldZoom; zoomMode = oldMode; Reflow(); Render();
            int height = ViewHeight(); ShowSearch(false);
            if (!searchVisible || ViewHeight() != height - 44 || !(GetWindowLongPtrW(query, GWL_STYLE) & WS_VISIBLE)) { BenchmarkResult(false); return; }
            interfaceStep = 1; SetWindowTextW(query, L"Chucao"); Search(); return;
        }
        if (interfaceStep == 1 && searched) {
            if (searchBoxes.empty() || searchPage != 0) { BenchmarkResult(false); return; }
            auto box = searchBoxes.front(); TextPoint hit;
            int x = ViewLeft() + layout.Left(0, ViewWidth(), horizontal) + static_cast<int>((box.left + box.right) * .5 * layout.Width(0));
            int y = ContentTop() + static_cast<int>(layout.Top(0) - scroll + (box.top + box.bottom) * .5 * layout.Height(0));
            if (!PointOnPage(x, y, hit) || hit.page != 0) { BenchmarkResult(false); return; }
            SaveTestImage(benchmarkReport + L".interface.bmp", window);
            HideSearch();
            if (searchVisible || Text(query) != L"Chucao" || !searchBoxes.empty()) { BenchmarkResult(false); return; }
            auto stale = std::make_unique<Result>(); stale->kind = Result::Search; stale->document = document;
            stale->text = L"Chucao"; stale->match = 0; stale->boxes = {box}; Receive(std::move(stale));
            if (!searchBoxes.empty()) { BenchmarkResult(false); return; }
            interfaceStep = 2; Load(path.substr(0, path.find_last_of(L"\\/") + 1) + L"sample.pdf");
        }
    }
    void PopulateOutline() {
        if (!outlineTree) return;
        auto foldedQuery = FoldText(outlineFilterQuery);
        std::vector<bool> visible(outline.entries.size(), false);
        int matches = 0;
        if (foldedQuery.empty()) {
            std::fill(visible.begin(), visible.end(), true);
            matches = static_cast<int>(outline.entries.size());
        } else {
            for (size_t i = 0; i < outline.entries.size(); ++i) {
                if (FoldText(outline.entries[i].title).find(foldedQuery) == std::wstring::npos) continue;
                visible[i] = true; ++matches;
                for (int parent = outline.entries[i].parent; parent >= 0 && parent < static_cast<int>(visible.size());
                    parent = outline.entries[parent].parent) visible[parent] = true;
            }
        }
        SendMessageW(outlineTree, WM_SETREDRAW, FALSE, 0);
        TreeView_DeleteAllItems(outlineTree);
        outlineHighlight = nullptr;
        outlineItems.assign(outline.entries.size(), nullptr);
        for (size_t i = 0; i < outline.entries.size(); ++i) {
            if (!visible[i]) continue;
            const auto& entry = outline.entries[i];
            auto label = entry.title + (entry.page >= 0 ? L"  ·  " + std::to_wstring(entry.page + 1) : L"");
            TVINSERTSTRUCTW item{};
            item.hParent = entry.parent >= 0 && outlineItems[entry.parent] ? outlineItems[entry.parent] : TVI_ROOT;
            item.hInsertAfter = TVI_LAST; item.item.mask = TVIF_TEXT | TVIF_PARAM;
            item.item.pszText = &label[0]; item.item.lParam = static_cast<LPARAM>(i);
            outlineItems[i] = TreeView_InsertItem(outlineTree, &item);
        }
        for (size_t i = 0; i < outlineItems.size(); ++i) {
            if (!outlineItems[i]) continue;
            // While filtering, expand everything so the matches are visible.
            if (outline.entries[i].parent < 0 || !foldedQuery.empty())
                TreeView_Expand(outlineTree, outlineItems[i], TVE_EXPAND);
        }
        SendMessageW(outlineTree, WM_SETREDRAW, TRUE, 0); InvalidateRect(outlineTree, nullptr, FALSE);
        const wchar_t* label = !outlineLoaded ? (outlinePending ? L"Cargando índice…" : L"Abrí un PDF para consultar su índice.") :
            outline.entries.empty() ? L"Este documento no tiene índice." :
            outline.limited ? L"Índice limitado. Clic o Enter para ir." : L"Clic o Enter para ir al capítulo.";
        SetWindowTextW(outlineLabel, label);
        if (outlineLoaded && !outline.entries.empty()) {
            std::wstring summary;
            if (!foldedQuery.empty())
                summary = std::to_wstring(matches) + L" de " + std::to_wstring(outline.entries.size()) +
                    (outline.entries.size() == 1 ? L" sección" : L" secciones");
            else
                summary = std::to_wstring(outline.entries.size()) +
                    (outline.entries.size() == 1 ? L" sección" : L" secciones");
            if (outline.limited) summary += L" · Índice parcial";
            SetWindowTextW(outlineLabel, summary.c_str());
            UpdateOutlineHighlight();
        }
    }
    // Last entry, in document order, that starts at or before the page under the
    // reading point. It is page-granular on purpose: a destination's Y is expressed in
    // the unrotated, uncropped page space and the outline is read without loading pages,
    // so comparing Y would be wrong on rotated or cropped pages.
    int CurrentOutlineEntry() const {
        if (!pages || !outlineLoaded || outline.entries.empty() || layout.Count() == 0) return -1;
        int p = layout.PageAt(scroll + std::min(D(40), ViewHeight() / 2));
        int found = -1;
        for (int i = 0; i < static_cast<int>(outline.entries.size()); ++i)
            if (outline.entries[i].page >= 0 && outline.entries[i].page <= p) found = i;
        return found;
    }
    // Shades the row of the section being read. A section inside a collapsed chapter
    // shades the nearest visible ancestor; the user's expand/collapse state is untouched.
    void UpdateOutlineHighlight(bool scrollIntoView = true) {
        if (!outlineTree || outlineItems.size() != outline.entries.size()) return;
        HTREEITEM target = nullptr;
        int entry = CurrentOutlineEntry();
        if (entry >= 0) {
            std::vector<int> chain;
            for (int i = entry; i >= 0 && chain.size() < 70; i = outline.entries[i].parent) chain.push_back(i);
            std::reverse(chain.begin(), chain.end());
            for (int index : chain) {
                if (index < 0 || index >= static_cast<int>(outlineItems.size()) || !outlineItems[index]) continue;
                if (!target) { target = outlineItems[index]; continue; }
                if (!(TreeView_GetItemState(outlineTree, target, TVIS_EXPANDED) & TVIS_EXPANDED)) break;
                target = outlineItems[index];
            }
        }
        if (target == outlineHighlight) return;
        outlineHighlight = target;
        InvalidateRect(outlineTree, nullptr, FALSE);
        if (!target || !scrollIntoView || GetFocus() == outlineTree) return;
        POINT cursor{}; RECT bounds{}; GetCursorPos(&cursor); GetWindowRect(outlineTree, &bounds);
        if (IsWindowVisible(outlineTree) && PtInRect(&bounds, cursor)) return; // the user is browsing the index
        TreeView_EnsureVisible(outlineTree, target);
    }
    COLORREF OutlineTint() const {
        int percent = UiDark() ? 24 : 16; COLORREF a = UiTheme().bar, b = UiTheme().accent;
        return RGB((GetRValue(a) * (100 - percent) + GetRValue(b) * percent) / 100,
                   (GetGValue(a) * (100 - percent) + GetGValue(b) * percent) / 100,
                   (GetBValue(a) * (100 - percent) + GetBValue(b) * percent) / 100);
    }
    // Drawn before the items: the tree repaints each label's own background afterwards
    // (with the tint, see NM_CUSTOMDRAW), so the full row and the edge bar go here.
    void PaintOutlineHighlight(HDC dc, HTREEITEM item) {
        RECT row{}, client{}; GetClientRect(outlineTree, &client);
        if (!TreeView_GetItemRect(outlineTree, item, &row, FALSE)) return;
        row.left = 0; row.right = client.right;
        SetDCBrushColor(dc, OutlineTint());
        FillRect(dc, &row, reinterpret_cast<HBRUSH>(GetStockObject(DC_BRUSH)));
        RECT edge{row.left, row.top + D(4), row.left + std::max(2, D(3)), row.bottom - D(4)};
        SetDCBrushColor(dc, UiTheme().accent);
        FillRect(dc, &edge, reinterpret_cast<HBRUSH>(GetStockObject(DC_BRUSH)));
    }
    void ApplyOutlineFilter() {
        outlineFilterQuery = outlineFilter ? Text(outlineFilter) : std::wstring{};
        PopulateOutline();
    }
    bool ClearOutlineFilterText() {
        bool had = !outlineFilterQuery.empty() || (outlineFilter && GetWindowTextLengthW(outlineFilter) > 0);
        if (!had) return false;
        outlineFilterQuery.clear();
        if (outlineFilter) SetWindowTextW(outlineFilter, L"");
        PopulateOutline();
        return true;
    }
    void FocusOutlineTree() {
        if (!outlineTree) return;
        SetFocus(outlineTree);
        if (!TreeView_GetSelection(outlineTree)) {
            auto first = TreeView_GetRoot(outlineTree);
            if (first) TreeView_SelectItem(outlineTree, first);
        }
    }
    void NavigateFirstOutlineMatch() {
        auto folded = FoldText(outlineFilterQuery);
        for (size_t i = 0; i < outline.entries.size(); ++i) {
            if (i >= outlineItems.size() || !outlineItems[i]) continue;
            if (folded.empty() || FoldText(outline.entries[i].title).find(folded) != std::wstring::npos) {
                TreeView_SelectItem(outlineTree, outlineItems[i]);
                NavigateOutline();
                return;
            }
        }
    }
    void CheckOutlineFilter() {
        bool ok = outlineLoaded && outline.entries.size() == 5;
        SetWindowTextW(outlineFilter, L"café"); ApplyOutlineFilter();
        ok = ok && TreeView_GetCount(outlineTree) == 1 && Text(outlineLabel).find(L"1 de 5") != std::wstring::npos;
        auto root = TreeView_GetRoot(outlineTree);
        ok = ok && root != nullptr;
        NavigateFirstOutlineMatch();
        ok = ok && TreeView_GetSelection(outlineTree) == root;
        SetWindowTextW(outlineFilter, L"sub"); ApplyOutlineFilter();
        ok = ok && TreeView_GetCount(outlineTree) == 2 && Text(outlineLabel).find(L"1 de 5") != std::wstring::npos;
        SaveTestImage(benchmarkReport + L".outline-filter.bmp", outlineWindow);
        SetWindowTextW(outlineFilter, L"zzz"); ApplyOutlineFilter();
        ok = ok && TreeView_GetCount(outlineTree) == 0 && Text(outlineLabel).find(L"0 de 5") != std::wstring::npos;
        ClearOutlineFilterText();
        ok = ok && TreeView_GetCount(outlineTree) == 5 && Text(outlineLabel).find(L"5 secciones") != std::wstring::npos;
        ++outlineFilterChecks;
        BenchmarkResult(ok);
    }
    void RequestOutline() {
        if (pages && !outlineLoaded && !outlinePending) {
            outlinePending = true; PopulateOutline(); worker->LoadOutline(document);
        }
    }
    void ToggleOutline() {
        RequestOutline(); SetOutlinePinned(!outlinePinned);
        SetFocus(outlinePinned ? outlineTree : window);
    }
    void NavigateOutline() {
        auto selectedItem = TreeView_GetSelection(outlineTree);
        TVITEMW item{}; item.mask = TVIF_PARAM; item.hItem = selectedItem;
        if (!selectedItem || !TreeView_GetItem(outlineTree, &item)) return;
        ++linkEpoch;
        int index = static_cast<int>(item.lParam);
        if (index >= 0 && index < static_cast<int>(outline.entries.size()) && outline.entries[index].page >= 0) {
            pendingReadingState.reset(); ClearSelection();
            worker->FollowDestination(outline.entries[index].destination, document, linkEpoch);
        }
        else { status = L"Esta entrada no tiene un destino dentro del documento."; Redraw(); }
    }
    void ActivateOutlineItem(HTREEITEM item) {
        if (item) {
            TreeView_SelectItem(outlineTree, item); NavigateOutline();
            if (outlinePeek) SetFocus(window);
        }
    }
    void CheckOutline() {
        // The outline is loaded before the first page render so the panel is
        // already pinned when the document appears.
        if (viewReady) { BenchmarkResult(false); return; }
        if (outlineChecks == 2) {
            if (!outline.entries.empty() || TreeView_GetCount(outlineTree) != 0 || Text(outlineLabel) != L"Este documento no tiene índice.") { BenchmarkResult(false); return; }
            ++outlineChecks; BenchmarkResult(true); return;
        }
        if (outline.entries.size() != 5 || outline.entries[0].title != L"Capítulo café" || outline.entries[1].parent != 0 ||
            outline.entries[1].page != 1 || outline.entries[2].page != -1 || outline.entries[3].page != -1 || outline.entries[4].page != -1 ||
            TreeView_GetCount(outlineTree) != 5) { BenchmarkResult(false); return; }
        auto root = TreeView_GetRoot(outlineTree), child = TreeView_GetChild(outlineTree, root);
        TreeView_SelectItem(outlineTree, child);
        SaveTestImage(benchmarkReport + L".outline.bmp", outlineWindow);
        zoom = 3; zoomMode = ZoomMode::Manual; Reflow(); Render();
        outlineStep = 1;
        ActivateOutlineItem(child);
    }
    void CheckOutlineNavigation(const LinkTarget& target) {
        if (!outlineTest) return;
        auto root = TreeView_GetRoot(outlineTree);
        if (outlineStep == 1) {
            // Outline navigation preserves the reader's zoom/mode; only the page
            // and scroll position come from the destination.
            bool positioned = page == 1 && target.page == 1 && zoom == 3;
            if (!destinationsTest) positioned = positioned && target.y > 0 && scroll > layout.Top(1);
            if (!positioned) { BenchmarkResult(false); return; }
            // The section being read (the child that was just opened) must be the shaded row.
            if (!destinationsTest && (outlineItems.size() < 2 || CurrentOutlineEntry() != 1 || outlineHighlight != outlineItems[1])) { BenchmarkResult(false); return; }
            testedDestinationZoom = zoom;
            ++outlineChecks;
            auto stale = std::make_unique<Result>(); stale->kind = Result::OutlineFollowed;
            stale->document = document - 1; stale->linkTarget.page = 0; Receive(std::move(stale));
            auto superseded = std::make_unique<Result>(); superseded->kind = Result::OutlineFollowed;
            superseded->document = document; superseded->revision = linkEpoch - 1; superseded->linkTarget.page = 0; Receive(std::move(superseded));
            if (page != 1) { BenchmarkResult(false); return; }
            outlineStep = 2;
            TreeView_SelectItem(outlineTree, root);
            NMTVKEYDOWN enter{}; enter.hdr.hwndFrom = outlineTree; enter.hdr.code = TVN_KEYDOWN; enter.wVKey = VK_RETURN;
            SendMessageW(outlineWindow, WM_NOTIFY, 0, reinterpret_cast<LPARAM>(&enter));
            return;
        }
        if (page != 0) { BenchmarkResult(false); return; }
        BackFromLink();
        if (page != 1 || zoom != testedDestinationZoom || (!destinationsTest && scroll <= layout.Top(1))) { BenchmarkResult(false); return; }
        BackFromLink();
        if (page != 0 || !linkHistory.empty()) { BenchmarkResult(false); return; }
        TreeView_SelectItem(outlineTree, TreeView_GetNextSibling(outlineTree, root)); NavigateOutline();
        if (page != 0) { BenchmarkResult(false); return; }
        outlineStep = 0;
        if (outlineChecks == 1) {
            Load(path); // Clear the old tree and reject late outline results on reopen.
            if (TreeView_GetCount(outlineTree) != 0) { BenchmarkResult(false); return; }
            auto stale = std::make_unique<Result>(); stale->kind = Result::OutlineReady; stale->document = document - 1;
            Receive(std::move(stale)); RequestOutline();
        } else {
            Load(path.substr(0, path.find_last_of(L"\\/") + 1) + L"sample.pdf");
        }
    }
    void ComposeFrame() {
        RECT rc{}; GetClientRect(window, &rc);
        if (rc.right <= 0 || rc.bottom <= 0) return;
        HDC target = CreateCompatibleDC(nullptr);
        HBITMAP bitmap = target ? CreateCompatibleBitmap(target, rc.right, rc.bottom) : nullptr;
        if (!target || !bitmap) { if (bitmap) DeleteObject(bitmap); if (target) DeleteDC(target); return; }
        auto old = SelectObject(target, bitmap);
        Draw(target, rc, rc);
        SelectObject(target, old); DeleteObject(bitmap); DeleteDC(target);
    }
    void BenchmarkResult(bool success) {
        if (benchmarkReport.empty()) return;
        double readyMs = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - started).count();
        char data[1024]{};
        Endpoint a, b; SelectionRange(a, b);
        int size = sprintf_s(data, "{\"success\":%s,\"shell_ms\":%.3f,\"opened_ms\":%.3f,\"ready_ms\":%.3f,\"pages\":%d,\"navigation_steps\":%d,\"cache_hits\":%zu,\"cache_bytes\":%zu,\"outline_checks\":%d,\"link_checks\":%d,\"password_checks\":%d,\"state_checks\":%d,\"recent_checks\":%d,\"session_checks\":%d,\"tabs_checks\":%d,\"tabs_step\":%d,\"tabs_password_prompts\":%d,\"tabs_closed\":%d,\"text_checks\":%d,\"text_step\":%d,\"selection_first\":%d,\"selection_last\":%d,\"first_page\":%d,\"first_render_ms\":%.3f,\"first_bitmap_ms\":%.3f,\"first_compose_ms\":%.3f}\n", success ? "true" : "false", shellMs, openedMs, readyMs, pages, navigationStep, navigationHits, cacheBytes, outlineChecks, linkChecks, passwordChecks, readingStateChecks, recentChecks, sessionChecks, tabsChecks, tabsStep, tabsPrompts, tabsClosed, textChecks, textStep, a.character, b.character, startupPage, startupRenderMs, startupBitmapMs, startupComposeMs);
        HANDLE output = CreateFileW(benchmarkReport.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (output != INVALID_HANDLE_VALUE) { DWORD written = 0; WriteFile(output, data, size, &written, nullptr); CloseHandle(output); }
        PostMessageW(window, WM_CLOSE, 0, 0);
    }
    void LoadPasswordCase(const wchar_t* name) {
        passwordAttempts = 0; Load(passwordTestRoot + name);
    }
    void TestPasswordPrompt(const std::wstring& prompt) {
        PasswordScript script;
        script.expectIncorrect = passwordAttempts > 0;
        if (passwordStage == 0) script.value = passwordAttempts == 0 ? L"" : passwordAttempts == 1 ? L"wrong" : L"Chucao-test";
        else if (passwordStage == 1) script.value = passwordAttempts == 0 ? L"wrong" : L"Chucao-café-漢字";
        else if (passwordStage == 2) script.action = PasswordScript::Cancel;
        else if (passwordStage == 4) { script.value = L"wrong"; if (passwordAttempts) script.action = PasswordScript::Escape; }
        else if (passwordStage == 6) script.action = PasswordScript::Close;
        else if (passwordStage == 8) script.value = L"reader";
        else { BenchmarkResult(false); return; }
        if (passwordAttempts >= 3) { BenchmarkResult(false); return; }
        std::wstring password;
        bool enabled = IsWindowEnabled(window) != 0;
        bool accepted = PasswordDialog(window, password, prompt, font, &script);
        EraseSecretOnExit<wchar_t> wipe{password};
        ++passwordAttempts;
        if (!script.verified || (IsWindowEnabled(window) != 0) != enabled || accepted != (script.action == PasswordScript::Submit)) { BenchmarkResult(false); return; }
        ++passwordChecks;
        if (accepted) {
            if (password != script.value) { BenchmarkResult(false); return; }
            Load(path, password, true);
        } else {
            status = L"Apertura cancelada."; Redraw();
            if (pages != 0 || !cache.empty() || !password.empty()) { BenchmarkResult(false); return; }
            ++passwordChecks; ++passwordStage; LoadPasswordCase(L"plain.pdf");
        }
    }
    void CheckPasswordImage() {
        if (pages != 2) { BenchmarkResult(false); return; }
        if ((passwordStage == 0 && passwordAttempts != 3) || (passwordStage == 1 && passwordAttempts != 2) ||
            ((passwordStage == 3 || passwordStage == 5 || passwordStage == 7) && passwordAttempts != 0)) { BenchmarkResult(false); return; }
        ++passwordChecks;
        if (passwordStage == 0) { passwordStage = 1; LoadPasswordCase(L"aes256-unicode.pdf"); }
        else if (passwordStage == 1) { passwordStage = 2; LoadPasswordCase(L"aes256-ascii.pdf"); }
        else if (passwordStage == 3) { passwordStage = 4; LoadPasswordCase(L"aes256-ascii.pdf"); }
        else if (passwordStage == 5) { passwordStage = 6; LoadPasswordCase(L"aes256-ascii.pdf"); }
        else if (passwordStage == 7) { passwordStage = 8; LoadPasswordCase(L"restricted.pdf"); }
        else if (passwordStage == 8) {
            if (canCopy || canPrint) { BenchmarkResult(false); return; }
            passwordStage = 9; worker->CopyText(0, 0, 0, 5, document, selectionRevision);
        } else BenchmarkResult(false);
    }
    void CheckLinks(Result::Kind kind, const LinkTarget& target = {}) {
        if (!linksTest) return;
        if (kind == Result::Image && (linkStep == 0 || linkStep == 5)) {
            linkStep = linkStep == 0 ? 1 : 6;
            pendingLinks.insert(0); worker->LoadLinks(0, document);
        } else if (kind == Result::LinksReady && linkStep == 1) {
            if (pageLinks[0].size() != 4 || webOpenAttempts) { BenchmarkResult(false); return; }
            auto box = pageLinks[0][0].box; TextPoint point{0, (box.left + box.right) / 2, (box.top + box.bottom) / 2};
            if (!LinkAt(point)) { BenchmarkResult(false); return; }
            zoom = 3; zoomMode = ZoomMode::Manual; Reflow(); Render();
            ScrollHorizontal(static_cast<int>(layout.Left(0, ViewWidth()) + point.x * layout.Width(0) - ViewWidth() / 2.0));
        int sx = ViewLeft() + layout.Left(0, ViewWidth(), horizontal) + static_cast<int>(point.x * layout.Width(0));
            int sy = ContentTop() + static_cast<int>(layout.Top(0) - scroll + point.y * layout.Height(0)); TextPoint hit;
            if (!PointOnPage(sx, sy, hit) || !LinkAt(hit)) { BenchmarkResult(false); return; }
            point = hit; linkTestHorizontal = horizontal;
            linkTestScroll = scroll; linkStep = 2; ActivateLink(point);
        } else if (kind == Result::LinkFollowed && linkStep == 2) {
            if (target.page != 1 || page != 1 || linkHistory.size() != 1 || scroll <= layout.Top(1)) { BenchmarkResult(false); return; }
            BackFromLink();
            if (page != 0 || scroll != linkTestScroll || std::abs(horizontal - linkTestHorizontal) > 1 || zoom != 3 || !linkHistory.empty()) { BenchmarkResult(false); return; }
            ++linkChecks;
            auto stale = std::make_unique<Result>(); stale->kind = Result::LinkFollowed; stale->document = document - 1; stale->linkTarget = target;
            Receive(std::move(stale));
            if (page != 0) { BenchmarkResult(false); return; }
            ++linkChecks;
            auto superseded = std::make_unique<Result>(); superseded->kind = Result::LinkFollowed;
            superseded->document = document; superseded->revision = linkEpoch - 1; superseded->linkTarget = target;
            Receive(std::move(superseded));
            if (page != 0) { BenchmarkResult(false); return; }
            auto web = pageLinks[0][2].box;
            ++linkChecks; linkStep = 4; ActivateLink({0, (web.left + web.right) / 2, (web.top + web.bottom) / 2});
        } else if (kind == Result::LinkFollowed && linkStep == 4) {
            if (!target.hit || target.page >= 0 || target.webUrl != L"https://example.invalid" || webOpenAttempts != 1 || page != 0 || !linkHistory.empty()) { BenchmarkResult(false); return; }
            auto late = std::make_unique<Result>(); late->kind = Result::LinkFollowed; late->document = document - 1; late->linkTarget = target;
            Receive(std::move(late));
            auto old = std::make_unique<Result>(); old->kind = Result::LinkFollowed; old->document = document; old->revision = linkEpoch - 1; old->linkTarget = target;
            Receive(std::move(old));
            if (webOpenAttempts != 1) { BenchmarkResult(false); return; }
            ++linkChecks; linkStep = 5; Load(path);
            if (!pageLinks.empty()) { BenchmarkResult(false); return; }
        } else if (kind == Result::LinksReady && linkStep == 6) {
            if (pageLinks[0].size() != 4 || !linkHistory.empty() || webOpenAttempts != 1) { BenchmarkResult(false); return; }
            ++linkChecks; linkStep = 7; BenchmarkResult(true);
        }
    }
    void CheckText(Result::Kind kind, const std::wstring& value = L"") {
        // The diagnostic consumes copy results internally, never touches the clipboard.
        if (!textTest) return;
        if (textStep == 0 && kind == Result::Image) {
            if (pages < 2) { BenchmarkResult(false); return; }
            textStep = 1; SetWindowTextW(query, L"Chucao"); Search();
        } else if (textStep == -1 && kind == Result::Image) {
            textStep = 1; Search();
        } else if (textStep == 1 && kind == Result::Search) {
            if (searchPage != 0 || searchBoxes.empty() || !searchBoxes.front().Valid()) { BenchmarkResult(false); return; }
            testWordStart[0] = searchMatch;
            SaveTestImage(benchmarkReport + L".search.bmp");
            ++textChecks; textStep = 2; NeedText(0);
        } else if (textStep == 2 && kind == Result::Geometry) {
            const auto& boxes = textPages[0];
            int first = testWordStart[0];
            if (first < 0 || boxes.size() < static_cast<size_t>(first + 6) || !boxes[first].Valid() || !boxes[first + 5].Valid()) { BenchmarkResult(false); return; }
            const auto& a = boxes[first]; const auto& b = boxes[first + 5];
            selectionStart = {0, (a.left + a.right) / 2, (a.top + a.bottom) / 2};
            selectionEnd = {0, (b.left + b.right) / 2, (b.top + b.bottom) / 2};
            TextPoint hit;
            int x = ViewLeft() + layout.Left(0, ViewWidth(), horizontal) + static_cast<int>(selectionStart.x * layout.Width(0));
            int y = ContentTop() + static_cast<int>(layout.Top(0) - scroll + selectionStart.y * layout.Height(0));
            if (!PointOnPage(x, y, hit) || NearestCharacter(boxes, hit.x, hit.y, layout.Height(0) / static_cast<double>(layout.Width(0))) != first) { BenchmarkResult(false); return; }
            selected = true; ++selectionRevision; textStep = 3; CopySelection();
        } else if (textStep == 3 && kind == Result::Copied) {
            if (value != L"Chucao") { BenchmarkResult(false); return; }
            ++textChecks; textStep = 4; ScrollHorizontal(layout.MaxHorizontal(ViewWidth())); Search(true);
        } else if (textStep == 4 && kind == Result::Search) {
            if (searchPage != 1 || searchBoxes.empty()) { BenchmarkResult(false); return; }
            int left = layout.Left(1, ViewWidth(), horizontal) + static_cast<int>(searchBoxes.front().left * layout.Width(1));
            if (left < 0 || left >= ViewWidth()) { BenchmarkResult(false); return; }
            testWordStart[1] = searchMatch;
            ++textChecks; textStep = 5; NeedText(1);
        } else if (textStep == 5 && kind == Result::Geometry) {
            const auto& start = textPages[1]; const auto& end = textPages[0];
            if (testWordStart[1] < 0 || start.size() < static_cast<size_t>(testWordStart[1] + 6) || end.empty()) { BenchmarkResult(false); return; }
            const auto& a = start[testWordStart[1] + 5]; const auto& b = end[testWordStart[0]];
            selectionStart = {1, (a.left + a.right) / 2, (a.top + a.bottom) / 2};
            selectionEnd = {0, (b.left + b.right) / 2, (b.top + b.bottom) / 2};
            selected = true; ++selectionRevision; textStep = 6; CopySelection();
        } else if (textStep == 6 && kind == Result::Copied) {
            if (value.size() <= 12 || value.substr(0, 6) != L"Chucao" || value.substr(value.size() - 6) != L"Chucao" || value.find(L"\r\n") == std::wstring::npos || value.find(L"café") == std::wstring::npos) { BenchmarkResult(false); return; }
            crossPageExpected = value; ++textChecks;
            std::swap(selectionStart, selectionEnd); ++selectionRevision; textStep = 7; CopySelection();
        } else if (textStep == 7 && kind == Result::Copied) {
            if (value != crossPageExpected) { BenchmarkResult(false); return; }
            ++textChecks; textStep = 8;
            SaveTestImage(benchmarkReport + L".bmp");
            if (zoom == 0) {
                ClearSelection(); textPages.clear(); pendingText.clear();
                textStep = -1; zoom = 3; zoomMode = ZoomMode::Manual; Reflow(); Render(); return;
            }
            BenchmarkResult(true);
        }
    }
    void CheckNavigation() {
        if (!navigationTest || !pages || benchmarkReport.empty()) return;
        auto current = cache.find(page);
        if (current == cache.end() || current->second.version != viewVersion) return;
        if (cacheBytes > CacheLimit) { BenchmarkResult(false); return; }
        switch (navigationStep) {
        case 0:
            // A native scrollbar can notify a capture change to this same HWND.
            SetCapture(window);
            SendMessageW(window, WM_CAPTURECHANGED, 0, reinterpret_cast<LPARAM>(window));
            EndSelection();
            if (GetCapture() != window) { BenchmarkResult(false); return; }
            SendMessageW(window, WM_VSCROLL, SB_PAGEDOWN, 0);
            if (scroll <= 0 || GetCapture() != window) { BenchmarkResult(false); return; }
            dragging = true;
            SendMessageW(window, WM_CAPTURECHANGED, 0, reinterpret_cast<LPARAM>(window));
            if (!dragging || GetCapture() != window) { BenchmarkResult(false); return; }
            ReleaseCapture();
            if (dragging) { BenchmarkResult(false); return; }
            SetCapture(window); dragging = true; EndSelection();
            if (GetCapture() == window || dragging) { BenchmarkResult(false); return; }
            SendMessageW(window, WM_VSCROLL, SB_TOP, 0);
            if (scroll != 0) { BenchmarkResult(false); return; }
            navigationStep = 1;
            ScrollTo(layout.Top(std::min(1, pages - 1)) + layout.Height(std::min(1, pages - 1)) / 3);
            break;
        case 1: {
            int anchor = layout.PageAt(scroll);
            double before = (scroll - layout.Top(anchor)) / static_cast<double>(layout.Height(anchor));
            navigationStep = 2; Command(Larger);
            double after = (scroll - layout.Top(anchor)) / static_cast<double>(layout.Height(anchor));
            if (std::abs(before - after) > 0.01 && scroll > 0 && scroll < MaxScroll()) { BenchmarkResult(false); return; }
            break;
        }
        case 2: navigationStep = 3; zoom = 4; zoomMode = ZoomMode::Manual; Reflow(); Render(); break;
        case 3: {
            auto before = revision;
            SendMessageW(window, WM_HSCROLL, SB_RIGHT, 0);
            int right = layout.Left(page, ViewWidth(), horizontal) + layout.Width(page);
            if (horizontal <= 0 || horizontal != layout.MaxHorizontal(ViewWidth()) || right > ViewWidth() - PageGap()) { BenchmarkResult(false); return; }
            SaveTestImage(benchmarkReport + L".horizontal.bmp");
            SendMessageW(window, WM_HSCROLL, SB_LEFT, 0);
            if (horizontal != 0 || revision != before) { BenchmarkResult(false); return; }
            ScrollHorizontal(layout.MaxHorizontal(ViewWidth()) / 2);
            int anchor = layout.PageAt(scroll);
            double center = (ViewWidth() / 2.0 - layout.Left(anchor, ViewWidth(), horizontal)) / layout.Width(anchor);
            Command(Smaller);
            double after = (ViewWidth() / 2.0 - layout.Left(anchor, ViewWidth(), horizontal)) / layout.Width(anchor);
            if (std::abs(center - after) > 0.003) { BenchmarkResult(false); return; }
            navigationStep = 4; Command(Fit);
            if (horizontal != 0 || layout.MaxHorizontal(ViewWidth()) != 0) { BenchmarkResult(false); return; }
            break;
        }
        case 4: navigationStep = 5; ChangePage(pages - 1); break;
        case 5:
            // Reuse the last page at the same scale without another render.
            {
                auto before = revision; ChangePage(pages - 1);
                if (revision != before) { BenchmarkResult(false); return; }
                ++navigationHits; navigationStep = 6;
            }
            break;
        case 6: navigationStep = 7; ChangePage(0); break;
        case 7: {
            // Anchor zoom: the document point under an explicit anchor must stay
            // under the same pixel, with horizontal scrolling active too.
            zoom = 2; zoomMode = ZoomMode::Manual; Reflow();
            ScrollTo(layout.Top(0) + layout.Height(0) / 4);
            ScrollHorizontal(layout.MaxHorizontal(ViewWidth()) / 2);
            int cx = ViewLeft() + ViewWidth() / 2;
            int cy = ContentTop() + ViewHeight() / 2;
            TextPoint before;
            if (!PointOnPage(cx, cy, before)) { BenchmarkResult(false); return; }
            double zoomBefore = zoom;
            Zoom(true, cx, cy);
            TextPoint after;
            if (!PointOnPage(cx, cy, after)) { BenchmarkResult(false); return; }
            bool preserved = after.page == before.page &&
                std::abs(after.x - before.x) <= 0.01 && std::abs(after.y - before.y) <= 0.01;
            if (!preserved || !(zoom > zoomBefore) || zoomMode != ZoomMode::Manual) { BenchmarkResult(false); return; }
            navigationStep = 8; break;
        }
        case 8: {
            // Page-number entry: validate the parser and the jump, without mouse.
            bool ok = ParsePageNumber(L"", pages) == 0 &&
                ParsePageNumber(L"abc", pages) == 0 &&
                ParsePageNumber(L"3x", pages) == 0 &&
                ParsePageNumber(L"0", pages) == 1 &&
                ParsePageNumber(L"1", pages) == 1 &&
                ParsePageNumber(L"2", pages) == 2 &&
                ParsePageNumber(L"999999999999", pages) == pages;
            if (!ok) { BenchmarkResult(false); return; }
            int target = ParsePageNumber(L"2", pages);
            if (target < 1 || target > pages) { BenchmarkResult(false); return; }
            ChangePage(target - 1);
            if (page != 1) { BenchmarkResult(false); return; }
            navigationStep = 9; break;
        }
        case 9:
            navigationStep = 10;
            // Reopen with queued work and verify that old-document results are ignored.
            Load(path); break;
        case 10: navigationStep = 11; BenchmarkResult(true); break;
        }
        if (navigationStep < 11) PostMessageW(window, WM_TIMER, 3, 0);
    }

    void CheckFit() {
        if (!fitTest || !pages) return;
        int margin = 2 * PageGap() + 16;
        int expectedWidth = ViewWidth() - margin, expectedHeight = ViewHeight() - margin;
        int width = layout.Width(page), height = layout.Height(page);
        if (fitStep == 0) { fitStep = 1; Command(FitPageCmd); return; }
        if (fitStep == 1) {
            bool fits = zoomMode == ZoomMode::FitPage && width <= expectedWidth + 1 && height <= expectedHeight + 1 &&
                (std::abs(width - expectedWidth) <= 2 || std::abs(height - expectedHeight) <= 2);
            if (!fits) { BenchmarkResult(false); return; }
            fitStep = 2; Command(FitHeightCmd); return;
        }
        if (fitStep == 2) {
            bool fits = zoomMode == ZoomMode::FitHeight && std::abs(height - expectedHeight) <= 2;
            if (!fits) { BenchmarkResult(false); return; }
            fitStep = 3; Command(Fit); return;
        }
        if (fitStep == 3) {
            bool fits = zoomMode == ZoomMode::FitWidth && std::abs(width - expectedWidth) <= 2;
            BenchmarkResult(fits);
        }
    }
    // Screen readers follow the status line through a real control: changing its
    // text raises an accessibility event only when the message actually changed.
    void SyncStatus() {
        if (!statusBar || status == announcedStatus) return;
        announcedStatus = status;
        SetWindowTextW(statusBar, status.c_str());
        NotifyWinEvent(EVENT_OBJECT_NAMECHANGE, statusBar, OBJID_CLIENT, CHILDID_SELF);
    }
    bool IsLoading() const { return !loadingFailed && (openingDocument || (pages > 0 && !viewReady)); }
    void UpdateLoadingAnimation() {
        bool loading = IsLoading();
        if (loading && !loadingTimer) { SetTimer(window, 5, 40, nullptr); loadingTimer = true; }
        else if (!loading && loadingTimer) { KillTimer(window, 5); loadingTimer = false; }
    }
    void TickLoading() {
        if (!IsLoading()) { KillTimer(window, 5); loadingTimer = false; return; }
        spinAngle = (spinAngle + 15) % 360;
        RECT card = LoadingCardRect();
        InflateRect(&card, D(6), D(6));
        InvalidateRect(window, &card, FALSE);
    }
    void Redraw() { UpdateLoadingAnimation(); SyncStatus(); InvalidateRect(window, nullptr, FALSE); }
    bool ClearReadingHistory(bool confirm = true, HWND owner = nullptr) {
        if (!readingStateWrites || !ConfigureReadingState() || (!confirm && !recentTest)) return false;
        if (confirm && MessageBoxW(owner ? owner : window, L"¿Olvidar la lista de recientes y las posiciones guardadas?", L"Historial de Chucao", MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2) != IDYES) return false;
        if (!ReadingStateStore(readingStateFile).Clear()) { status = L"No se pudo guardar el cambio del historial. Se conserva el archivo anterior."; Redraw(); return false; }
        recentEntries.clear(); suppressCurrentState = true; pendingReadingState.reset();
        for (auto& tab : tabs) tab.reading.reset();
        if (settingsEnabled && sessionWrites) { auto file = SessionFile(); if (!file.empty()) SessionStore(file).Write({}); }
        status = L"Historial y posiciones olvidados."; RefreshHomeRecents(); return true;
    }
    void CheckRecents() {
        if (!recentTest || !pages || pendingReadingState || !cache.count(layout.PageAt(scroll))) return;
        RefreshHomeRecents();
        bool ok = homeList && SendMessageW(homeList, LB_GETCOUNT, 0, 0) == static_cast<LRESULT>(recentEntries.size());
        if (recentStep == 0) {
            ok = ok && recentEntries.size() == 3 && SameTab(recentEntries.front().path, path);
            for (const auto& entry : recentEntries) if (!SameTab(entry.path, path)) {
                if (entry.path.find(L"ausente.pdf") != std::wstring::npos) recentMissing = entry.path;
                else recentOther = entry.path;
            }
            ok = ok && !recentOther.empty() && !recentMissing.empty();
        } else ok = ok && SameTab(path, recentOther);
        if (!ok) { BenchmarkResult(false); return; }
        ++recentChecks;
        if (recentStep == 0) { recentStep = 1; OpenRecentForTest(recentOther); }
        else if (recentStep == 1) { recentStep = 2; OpenRecentForTest(recentMissing); }
        else {
            if (!ClearReadingHistory(false) || !ReadingStateStore(readingStateFile).Read().empty() || SaveReadingState()) { BenchmarkResult(false); return; }
            ++recentChecks; recentStep = 4;
            BenchmarkResult(recentEntries.empty() && SendMessageW(homeList, LB_GETCOUNT, 0, 0) == 0);
        }
    }
    void OpenRecentForTest(const std::wstring& target) {
        for (size_t i = 0; i < recentEntries.size(); ++i) if (recentEntries[i].path == target) { Command(RecentBase + static_cast<int>(i)); return; }
        BenchmarkResult(false);
    }
    void CheckReadingState() {
        if (!readingStateTest || !pages || pendingReadingState) return;
        if (readingStateTest == 1 && readingStateStep == 0) {
            readingStateStep = 1;
            ReadingState desired{path, pages, pages - 1, 3, .35, .65, 0};
            ApplyReadingState(desired); Render(); PostMessageW(window, WM_TIMER, 6, 0); return;
        }
        auto image = cache.find(layout.PageAt(scroll));
        if (image == cache.end() || image->second.version != viewVersion) return;
        auto current = CurrentReadingState();
        if (current.page != pages - 1 || current.zoom != 3 || std::abs(current.vertical - .35) > .005 || std::abs(current.horizontal - .65) > .005) { BenchmarkResult(false); return; }
        ++readingStateChecks;
        if (readingStateTest == 1) {
            if (!SaveReadingState() || !ReadingStateStore(readingStateFile).Find(path, pages)) { BenchmarkResult(false); return; }
        } else if (!readingStateFound) { BenchmarkResult(false); return; }
        ++readingStateChecks; readingStateStep = 2; BenchmarkResult(true);
    }
    bool ConfigureReadingState() {
        if (!readingStateEnabled) return false;
        if (stateConfigured) return !readingStateFile.empty();
        stateConfigured = true;
        if (!readingStateFile.empty()) return true;
        PWSTR folder = nullptr;
        if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, nullptr, &folder))) {
            readingStateFile = std::wstring(folder) + L"\\ChucaoPDFReader\\reading-state.dat"; CoTaskMemFree(folder);
        }
        return !readingStateFile.empty();
    }
    ReadingState CurrentReadingState() const {
        ReadingState state; state.path = path; state.pages = pages;
        state.page = layout.PageAt(scroll); state.zoom = zoom;
        auto offset = scroll - layout.Top(state.page); int height = layout.Height(state.page);
        state.vertical = std::max(0.0, std::min(1.0, offset / static_cast<double>(height)));
        state.gap = offset < 0 ? static_cast<int>(offset) : offset > height ? static_cast<int>(offset - height) : 0;
        state.horizontal = std::max(0.0, std::min(1.0, (ViewWidth() / 2.0 - layout.Left(state.page, ViewWidth(), horizontal)) / layout.Width(state.page)));
        return state;
    }
    bool SaveReadingState() {
        if (suppressCurrentState || !readingStateEnabled || !readingStateWrites || !pages || !viewReady || pendingReadingState || !cache.count(layout.PageAt(scroll)) || !ConfigureReadingState()) return false;
        return ReadingStateStore(readingStateFile).Update(CurrentReadingState());
    }
    void ApplyReadingState(const ReadingState& state) {
        zoom = state.zoom; zoomMode = state.zoom > 0 ? ZoomMode::Manual : ZoomMode::FitWidth;
        layout.Rebuild(ViewWidth(), ViewHeight(), zoom, zoomMode, PageGap());
        ScrollTo(layout.Top(state.page) + static_cast<std::int64_t>(state.vertical * layout.Height(state.page)) + state.gap, false);
        ScrollHorizontal(static_cast<int>(layout.Left(state.page, ViewWidth()) + state.horizontal * layout.Width(state.page) - ViewWidth() / 2.0));
        PageStatus();
    }
    void RememberTabView() {
        if (suppressCurrentState || !pages || !viewReady || pendingReadingState || !cache.count(layout.PageAt(scroll))) return;
        for (auto& tab : tabs) if (SameTab(tab.path, path)) { tab.reading = CurrentReadingState(); break; }
    }
    std::optional<ReadingState> SessionTabView() const {
        for (const auto& tab : tabs) if (SameTab(tab.path, path) && tab.reading && tab.reading->pages == pages) return tab.reading;
        return {};
    }
    std::wstring TabRetryHint() const {
        return activeTab >= 0 && activeTab < static_cast<int>(tabs.size()) && SameTab(tabs[activeTab].path, path) ?
            L" · Clic en la pestaña para reintentar." : L"";
    }
    void CancelPasswordOpen() {
        openingDocument = false; status = L"Apertura cancelada." + TabRetryHint(); ArrangeHome(); Redraw();
    }
    bool PointOnPage(int x, int y, TextPoint& point, bool clamp = false) {
        if (!pages || (!clamp && (x < ViewLeft() || x >= ViewLeft() + ViewWidth() || y < ContentTop() || y >= ContentTop() + ViewHeight()))) return false;
        auto documentY = scroll + y - ContentTop();
        int index = layout.PageAt(documentY);
        int left = ViewLeft() + layout.Left(index, ViewWidth(), horizontal);
        double px = (x - left) / static_cast<double>(layout.Width(index));
        double py = (documentY - layout.Top(index)) / static_cast<double>(layout.Height(index));
        if (!clamp && (px < 0 || px > 1 || py < 0 || py > 1)) return false;
        point = {index, std::max(0.0, std::min(1.0, px)), std::max(0.0, std::min(1.0, py))}; return true;
    }
    void NeedText(int index) {
        if (textPages.count(index) || pendingText.count(index)) return;
        if (pendingText.size() >= 4) return;
        pendingText.insert(index); worker->TextGeometry(index, document);
    }
    void SelectionText() {
        NeedText(selectionStart.page); NeedText(selectionEnd.page);
        int low = std::min(selectionStart.page, selectionEnd.page), high = std::max(selectionStart.page, selectionEnd.page);
        int first = layout.PageAt(scroll), last = layout.PageAt(scroll + ViewHeight());
        for (int i = std::max(low, first); i <= std::min(high, last); ++i) NeedText(i);
    }
    bool SelectionRange(Endpoint& first, Endpoint& last) {
        if (!selected) return false;
        auto a = textPages.find(selectionStart.page), b = textPages.find(selectionEnd.page);
        if (a == textPages.end() || b == textPages.end()) return false;
        first = {selectionStart.page, NearestCharacter(a->second, selectionStart.x, selectionStart.y, layout.Height(selectionStart.page) / static_cast<double>(layout.Width(selectionStart.page)))};
        last = {selectionEnd.page, NearestCharacter(b->second, selectionEnd.x, selectionEnd.y, layout.Height(selectionEnd.page) / static_cast<double>(layout.Width(selectionEnd.page)))};
        if (first.character < 0 || last.character < 0) return false;
        if (first.page > last.page || (first.page == last.page && first.character > last.character)) std::swap(first, last);
        return true;
    }
    void ClearSelection() { selected = false; copyWhenReady = false; ++selectionRevision; ++linkEpoch; Redraw(); }
    bool LinkAt(const TextPoint& point) const {
        auto found = pageLinks.find(point.page); if (found == pageLinks.end()) return false;
        for (const auto& link : found->second) if (point.x >= link.box.left && point.x <= link.box.right && point.y >= link.box.top && point.y <= link.box.bottom) return true;
        return false;
    }
    void HoverLinks(int x, int y) {
        if (dragging) return;
        TextPoint point;
        if (PointOnPage(x, y, point) && !pageLinks.count(point.page) && !pendingLinks.count(point.page) && pendingLinks.size() < 2) {
            pendingLinks.insert(point.page); worker->LoadLinks(point.page, document);
        }
    }
    void ActivateLink(const TextPoint& point) { worker->FollowLink(point.page, point.x, point.y, document, linkEpoch); }
    void OpenWebLink(const std::wstring& url) {
        if (!IsWebUrl(url)) return;
        // Diagnostics exercise dispatch without opening an external application.
        INT_PTR result = 0;
        if (linksTest) { ++webOpenAttempts; result = 33; }
        else if (benchmarkReport.empty()) result = reinterpret_cast<INT_PTR>(ShellExecuteW(window, L"open", url.c_str(), nullptr, nullptr, SW_SHOWNORMAL));
        status = result > 32 ? L"Enlace enviado al navegador." : L"No se pudo abrir el navegador para este enlace.";
        Redraw();
    }
    void BackFromLink() {
        ++linkEpoch;
        if (linkHistory.empty() || !pages) return;
        auto previous = linkHistory.back(); linkHistory.pop_back();
        ClearSelection(); zoom = previous.zoom;
        zoomMode = previous.zoom > 0 ? ZoomMode::Manual : ZoomMode::FitWidth;
        Reflow();
        ScrollTo(layout.Top(previous.page) + static_cast<std::int64_t>(previous.fraction * layout.Height(previous.page)));
        ScrollHorizontal(static_cast<int>(layout.Left(previous.page, ViewWidth()) + previous.horizontal * layout.Width(previous.page) - ViewWidth() / 2.0));
        PageStatus();
    }
    void BeginSelection(int x, int y) {
        pendingReadingState.reset();
        TextPoint point;
        if (!PointOnPage(x, y, point)) { ClearSelection(); SetFocus(window); return; }
        ClearSelection(); selectionStart = selectionEnd = point; dragStart = {x, y}; dragging = true;
        SetFocus(window); SetCapture(window); SetTimer(window, 4, 35, nullptr);
        if (!LinkAt(point)) NeedText(point.page);
    }
    void MoveSelection(int x, int y) {
        if (!dragging) return;
        TextPoint point; if (!PointOnPage(x, y, point, true)) return;
        if (std::abs(x - dragStart.x) + std::abs(y - dragStart.y) >= 3 || point.page != selectionStart.page) selected = true;
        selectionEnd = point; copyWhenReady = false; ++selectionRevision;
        if (selected) SelectionText(); Redraw();
    }
    void EndSelection(bool release = true) {
        // The same HWND also owns capture during native scrollbar tracking.
        if (!dragging) return;
        dragging = false; KillTimer(window, 4);
        if (release && GetCapture() == window) ReleaseCapture();
    }
    void FinishSelection(int x, int y) {
        if (!dragging) return;
        MoveSelection(x, y); bool clicked = !selected; TextPoint point;
        bool onPage = PointOnPage(x, y, point); EndSelection();
        if (clicked && onPage) ActivateLink(point);
    }
    void ShowTextMenu(POINT screen) {
        HMENU menu = CreatePopupMenu();
        if (!menu) return;
        // Enable the item only when there is a selection. CopySelection itself
        // reports permission or pending-text limitations after the click.
        AppendMenuW(menu, MF_STRING | (selected ? MF_ENABLED : MF_GRAYED), Copy, L"Copiar\tCtrl+C");
        SetForegroundWindow(window);
        int command = TrackPopupMenu(menu, TPM_RETURNCMD | TPM_RIGHTBUTTON | TPM_LEFTALIGN | TPM_TOPALIGN,
            screen.x, screen.y, 0, window, nullptr);
        DestroyMenu(menu);
        // Required by the classic TrackPopupMenu contract so the next click is
        // delivered to the reader instead of the dismissed menu.
        PostMessageW(window, WM_NULL, 0, 0);
        if (command) Command(command);
    }
    void ShowTextMenuAtSelection() {
        if (!pages) return;
        int x = ViewLeft() + ViewWidth() / 2, y = ContentTop() + ViewHeight() / 2;
        if (selected) {
            int selectionPage = selectionStart.page;
            x = ViewLeft() + layout.Left(selectionPage, ViewWidth(), horizontal) + static_cast<int>(selectionStart.x * layout.Width(selectionPage));
            y = ContentTop() + static_cast<int>(layout.Top(selectionPage) - scroll + selectionStart.y * layout.Height(selectionPage));
            if (x < ViewLeft() || x >= ViewLeft() + ViewWidth() || y < ContentTop() || y >= ContentTop() + ViewHeight()) {
                x = ViewLeft() + ViewWidth() / 2; y = ContentTop() + ViewHeight() / 2;
            }
        }
        POINT screen{x, y}; ClientToScreen(window, &screen);
        ShowTextMenu(screen);
    }
    void CopySelection() {
        if (!selected) return;
        if (!canCopy) { status = L"El documento no permite copiar texto con esta contraseña."; Redraw(); return; }
        Endpoint first, last;
        if (!SelectionRange(first, last)) {
            if (!pendingText.empty()) { copyWhenReady = true; status = L"Preparando texto…"; }
            else status = L"No hay texto seleccionable en esta zona.";
            Redraw(); return;
        }
        copyWhenReady = false;
        worker->CopyText(first.page, first.character, last.page, last.character, document, selectionRevision);
    }
    void SetClipboardText(const std::wstring& value) {
        if (value.empty()) { status = L"La selección no contiene texto."; Redraw(); return; }
        size_t bytes = (value.size() + 1) * sizeof(wchar_t);
        HGLOBAL memory = GlobalAlloc(GMEM_MOVEABLE, bytes);
        void* destination = memory ? GlobalLock(memory) : nullptr;
        if (!destination) { if (memory) GlobalFree(memory); status = L"No hay memoria para copiar el texto."; Redraw(); return; }
        memcpy(destination, value.c_str(), bytes); GlobalUnlock(memory);
        if (!OpenClipboard(window)) { GlobalFree(memory); status = L"El portapapeles está ocupado. Intentá copiar otra vez."; }
        else {
            if (EmptyClipboard() && SetClipboardData(CF_UNICODETEXT, memory)) { memory = nullptr; status = L"Texto copiado."; }
            else status = L"No se pudo copiar el texto.";
            CloseClipboard(); if (memory) GlobalFree(memory);
        }
        Redraw();
    }
    int ViewHeight() const { RECT rc{}; GetClientRect(window, &rc); return std::max<int>(1, rc.bottom - ContentTop() - D(26) - (searchVisible ? D(44) : 0)); }
    int ViewWidth() const { RECT rc{}; GetClientRect(window, &rc); return std::max<int>(1, rc.right - ViewLeft()); }
    std::int64_t MaxScroll() const { return std::max<std::int64_t>(0, layout.Total() - ViewHeight()); }
    void ScrollHorizontal(int value) {
        horizontal = std::max(0, std::min(value, layout.MaxHorizontal(ViewWidth())));
        SCROLLINFO info{sizeof(info), SIF_RANGE | SIF_PAGE | SIF_POS | SIF_DISABLENOSCROLL};
        info.nMax = layout.HorizontalExtent(ViewWidth()) - 1;
        info.nPage = ViewWidth(); info.nPos = horizontal;
        SetScrollInfo(window, SB_HORZ, &info, TRUE); Redraw();
    }
    void PageStatus() {
        if (pages) status = L"Página " + std::to_wstring(page + 1) + L" de " + std::to_wstring(pages);
        SyncStatus();
    }
    void ScrollTo(std::int64_t value, bool request = true) {
        RECT rc{}; GetClientRect(window, &rc);
        ScrollHorizontal(horizontal);
        scroll = std::max<std::int64_t>(0, std::min(value, MaxScroll()));
        int previous = page;
        if (pages) page = layout.PageAt(scroll + std::min(D(40), ViewHeight() / 2));
        if (page != previous) PageStatus();
        UpdateOutlineHighlight();
        SCROLLINFO info{sizeof(info), SIF_RANGE | SIF_PAGE | SIF_POS};
        info.nMin = 0;
        if (layout.Total() < 1000000000) {
            info.nMax = static_cast<int>(std::max<std::int64_t>(0, layout.Total() - 1));
            info.nPage = ViewHeight(); info.nPos = static_cast<int>(scroll);
        } else {
            info.nMax = 1000000000;
            info.nPage = std::max(1u, static_cast<unsigned int>(1000000000.0 * ViewHeight() / layout.Total()));
            info.nPos = MaxScroll() ? static_cast<int>((info.nMax - info.nPage + 1.0) * scroll / MaxScroll()) : 0;
        }
        SetScrollInfo(window, SB_VERT, &info, TRUE); Redraw();
        if (request) Render();
        if (selected && worker) SelectionText();
    }
    void Reflow(bool keepAnchor = true, int oldWidth = 0) {
        int anchor = pages ? layout.PageAt(scroll) : 0;
        double fraction = pages && layout.Count() ? (scroll - layout.Top(anchor)) / static_cast<double>(layout.Height(anchor)) : 0;
        int beforeWidth = oldWidth > 0 ? oldWidth : ViewWidth();
        double center = pages && layout.Count() ? (beforeWidth / 2.0 - layout.Left(anchor, beforeWidth, horizontal)) / layout.Width(anchor) : 0.5;
        layout.Rebuild(ViewWidth(), ViewHeight(), zoom, zoomMode, PageGap());
        if (keepAnchor && pages) scroll = layout.Top(anchor) + static_cast<std::int64_t>(fraction * layout.Height(anchor));
        if (keepAnchor && pages) horizontal = static_cast<int>(layout.Left(anchor, ViewWidth()) + center * layout.Width(anchor) - ViewWidth() / 2.0);
        ScrollTo(scroll, false);
        if (pages) zoom = zoomMode == ZoomMode::FitWidth ? 0 : zoomMode == ZoomMode::Manual ? zoom : layout.Scale(page);
    }
    void TrimCache() {
        int first = pages ? layout.PageAt(scroll) : 0, last = pages ? layout.PageAt(scroll + ViewHeight()) : 0;
        while (cacheBytes > CacheLimit && cache.size() > 1) {
            auto victim = cache.end();
            for (auto i = cache.begin(); i != cache.end(); ++i) {
                if (i->first >= first && i->first <= last) continue;
                if (victim == cache.end() || i->second.used < victim->second.used) victim = i;
            }
            // Very short pages can make many pages visible: the cap still applies.
            if (victim == cache.end()) {
                for (auto i = cache.begin(); i != cache.end(); ++i)
                    if (i->first != page && (victim == cache.end() || i->second.used < victim->second.used)) victim = i;
            }
            if (victim == cache.end()) break;
            cacheBytes -= victim->second.image->pixels.size(); cache.erase(victim);
        }
    }
    void Render() {
        KillTimer(window, 1); // An immediate render supersedes a pending resize render.
        if (!pages) return;
        int width = ViewWidth(), height = ViewHeight();
        if (renderWidth != width || renderHeight != height || renderZoom != zoom || renderMode != zoomMode) {
            renderWidth = width; renderHeight = height; renderZoom = zoom; renderMode = zoomMode; ++viewVersion;
        }
        int first = layout.PageAt(scroll), last = layout.PageAt(scroll + ViewHeight());
        std::vector<int> wanted;
        for (int i = first; i <= last; ++i) wanted.push_back(i);
        if (benchmarkReport.empty() || navigationTest) {
            if (last + 1 < pages) wanted.push_back(last + 1);
            if (first > 0) wanted.push_back(first - 1);
        }
        if (wanted == planned && plannedVersion == viewVersion) return;
        planned = wanted; plannedVersion = viewVersion;
        std::vector<int> missing;
        for (int i : wanted) {
            auto found = cache.find(i);
            if (found != cache.end()) found->second.used = ++useClock;
            if (found == cache.end() || found->second.version != viewVersion) missing.push_back(i);
        }
        // Even an empty batch cancels work from a view that is no longer visible.
        revision = worker->Render(std::move(missing), width, height, zoom, zoomMode, document, viewVersion);
        Redraw();
    }
    void ResetDocument(const std::wstring& file) {
        RememberTabView(); openingDocument = false; loadingFailed = false;
        SaveReadingState(); viewReady = false; pendingReadingState.reset();
        outlinePinned = outlinePeek = false; outlineAutoShow = autoShowOutline; KillTimer(window, 7);
        suppressCurrentState = false;
        pageLinks.clear(); pendingLinks.clear(); linkHistory.clear();
        outlineFilterQuery.clear();
        if (outlineFilter) SetWindowTextW(outlineFilter, L"");
        outline = {}; outlineLoaded = outlinePending = false; PopulateOutline();
        EndSelection(); ClearSelection(); textPages.clear(); pendingText.clear(); searchPage = -1; searchBoxes.clear();
        path = file; ++document; pages = 0; canCopy = canPrint = false; page = 0; scroll = 0; horizontal = 0; zoom = 0;
        zoomMode = ZoomMode::FitWidth; searching = false;
        cache.clear(); cacheBytes = 0; layout.Reset(0); layout.Rebuild(ViewWidth(), ViewHeight(), zoom, zoomMode, PageGap());
        planned.clear(); renderWidth = renderHeight = 0; renderZoom = -1; renderMode = ZoomMode::FitWidth; ++viewVersion;
        ScrollHorizontal(0);
        ArrangeChrome();
    }
    void Load(const std::wstring& file, const std::wstring& password = L"", bool retry = false) {
        homeActive = false;
        if (!worker) worker = std::make_unique<PdfWorker>(window);
        if (!retry) ResetDocument(file);
        openingDocument = true; status = L"Abriendo documento…"; ArrangeHome(); Redraw(); worker->Load(path, password, document, retry);
    }
    void OpenDocument(const std::wstring& file) {
        if (file.empty()) return;
        for (int i = 0; i < static_cast<int>(tabs.size()); ++i)
            if (SameTab(tabs[i].path, file)) { ActivateTab(i); return; }
        tabs.push_back({file, {}});
        activeTab = static_cast<int>(tabs.size()) - 1;
        SaveSession(); Load(file);
    }
    void ActivateTab(int index) {
        if (index < 0 || index >= static_cast<int>(tabs.size())) return;
        if (index == activeTab && !homeActive && (pages || openingDocument)) return;
        homeActive = false;
        activeTab = index;
        SaveSession(); Load(tabs[index].path);
    }
    void CloseTab(int index) {
        if (index < 0 || index >= static_cast<int>(tabs.size())) return;
        bool wasActive = index == activeTab;
        tabs.erase(tabs.begin() + index);
        if (tabs.empty()) { activeTab = -1; homeActive = false; SaveSession(); CloseDocument(); return; }
        if (wasActive) {
            int next = std::min(index, static_cast<int>(tabs.size()) - 1);
            activeTab = next;
            SaveSession();
            if (!homeActive) Load(tabs[next].path);
        } else { if (index < activeTab) --activeTab; SaveSession(); }
        ArrangeChrome(); Redraw();
    }
    void NextTab(int delta) {
        if (tabs.empty()) return;
        if (tabs.size() == 1) { ActivateTab(0); return; }
        ActivateTab((activeTab + delta + static_cast<int>(tabs.size())) % static_cast<int>(tabs.size()));
    }
    void CloseDocument() {
        ResetDocument(L"");
        if (worker) worker->Close(document);
        SetWindowTextW(window, L"Chucao PDF Reader");
        status = L"Abrí un PDF para empezar · Ctrl+O";
        RefreshHomeRecents();
        Redraw();
    }
    void ShowHome() {
        if (tabs.empty() || homeActive) return;
        // Keep the document tabs and their saved positions; only unload the
        // current PDF and return to the recents screen represented by the
        // small home tab at the start of the tab strip.
        homeActive = true;
        CloseDocument();
        SaveSession();
        SetFocus(window);
    }
    bool TabsCheck(bool ok) {
        if (tabsTestFailed) return false;
        if (!ok) { tabsTestFailed = true; BenchmarkResult(false); return false; }
        ++tabsChecks; return true;
    }
    bool TabViewMatches(const ReadingState& expected) const {
        auto actual = CurrentReadingState();
        return actual.page == expected.page && actual.zoom == expected.zoom &&
            std::abs(actual.vertical - expected.vertical) < .01 && std::abs(actual.horizontal - expected.horizontal) < .01;
    }
    void SetTabsTestView(ReadingState& expected, double scale) {
        expected = {path, pages, pages - 1, scale, .3, .65, 0};
        pendingReadingState = expected; ApplyReadingState(expected); Render();
    }
    void RunWindowTest() {
        bool ok = true;
        AppSettings saved{};
        saved.hasWindow = true; saved.windowRect = {120, 90, 1020, 710}; saved.maximized = false;
        ok = ok && SettingsStore(settingsFile).Write(saved);
        bool maximized = PrepareWindowPlacement();
        RECT rc{}; GetWindowRect(window, &rc);
        ok = ok && !maximized && std::abs(rc.left - 120) <= 2 && std::abs(rc.top - 90) <= 2 &&
            std::abs((rc.right - rc.left) - 900) <= 4 && std::abs((rc.bottom - rc.top) - 620) <= 4;
        SetWindowPos(window, nullptr, 150, 120, 1000, 750, SWP_NOZORDER | SWP_NOACTIVATE);
        SaveWindowPlacement();
        auto read = SettingsStore(settingsFile).Read();
        ok = ok && read && read->hasWindow && std::abs(read->windowRect.left - 150) <= 2 &&
            std::abs(read->windowRect.top - 120) <= 2 && !read->maximized;
        // A placement on a monitor that no longer exists must be ignored.
        AppSettings offscreen{}; offscreen.hasWindow = true; offscreen.windowRect = {-30000, -30000, -29000, -29000};
        ok = ok && SettingsStore(settingsFile).Write(offscreen);
        RECT before{}; GetWindowRect(window, &before);
        PrepareWindowPlacement();
        RECT after{}; GetWindowRect(window, &after);
        ok = ok && before.left == after.left && before.top == after.top;
        // The maximized flag survives a round trip.
        AppSettings maximizedSettings{}; maximizedSettings.hasWindow = true;
        maximizedSettings.windowRect = {200, 150, 1100, 850}; maximizedSettings.maximized = true;
        ok = ok && SettingsStore(settingsFile).Write(maximizedSettings);
        ok = ok && PrepareWindowPlacement();
        BenchmarkResult(ok);
    }
    void RunTabsTest() {
        tabsTestMissing = tabsTestA.substr(0, tabsTestA.find_last_of(L"\\/") + 1) + L"unavailable.pdf";
        tabsStep = 0; OpenDocument(tabsTestA);
    }
    void CheckTabsImage() {
        if (!tabsTest || tabsTestFailed || !pages || !viewReady || pendingReadingState) return;
        auto bitmap = cache.find(layout.PageAt(scroll));
        if (bitmap == cache.end() || bitmap->second.version != viewVersion) return;
        switch (tabsStep) {
        case 0:
            if (!TabsCheck(tabs.size() == 1 && activeTab == 0 && SameTab(path, tabsTestA) && pages == 2)) return;
            tabsStep = 1; SetTabsTestView(tabsExpectedA, 3); break;
        case 1:
            if (!TabsCheck(TabViewMatches(tabsExpectedA))) return;
            tabsStep = 2; OpenDocument(tabsTestB); break;
        case 2:
            if (!TabsCheck(tabs.size() == 2 && activeTab == 1 && SameTab(path, tabsTestB) && pages == 4)) return;
            // Keep horizontal panning reachable even before the other page sizes are learned.
            tabsStep = 3; SetTabsTestView(tabsExpectedB, 4); break;
        case 3:
            if (!TabsCheck(TabViewMatches(tabsExpectedB))) return;
            tabsStep = 4; ActivateTab(0); break;
        case 4: {
            if (!TabsCheck(TabViewMatches(tabsExpectedA) && tabs.size() == 2 && activeTab == 0)) return;
            int originalPages = pages; auto generation = document;
            for (auto kind : {Result::Opened, Result::Image, Result::Failure, Result::Password, Result::Closed}) {
                auto stale = std::make_unique<Result>(); stale->kind = kind; stale->document = document - 1; stale->pages = 9999;
                Receive(std::move(stale));
            }
            if (!TabsCheck(pages == originalPages && document == generation && tabsPrompts == 0 && tabsClosed == 0)) return;
            OpenDocument(tabsTestA);
            if (!TabsCheck(tabs.size() == 2 && document == generation)) return;
            tabsStep = 5; ActivateTab(1); break;
        }
        case 5: {
            if (!TabsCheck(TabViewMatches(tabsExpectedB))) return;
            auto generation = document; CloseTab(0);
            if (!TabsCheck(tabs.size() == 1 && activeTab == 0 && document == generation && SameTab(path, tabsTestB))) return;
            tabsStep = 6; CloseTab(0); break;
        }
        case 7:
            if (!TabsCheck(tabs.size() == 1 && activeTab == 0 && page == 0 && zoom == 0 && SameTab(path, tabsTestA))) return;
            tabsStep = 8; OpenDocument(tabsTestMissing); break;
        case 10:
            if (!TabsCheck(tabs.size() == 1 && pages == 2 && SameTab(path, tabsTestA))) return;
            tabsStep = 11; OpenDocument(tabsTestProtected); break;
        case 13: {
            if (!TabsCheck(tabs.size() == 2 && activeTab == 1 && pages == 2 && SameTab(path, tabsTestProtected))) return;
            auto generation = document; OpenDocument(tabsTestProtected);
            if (!TabsCheck(document == generation && tabsPrompts == 2)) return;
            SaveTestImage(benchmarkReport + L".tabs.bmp", window);
            tabsStep = 14; ActivateTab(0); break;
        }
        case 14:
            if (!TabsCheck(SameTab(path, tabsTestA) && pages == 2)) return;
            tabsStep = 16; ActivateTab(1); break;
        case 17: {
            if (!TabsCheck(SameTab(path, tabsTestProtected) && pages == 2 && tabsPrompts == 3)) return;
            auto generation = document; CloseTab(0);
            if (!TabsCheck(tabs.size() == 1 && activeTab == 0 && generation == document)) return;
            tabsStep = 15; CloseTab(0); break;
        }
        }
    }
    void CheckTabsFailure() {
        if (!TabsCheck((tabsStep == 8 || tabsStep == 9) && !openingDocument && !pages && tabs.size() == 2 && SameTab(path, tabsTestMissing))) return;
        if (tabsStep == 8) {
            auto generation = document; tabsStep = 9; OpenDocument(tabsTestMissing);
            if (document <= generation || tabs.size() != 2) BenchmarkResult(false);
        } else { tabsStep = 10; CloseTab(activeTab); }
    }
    void CheckTabsPassword(const std::wstring& prompt) {
        if (!TabsCheck((tabsStep == 11 || tabsStep == 12 || tabsStep == 16) && SameTab(path, tabsTestProtected))) return;
        ++tabsPrompts;
        PasswordScript script;
        if (tabsStep == 11) script.action = PasswordScript::Cancel;
        else script.value = L"Chucao-test";
        std::wstring password; EraseSecretOnExit<wchar_t> wipe{password};
        bool accepted = PasswordDialog(window, password, prompt, font, &script);
        if (!TabsCheck(script.verified && accepted == (tabsStep != 11))) return;
        if (!accepted) {
            CancelPasswordOpen();
            if (!TabsCheck(!openingDocument && !pages && tabs.size() == 2)) return;
            tabsStep = 12; OpenDocument(tabsTestProtected);
        } else {
            tabsStep = tabsStep == 12 ? 13 : 17; Load(path, password, true);
        }
    }
    void CheckTabsClosed() {
        if (!TabsCheck((tabsStep == 6 || tabsStep == 15) && tabs.empty() && activeTab == -1 && path.empty() && !pages && cacheBytes == 0)) return;
        ++tabsClosed;
        if (tabsStep == 6) { tabsStep = 7; OpenDocument(tabsTestA); }
        else { tabsStep = 18; BenchmarkResult(tabsClosed == 2 && tabsPrompts == 3); }
    }
    void ChooseFile() {
        wchar_t filename[32768]{};
        OPENFILENAMEW ofn{}; ofn.lStructSize = sizeof(ofn); ofn.hwndOwner = window;
        ofn.lpstrFilter = L"Documentos PDF\0*.pdf\0Todos los archivos\0*.*\0";
        ofn.lpstrFile = filename; ofn.nMaxFile = 32768; ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;
        if (GetOpenFileNameW(&ofn)) OpenDocument(filename);
    }
    void ChangePage(int target) {
        if (!pages) return;
        target = std::max(0, std::min(pages - 1, target));
        page = target; PageStatus(); ScrollTo(std::max<std::int64_t>(0, layout.Top(target) - PageGap()));
    }
    // Parses a 1-based page number from the field. Returns the clamped target in
    // [1, pages], or 0 when the text is empty or not a whole number (cancel).
    static int ParsePageNumber(const std::wstring& text, int pages) {
        if (text.empty() || pages <= 0) return 0;
        long long value = 0;
        for (wchar_t c : text) {
            if (c < L'0' || c > L'9') return 0;
            value = value * 10 + (c - L'0');
            if (value > pages) value = pages;
        }
        return static_cast<int>(std::max(1LL, std::min<long long>(pages, value)));
    }
    RECT PageFieldRect() const { return { D(226), D(11), D(324), D(41) }; }
    bool PageFieldClick(int x, int y) {
        if (!pages) return false;
        RECT field = PageFieldRect();
        if (x < field.left || x >= field.right || y < field.top || y >= field.bottom) return false;
        ShowPageEdit(); return true;
    }
    // The page indicator is drawn by hand; clicking it reveals a numeric EDIT
    // over the field with the current page preselected.
    void ShowPageEdit() {
        if (!pages || !pageEdit) return;
        pageEditActive = true;
        SetWindowTextW(pageEdit, std::to_wstring(page + 1).c_str());
        ShowWindow(pageEdit, SW_SHOW);
        SetFocus(pageEdit);
        SendMessageW(pageEdit, EM_SETSEL, 0, -1);
    }
    void EndPageEdit(bool focusWindow) {
        if (!pageEditActive) return;
        pageEditActive = false;
        if (pageEdit) ShowWindow(pageEdit, SW_HIDE);
        if (focusWindow) SetFocus(window);
        Redraw();
    }
    void CommitPageEdit() {
        if (!pageEditActive) return;
        int target = ParsePageNumber(Text(pageEdit), pages);
        if (target > 0) ChangePage(target - 1);
        EndPageEdit(true);
    }
    void CancelPageEdit(bool focusWindow = true) { EndPageEdit(focusWindow); }
    void Search(bool backwards = false) {
        KillTimer(window, 9);
        if (!pages || searching) return;
        auto value = Text(query); if (value.empty()) { SetFocus(query); return; }
        ClearSelection(); searching = true; status = L"Buscando…"; SetWindowTextW(findFeedback, L"Buscando…");
        Redraw(); worker->Search(value, page, document, backwards);
    }
    void PrintFile() {
        if (!pages || printing) return;
        if (!canPrint) { status = L"El documento no permite imprimir con esta contraseña."; Redraw(); return; }
        PRINTDLGW pd{}; pd.lStructSize = sizeof(pd); pd.hwndOwner = window;
        pd.Flags = PD_RETURNDC | PD_NOSELECTION | PD_USEDEVMODECOPIESANDCOLLATE;
        pd.nMinPage = 1; pd.nMaxPage = static_cast<WORD>(std::min(pages, 65535));
        pd.nFromPage = 1; pd.nToPage = pd.nMaxPage; pd.nCopies = 1;
        if (PrintDlgW(&pd)) {
            int first = pd.Flags & PD_PAGENUMS ? pd.nFromPage - 1 : 0;
            int last = pd.Flags & PD_PAGENUMS ? pd.nToPage - 1 : pages - 1;
            if (first >= 0 && last >= first && last < pages) {
                printing = true; status = L"Preparando impresión…"; Redraw();
                worker->PrintDocument(pd.hDC, first, last, document);
            } else { DeleteDC(pd.hDC); MessageBoxW(window, L"El rango de páginas no es válido.", L"Chucao", MB_OK | MB_ICONWARNING); }
        }
        if (pd.hDevMode) GlobalFree(pd.hDevMode);
        if (pd.hDevNames) GlobalFree(pd.hDevNames);
    }
    // Zooms in or out. With an anchor point (client coordinates) the document
    // point under the anchor stays under the same pixel after the zoom; without
    // one the view keeps the existing top/center anchoring from Reflow, exactly
    // as the +/− buttons and the +/− keys always did.
    void Zoom(bool larger, int anchorX = -1, int anchorY = -1) {
        // Any manual zoom leaves the fit modes and uses the current effective scale.
        if (zoomMode != ZoomMode::Manual) {
            zoom = pages ? layout.Scale(page) : 0;
            zoomMode = ZoomMode::Manual;
        }
        if (!zoom) {
            auto found = cache.find(page);
            zoom = found != cache.end() ? found->second.image->scale : 1.0;
        }
        int anchorPage = -1;
        double fy = 0, fx = 0;
        int cx = 0, cy = 0;
        if (pages && anchorX >= 0) {
            cx = anchorX - ViewLeft();
            cy = anchorY - ContentTop();
            // Outside the document area there is no page under the pointer, so
            // fall back to the default top/center anchoring of Reflow.
            if (cx >= 0 && cx < ViewWidth() && cy >= 0 && cy < ViewHeight()) {
                anchorPage = layout.PageAt(scroll + cy);
                fy = (scroll + cy - layout.Top(anchorPage)) / static_cast<double>(layout.Height(anchorPage));
                fx = (cx - layout.Left(anchorPage, ViewWidth(), horizontal)) / static_cast<double>(layout.Width(anchorPage));
            }
        }
        zoom = std::max(0.25, std::min(4.0, zoom * (larger ? 1.2 : 1 / 1.2)));
        Reflow();
        if (anchorPage >= 0) {
            // Re-anchor: keep the same page and fractions under the pointer.
            ScrollTo(layout.Top(anchorPage) + static_cast<std::int64_t>(fy * layout.Height(anchorPage)) - cy, false);
            ScrollHorizontal(static_cast<int>(layout.Left(anchorPage, ViewWidth()) + fx * layout.Width(anchorPage) - cx));
        }
        Render();
    }
    void Command(int id) {
        if (id == About) {
            // DialogBoxParam returns -1 when the template cannot be created (for
            // example, a missing Common Controls v6 activation context). Surface
            // that instead of leaving the user with a button that does nothing.
            if (DialogBoxParamW(GetModuleHandleW(nullptr), MAKEINTRESOURCEW(IDD_ABOUT), window, AboutProc, 0) == -1)
                MessageBoxW(window, L"Chucao PDF Reader " CHUCAO_WIDE(CHUCAO_VERSION_STRING)
                    L"\n\nNo se pudo abrir el cuadro Acerca de.\n" CHUCAO_WIDE(CHUCAO_REPOSITORY),
                    L"Acerca de Chucao", MB_OK | MB_ICONINFORMATION);
            return;
        }
        if (id == OpenMenu) { ChooseFile(); return; }
        if (id == ForgetHistory) { ClearReadingHistory(); return; }
        pendingReadingState.reset();
        ++linkEpoch;
        if (id >= RecentBase && id < RecentBase + static_cast<int>(recentEntries.size())) {
            auto selectedPath = recentEntries[id - RecentBase].path; SetFocus(window); OpenDocument(selectedPath); return;
        }
        switch (id) {
        case Open: ChooseFile(); break;
        case Previous: ChangePage(page - 1); break;
        case Next: ChangePage(page + 1); break;
        case Fit: zoomMode = ZoomMode::FitWidth; zoom = 0; Reflow(); Render(); break;
        case FitPageCmd: zoomMode = ZoomMode::FitPage; zoom = 0; Reflow(); Render(); break;
        case FitHeightCmd: zoomMode = ZoomMode::FitHeight; zoom = 0; Reflow(); Render(); break;
        case Larger: case Smaller: Zoom(id == Larger); break;
        case Find: ShowSearch(); break;
        case FindNext: ShowSearch(false); Search(); break;
        case FindPrevious: ShowSearch(false); Search(true); break;
        case FindClose: HideSearch(); break;
        case Copy: CopySelection(); break;
        case Outline: case OutlineRail: ToggleOutline(); break;
        case OutlineClose: SetOutlinePinned(false); SetFocus(window); break;
        case OutlinePin:
            if (outlinePinned) { SetOutlinePinned(false); PeekOutline(); }
            else SetOutlinePinned(true);
            break;
        case Print: PrintFile(); break;
        case Theme:
            if (DialogBoxParamW(GetModuleHandleW(nullptr), MAKEINTRESOURCEW(IDD_SETTINGS), window, SettingsProc, reinterpret_cast<LPARAM>(this)) == -1)
                MessageBoxW(window, L"No se pudieron abrir las opciones. Reiniciá Chucao e intentá de nuevo.",
                    L"Chucao", MB_OK | MB_ICONWARNING);
            else SetFocus(window);
            break;
        case NewTabCmd: ChooseFile(); break;
        case CloseTabCmd:
            // The home view behaves like a closable tab: Ctrl+W returns to the
            // last document instead of deleting a document tab.
            if (homeActive) {
                if (activeTab >= 0 && activeTab < static_cast<int>(tabs.size())) ActivateTab(activeTab);
            } else CloseTab(activeTab);
            break;
        case NextTabCmd: NextTab(1); break;
        case PrevTabCmd: NextTab(-1); break;
        }
    }
    void Receive(std::unique_ptr<Result> result) {
        if (result->kind == Result::Printed) printing = false;
        if (result->document != document) return;
        if (result->kind == Result::Failure && result->revision && result->revision != revision) return;
        switch (result->kind) {
        case Result::Opened:
            openingDocument = false;
            canCopy = result->copyAllowed; canPrint = result->printAllowed;
            openedMs = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - started).count();
            pages = result->pages; layout.Reset(pages); ArrangeHome();
            if (rapidTest) rapidPages = result->pages;
            pendingReadingState = SessionTabView();
            if (!pendingReadingState && ConfigureReadingState()) pendingReadingState = ReadingStateStore(readingStateFile).Find(path, pages);
            readingStateFound = pendingReadingState.has_value();
            if (pendingReadingState) ApplyReadingState(*pendingReadingState);
            else { layout.Rebuild(ViewWidth(), ViewHeight(), zoom, zoomMode, PageGap()); ScrollTo(0, false); }
            SetWindowTextW(window, (L"Chucao — " + path.substr(path.find_last_of(L"\\/") + 1)).c_str()); PageStatus();
            // Resolve the outline before requesting the first page. The worker is
            // single-threaded, so queueing LoadOutline first guarantees the panel
            // is pinned before the first bitmap is rendered; for documents with
            // bookmarks the index is visible from the start instead of appearing
            // after page 1. With auto-show disabled the page can render at once.
            if (!outlineAutoShow) Render();
            RequestOutline();
            if (outlineAutoShow && !outlinePending) Render();
            break;
        case Result::OutlineReady:
            outlinePending = false; outlineLoaded = true; outline = std::move(result->outline); PopulateOutline();
            if (outlineAutoShow) { outlinePinned = !outline.entries.empty(); outlinePeek = false; }
            ArrangeChrome(); Render();
            if (outlineFilterTest) CheckOutlineFilter();
            else if (interfaceTest) CheckInterface();
            else if (outlineTest) CheckOutline();
            break;
        case Result::LinksReady: {
            pendingLinks.erase(result->page); pageLinks[result->page] = std::move(result->links);
            if (pageLinks.size() > 16) {
                auto victim = pageLinks.begin();
                for (auto i = pageLinks.begin(); i != pageLinks.end(); ++i) if (std::abs(i->first - page) > std::abs(victim->first - page)) victim = i;
                pageLinks.erase(victim);
            }
            POINT screen{}; GetCursorPos(&screen); POINT pointer = screen; ScreenToClient(window, &pointer); TextPoint point;
            // Update only our cursor, without activating or moving any window.
            if (!dragging && WindowFromPoint(screen) == window && PointOnPage(pointer.x, pointer.y, point))
                SetCursor(LoadCursor(nullptr, LinkAt(point) ? IDC_HAND : IDC_IBEAM));
            CheckLinks(Result::LinksReady);
            break;
        }
        case Result::OutlineFollowed:
        case Result::LinkFollowed:
            if (result->revision != linkEpoch || selected || dragging) break;
            if (result->linkTarget.page >= 0) {
                int anchor = layout.PageAt(scroll);
                if (linkHistory.size() == 64) linkHistory.erase(linkHistory.begin());
                linkHistory.push_back({anchor, (scroll - layout.Top(anchor)) / static_cast<double>(layout.Height(anchor)),
                    (ViewWidth() / 2.0 - layout.Left(anchor, ViewWidth(), horizontal)) / layout.Width(anchor), zoom});
                auto target = result->linkTarget;
                ClearSelection(); layout.Learn(target.page, target.width, target.height);
                // A chapter in the outline is a navigation shortcut, not a request
                // to change the reading scale: keep the user's zoom and fit mode.
                // Internal page links still honor the destination's view/zoom.
                if (result->kind == Result::LinkFollowed) {
                    zoom = DestinationZoom(target, ViewWidth(), ViewHeight(), zoom, D(48));
                    zoomMode = ZoomMode::Manual;
                }
                Reflow();
                if (!target.preserveY)
                    ScrollTo(layout.Top(target.page) + static_cast<std::int64_t>(target.y * layout.Height(target.page)) - PageGap());
                // Whole-width modes center the page; rectangles and vertical fits anchor on their left edge.
                bool centered = target.view == PDFDEST_VIEW_FIT || target.view == PDFDEST_VIEW_FITB || target.view == PDFDEST_VIEW_FITBH;
                if (!target.preserveX)
                    ScrollHorizontal(static_cast<int>(layout.Left(target.page, ViewWidth()) +
                        (centered ? layout.Width(target.page) / 2.0 - ViewWidth() / 2.0 : target.x * layout.Width(target.page) - PageGap())));
                PageStatus();
            } else if (!result->linkTarget.webUrl.empty()) OpenWebLink(result->linkTarget.webUrl);
            else if (result->linkTarget.hit) {
                status = result->kind == Result::OutlineFollowed ? L"No se pudo acceder al destino de esta entrada del índice." :
                    L"Este enlace no es compatible. Se admiten destinos internos y direcciones HTTP/HTTPS.";
                Redraw();
            }
            if (result->kind == Result::OutlineFollowed) CheckOutlineNavigation(result->linkTarget);
            else CheckLinks(Result::LinkFollowed, result->linkTarget);
            break;
        case Result::Image:
            // A completed bitmap from the same zoom/width is useful even if the
            // scroll position has changed since it was queued.
            if (result->viewVersion != viewVersion) return;
            {
                int index = result->page;
                double renderMilliseconds = result->milliseconds;
                layout.Learn(index, result->pageWidth, result->pageHeight);
                if (pendingReadingState) {
                    ApplyReadingState(*pendingReadingState);
                    if (index == pendingReadingState->page) pendingReadingState.reset();
                } else Reflow();
                auto& entry = cache[index];
                if (entry.image) cacheBytes -= entry.image->pixels.size();
                cacheBytes += result->pixels.size(); entry = {std::move(result), viewVersion, ++useClock};
                if (!pendingReadingState) viewReady = true;
                ScrollTo(scroll, false); TrimCache(); Render();
                if (startupTest && startupStage == 0) {
                    startupStage = 1; startupPage = index; startupRenderMs = renderMilliseconds;
                    startupBitmapMs = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - started).count();
                    ComposeFrame();
                    startupComposeMs = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - started).count();
                    BenchmarkResult(true);
                }
                else if (rapidTest) {
                    // A late result from the first document must not replace the second.
                    int before = pages;
                    auto stale = std::make_unique<Result>(); stale->kind = Result::Opened;
                    stale->document = document - 1; stale->pages = 9999;
                    Receive(std::move(stale));
                    BenchmarkResult(pages == before && pages == rapidPages && rapidPages > 0);
                }
                else if (sessionTest) CheckSessionImage();
                else if (tabsTest) CheckTabsImage();
                else if (fitTest) CheckFit();
                else if (textTest) CheckText(Result::Image);
                else if (navigationTest) { PostMessageW(window, WM_TIMER, 3, 0); }
                else if (interfaceTest) { /* Wait for the outline and search diagnostics. */ }
                else if (outlineTest) { if (!outlineWindow) ToggleOutline(); }
                else if (linksTest) CheckLinks(Result::Image);
                else if (passwordTest) CheckPasswordImage();
                else if (readingStateTest) CheckReadingState();
                else if (recentTest) CheckRecents();
                else BenchmarkResult(true);
            }
            break;
        case Result::Password: {
            if (tabsTest) { CheckTabsPassword(result->message); break; }
            if (readingStateTest) {
                if (++stateAuthenticationAttempts > 1) { BenchmarkResult(false); break; }
                PasswordScript script; script.value = L"Chucao-test";
                std::wstring password; EraseSecretOnExit<wchar_t> wipe{password};
                if (!PasswordDialog(window, password, result->message, font, &script) || !script.verified) { BenchmarkResult(false); break; }
                ++readingStateChecks; Load(path, password, true); break;
            }
            if (passwordTest) { TestPasswordPrompt(result->message); break; }
            if (!benchmarkReport.empty()) { BenchmarkResult(false); break; }
            std::wstring password;
            if (PasswordDialog(window, password, result->message, font)) Load(path, password, true);
            else CancelPasswordOpen();
            if (!password.empty()) SecureZeroMemory(&password[0], password.size() * sizeof(wchar_t));
            break;
        }
        case Result::Search:
            searching = false;
            if (!searchVisible && !textTest) break;
            if (result->text != Text(query)) break;
            searchBoxes.clear(); searchPage = -1;
            if (result->match >= 0) {
                layout.Learn(result->page, result->pageWidth, result->pageHeight); Reflow();
                searchPage = result->page; searchMatch = result->match; searchBoxes = std::move(result->boxes);
                double top = searchBoxes.empty() ? 0 : searchBoxes.front().top;
                ScrollTo(layout.Top(searchPage) + static_cast<std::int64_t>(top * layout.Height(searchPage)) - ViewHeight() / 3);
                if (!searchBoxes.empty()) ScrollHorizontal(static_cast<int>(layout.Left(searchPage, ViewWidth()) + searchBoxes.front().left * layout.Width(searchPage) - ViewWidth() / 3.0));
                status = L"Coincidencia en página " + std::to_wstring(result->page + 1) + L". F3: siguiente.";
                auto feedback = L"Página " + std::to_wstring(result->page + 1);
                SetWindowTextW(findFeedback, feedback.c_str());
            } else { status = L"No se encontró el texto."; SetWindowTextW(findFeedback, L"Sin resultados"); }
            Redraw(); CheckText(Result::Search); if (interfaceTest) CheckInterface(true); break;
        case Result::Geometry: {
            pendingText.erase(result->page);
            textPages[result->page] = std::move(result->boxes);
            size_t bytes = 0; for (const auto& item : textPages) bytes += item.second.size() * sizeof(TextBox);
            while (bytes > 16u * 1024u * 1024u) {
                auto victim = textPages.end(); int distance = -1;
                for (auto i = textPages.begin(); i != textPages.end(); ++i) {
                    if (i->first == selectionStart.page || i->first == selectionEnd.page) continue;
                    if (std::abs(i->first - page) > distance) { victim = i; distance = std::abs(i->first - page); }
                }
                if (victim == textPages.end()) break;
                bytes -= victim->second.size() * sizeof(TextBox); textPages.erase(victim);
            }
            if (!result->message.empty()) status = result->message;
            if (dragging || selected) { NeedText(selectionStart.page); NeedText(selectionEnd.page); }
            if (copyWhenReady) CopySelection();
            Redraw(); CheckText(Result::Geometry); break;
        }
        case Result::Copied:
            if (result->revision != selectionRevision) break;
            if (passwordTest && passwordStage == 9) {
                if (!result->text.empty() || result->message != L"El documento no permite copiar texto con esta contraseña.") { BenchmarkResult(false); break; }
                ++passwordChecks;
                PrintFile();
                if (printing || status != L"El documento no permite imprimir con esta contraseña.") { BenchmarkResult(false); break; }
                ++passwordChecks; passwordStage = 10; worker->PrintDocument(nullptr, 0, 0, document); break;
            }
            if (textTest) { if (!result->message.empty()) BenchmarkResult(false); else CheckText(Result::Copied, result->text); break; }
            if (!result->message.empty()) { status = result->message; Redraw(); }
            else SetClipboardText(result->text);
            break;
        case Result::Printed:
            status = result->message; Redraw();
            if (passwordTest && passwordStage == 10) {
                if (result->message != L"El documento no permite imprimir con esta contraseña.") BenchmarkResult(false);
                else { ++passwordChecks; passwordStage = 11; BenchmarkResult(true); }
            }
            break;
        case Result::Failure: {
            openingDocument = false; loadingFailed = true;
            ArrangeHome();
            auto slash = path.find_last_of(L"\\/");
            auto name = path.substr(slash == std::wstring::npos ? 0 : slash + 1);
            status = name.empty() ? result->message : name + L": " + result->message;
            if (!pages) status += TabRetryHint();
            Redraw();
            if (tabsTest) CheckTabsFailure();
            else if (recentTest && recentStep == 2 && path == recentMissing) {
                RefreshHomeRecents();
                bool retained = false;
                for (const auto& entry : recentEntries) if (entry.path == recentMissing) retained = true;

                if (!retained) { BenchmarkResult(false); break; }
                ++recentChecks; recentStep = 3; OpenRecentForTest(recentOther);
            } else BenchmarkResult(false);
            break;
        }
        case Result::Closed:
            if (tabsTest) CheckTabsClosed();
            break;
        }
    }
    void ComputeTabs(const RECT& rc) {
        tabLayout.clear();
        homeTabBounds = {};
        if (tabs.empty()) return;
        int height = TabBarHeight(), x = D(8), gap = D(4), closeW = D(26);
        int homeW = D(30);
        int count = static_cast<int>(tabs.size());
        // Reserve the last slot for the home/mini tab; a PDF opened from the
        // recents screen becomes the last document tab, just before it.
        int tabW = std::max(D(96), std::min(D(220),
            (static_cast<int>(rc.right) - D(8) - homeW - D(8) - gap * count) / count));
        for (int i = 0; i < count; ++i) {
            TabLayout t;
            t.bounds = {x, BarPixels() + D(3), x + tabW, BarPixels() + height - D(2)};
            t.close = {t.bounds.right - closeW, t.bounds.top + D(3), t.bounds.right - D(6), t.bounds.bottom - D(3)};
            tabLayout.push_back(t);
            x += tabW + gap;
        }
        homeTabBounds = {x, BarPixels() + D(3), x + homeW, BarPixels() + height - D(2)};
    }
    bool HomeTabHitTest(int x, int y) {
        if (tabs.empty()) return false;
        RECT rc{}; GetClientRect(window, &rc); ComputeTabs(rc);
        return x >= homeTabBounds.left && x < homeTabBounds.right &&
            y >= homeTabBounds.top && y < homeTabBounds.bottom;
    }
    int TabHitTest(int x, int y, bool& close) {
        close = false;
        if (tabs.empty()) return -1;
        RECT rc{}; GetClientRect(window, &rc);
        ComputeTabs(rc);
        for (size_t i = 0; i < tabLayout.size(); ++i) {
            const auto& t = tabLayout[i];
            if (x >= t.bounds.left && x < t.bounds.right && y >= t.bounds.top && y < t.bounds.bottom) {
                close = x >= t.close.left;
                return static_cast<int>(i);
            }
        }
        return -1;
    }
    bool TabClick(int x, int y) {
        if (HomeTabHitTest(x, y)) { ShowHome(); return true; }
        if (tabs.empty()) return false;
        bool close = false;
        int index = TabHitTest(x, y, close);
        if (index < 0) return false;
        if (close) CloseTab(index);
        else ActivateTab(index);
        return true;
    }
    void DrawTabs(HDC dc, const RECT& rc) {
        if (tabs.empty()) return;
        ComputeTabs(rc);
        SetBkMode(dc, TRANSPARENT);
        // Small home/new-view tab, always at the start of the strip so it stays
        // reachable no matter how many documents are open.
        COLORREF homeFill = homeActive ? UiTheme().canvas : hoverHomeTab ? UiTheme().hover : UiTheme().bar;
        UiRoundRect(dc, homeTabBounds, homeFill, UiTheme().line, D(8));
        if (homeActive) {
            RECT underline{homeTabBounds.left + D(6), homeTabBounds.bottom - D(2), homeTabBounds.right - D(6), homeTabBounds.bottom - D(1)};
            SetDCBrushColor(dc, UiTheme().accent);
            FillRect(dc, &underline, reinterpret_cast<HBRUSH>(GetStockObject(DC_BRUSH)));
        }
        SelectObject(dc, strongFont); SetTextColor(dc, homeActive ? UiTheme().ink : UiTheme().muted);
        RECT plus = homeTabBounds;
        DrawTextW(dc, L"+", -1, &plus, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
        SelectObject(dc, font);
        for (size_t i = 0; i < tabLayout.size(); ++i) {
            const auto& t = tabLayout[i];
            bool active = !homeActive && static_cast<int>(i) == activeTab;
            bool hovered = static_cast<int>(i) == hoverTab;
            COLORREF fill = active ? UiTheme().canvas : hovered ? UiTheme().hover : UiTheme().bar;
            UiRoundRect(dc, t.bounds, fill, UiTheme().line, D(8));
            if (active) {
                RECT underline{t.bounds.left + D(6), t.bounds.bottom - D(2), t.bounds.right - D(6), t.bounds.bottom - D(1)};
                SetDCBrushColor(dc, UiTheme().accent);
                FillRect(dc, &underline, reinterpret_cast<HBRUSH>(GetStockObject(DC_BRUSH)));
            }
            SetTextColor(dc, active ? UiTheme().ink : UiTheme().muted);
            RECT text{t.bounds.left + D(10), t.bounds.top, t.close.left - D(4), t.bounds.bottom};
            auto title = TabTitle(tabs[i].path);
            DrawTextW(dc, title.c_str(), -1, &text, DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS | DT_NOPREFIX);
            // Small close cross with GDI+ so it stays crisp at every DPI.
            Gdiplus::Graphics graphics(dc);
            if (graphics.GetLastStatus() == Gdiplus::Ok) {
                graphics.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
                float scale = static_cast<float>(dpi) / 96.0f;
                Gdiplus::Pen pen(UiDraw::Color(hovered && hoverTabClose ? UiTheme().accent : UiTheme().muted),
                    std::max(1.4f, 1.6f * scale));
                pen.SetLineCap(Gdiplus::LineCapRound, Gdiplus::LineCapRound, Gdiplus::DashCapRound);
                float cx = (t.close.left + t.close.right) / 2.0f, cy = (t.close.top + t.close.bottom) / 2.0f;
                float r = std::min(4.0f * scale, (t.close.right - t.close.left) / 2.0f);
                graphics.DrawLine(&pen, cx - r, cy - r, cx + r, cy + r);
                graphics.DrawLine(&pen, cx - r, cy + r, cx + r, cy - r);
            }
        }
    }
    RECT LoadingCardRect() const {
        int width = ViewWidth(), height = ViewHeight();
        int cardWidth = std::min(D(400), std::max(D(220), width - D(48)));
        int cardHeight = D(154);
        int cx = ViewLeft() + width / 2, cy = ContentTop() + height / 2;
        return {cx - cardWidth / 2, cy - cardHeight / 2, cx + cardWidth / 2, cy + cardHeight / 2};
    }
    void DrawLoading(HDC dc) {
        int width = ViewWidth(), height = ViewHeight();
        if (width < D(160) || height < D(100)) return;
        RECT card = LoadingCardRect();
        int cx = (card.left + card.right) / 2;
        UiRoundRect(dc, card, UiTheme().bar, UiTheme().line, D(16));
        float scale = static_cast<float>(dpi) / 96.0f;
        float size = static_cast<float>(D(34));
        float sx = cx - size / 2, sy = static_cast<float>(card.top + D(22));
        Gdiplus::Graphics graphics(dc);
        if (graphics.GetLastStatus() == Gdiplus::Ok) {
            graphics.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
            float penWidth = std::max(2.0f, 3.0f * scale);
            Gdiplus::Pen track(UiDraw::Color(UiTheme().line), penWidth);
            graphics.DrawEllipse(&track, sx, sy, size, size);
            Gdiplus::Pen arc(UiDraw::Color(UiTheme().accent), penWidth);
            arc.SetStartCap(Gdiplus::LineCapRound);
            arc.SetEndCap(Gdiplus::LineCapRound);
            graphics.DrawArc(&arc, sx, sy, size, size, static_cast<float>(spinAngle), 110.0f);
        }
        SetBkMode(dc, TRANSPARENT);
        SelectObject(dc, strongFont); SetTextColor(dc, UiTheme().ink);
        auto title = pages > 0 ? L"Cargando la primera página…" : L"Abriendo documento…";
        RECT titleRect{card.left + D(18), card.top + D(66), card.right - D(18), card.top + D(92)};
        DrawTextW(dc, title, -1, &titleRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
        SelectObject(dc, font); SetTextColor(dc, UiTheme().muted);
        auto name = TabTitle(path);
        RECT nameRect{card.left + D(18), card.top + D(94), card.right - D(18), card.top + D(122)};
        DrawTextW(dc, name.c_str(), -1, &nameRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
    }
    void Draw(HDC target, RECT rc, RECT dirty) {
        int surfaceWidth = std::max<LONG>(1, rc.right), surfaceHeight = std::max<LONG>(1, rc.bottom);
        if (!backBitmap || backWidth != surfaceWidth || backHeight != surfaceHeight) {
            if (backDC) { DeleteDC(backDC); backDC = nullptr; }
            if (backBitmap) { DeleteObject(backBitmap); backBitmap = nullptr; }
            backDC = CreateCompatibleDC(target);
            if (!backDC) return;
            backBitmap = CreateCompatibleBitmap(target, surfaceWidth, surfaceHeight);
            backWidth = surfaceWidth; backHeight = surfaceHeight;
            dirty = rc;
            if (backBitmap) SelectObject(backDC, backBitmap);
        }
        if (!backBitmap) return;
        HDC dc = backDC;
        int savedClip = SaveDC(dc);
        IntersectClipRect(dc, dirty.left, dirty.top, dirty.right, dirty.bottom);
        HBRUSH background = CreateSolidBrush(UiTheme().canvas); FillRect(dc, &rc, background); DeleteObject(background);
        SetDCBrushColor(dc, UiTheme().bar);
        RECT bar{0, 0, rc.right, BarPixels()}; FillRect(dc, &bar, reinterpret_cast<HBRUSH>(GetStockObject(DC_BRUSH)));
        SelectObject(dc, font); SetBkMode(dc, TRANSPARENT); SetTextColor(dc, UiTheme().muted);
        bool loading = IsLoading();
        if (pages) {
            IntersectClipRect(dc, ViewLeft(), ContentTop(), rc.right, ContentTop() + ViewHeight());
            int first = layout.PageAt(scroll), last = layout.PageAt(scroll + ViewHeight());
            SetStretchBltMode(dc, COLORONCOLOR); // Quick preview while a new zoom is rendered.
            for (int i = first; i <= last; ++i) {
                int width = layout.Width(i), height = layout.Height(i);
                int x = ViewLeft() + layout.Left(i, ViewWidth(), horizontal);
                int y = ContentTop() + static_cast<int>(layout.Top(i) - scroll);
                RECT sheet{x, y, x + width, y + height};
                RECT shadow{x + D(2), y + D(3), x + width + D(2), y + height + D(3)};
                SetDCBrushColor(dc, UiTheme().shadow); FillRect(dc, &shadow, reinterpret_cast<HBRUSH>(GetStockObject(DC_BRUSH)));
                SetDCBrushColor(dc, UiTheme().sheet); FillRect(dc, &sheet, reinterpret_cast<HBRUSH>(GetStockObject(DC_BRUSH)));
                auto found = cache.find(i);
                if (found != cache.end()) {
                    const auto& image = found->second.image;
                    BITMAPINFO info{}; info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
                    info.bmiHeader.biWidth = image->width; info.bmiHeader.biHeight = -image->height;
                    info.bmiHeader.biPlanes = 1; info.bmiHeader.biBitCount = 32; info.bmiHeader.biCompression = BI_RGB;
                    StretchDIBits(dc, x, y, width, height, 0, 0, image->width, image->height, image->pixels.data(), &info, DIB_RGB_COLORS, SRCCOPY);
                    Endpoint selectionFirst, selectionLast;
                    auto text = textPages.find(i);
                    bool selectingHere = text != textPages.end() && SelectionRange(selectionFirst, selectionLast) && i >= selectionFirst.page && i <= selectionLast.page;
                    if (i == searchPage && !selectingHere) Highlight(dc, x, y, width, height, searchBoxes, RGB(255, 193, 24));
                    if (selectingHere) {
                        int firstChar = i == selectionFirst.page ? selectionFirst.character : 0;
                        int lastChar = i == selectionLast.page ? selectionLast.character : static_cast<int>(text->second.size()) - 1;
                        Highlight(dc, x, y, width, height, SelectionBoxes(text->second, firstChar, lastChar), RGB(65, 130, 245));
                    }
                } else {
                    auto label = loadingFailed ? L"No se pudo cargar la página" :
                        L"Página " + std::to_wstring(i + 1) + L" · Cargando…";
                    sheet.top += D(24); DrawTextW(dc, label.c_str(), -1, &sheet, DT_CENTER | DT_TOP | DT_SINGLELINE);
                }
            }
            SelectClipRgn(dc, nullptr);
        } else if (loading) {
            RECT empty{0, ContentTop(), rc.right, rc.bottom - D(26)};
            SetDCBrushColor(dc, UiTheme().bar); FillRect(dc, &empty, reinterpret_cast<HBRUSH>(GetStockObject(DC_BRUSH)));
        } else if (!pages) {
            RECT empty{0, ContentTop(), rc.right, rc.bottom - D(26)};
            SetDCBrushColor(dc, UiTheme().bar); FillRect(dc, &empty, reinterpret_cast<HBRUSH>(GetStockObject(DC_BRUSH)));
            int size = HomeImageSize(), top = ContentTop() + D(16);
            if (!welcomeLogo) welcomeLogo = LoadPngResource(IDB_CHUCAO_LOGO);
            if (welcomeLogo) {
                Gdiplus::Graphics graphics(dc);
                if (graphics.GetLastStatus() == Gdiplus::Ok) {
                    // The source PNG has an opaque white margin around the dark
                    // rounded card. Crop to the card and clip its rounded shape so
                    // no white square survives the dark theme.
                    float x = static_cast<float>((rc.right - size) / 2);
                    Gdiplus::GraphicsPath clip;
                    UiDraw::RoundedRectPath(clip, x, static_cast<float>(top), static_cast<float>(size),
                        static_cast<float>(size), static_cast<float>(size) * 0.13f);
                    graphics.SetClip(&clip);
                    graphics.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);
                    graphics.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
                    // The card spans pixels 19..236 of the 256 px PNG and pixel 237 is
                    // its light antialiased edge, so crop strictly inside the card
                    // (20..235) to avoid a light line on the right and bottom.
                    graphics.DrawImage(welcomeLogo, Gdiplus::RectF(x, static_cast<float>(top),
                        static_cast<float>(size), static_cast<float>(size)), 20, 20, 216, 216, Gdiplus::UnitPixel);
                    graphics.ResetClip();
                }
            } else {
                if (!welcomeImage) welcomeImage = static_cast<HBITMAP>(LoadImageW(GetModuleHandleW(nullptr), MAKEINTRESOURCEW(IDB_CHUCAO_HOME), IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION));
                if (welcomeImage) {
                    HDC picture = CreateCompatibleDC(dc); auto oldPicture = SelectObject(picture, welcomeImage);
                    BITMAP bitmap{}; GetObjectW(welcomeImage, sizeof(bitmap), &bitmap);
                    SetStretchBltMode(dc, HALFTONE); SetBrushOrgEx(dc, 0, 0, nullptr);
                    StretchBlt(dc, (rc.right - size) / 2, top, size, size, picture, 0, 0, bitmap.bmWidth, bitmap.bmHeight, SRCCOPY);
                    SelectObject(picture, oldPicture); DeleteDC(picture);
                }
            }
            RECT title{D(24), top + size + D(6), rc.right - D(24), top + size + D(42)};
            SelectObject(dc, welcomeFont); SetTextColor(dc, UiTheme().ink);
            DrawTextW(dc, L"Chucao", -1, &title, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
            int width = std::min(D(660), std::max(D(100), static_cast<int>(rc.right) - D(48)));
            RECT heading{(rc.right - width) / 2 + D(16), HomeListTop() - D(30), (rc.right + width) / 2, HomeListTop()};
            SelectObject(dc, strongFont); SetTextColor(dc, UiTheme().muted);
            DrawTextW(dc, L"Recientes", -1, &heading, DT_SINGLELINE | DT_VCENTER);
            if (recentEntries.empty()) {
                RECT hint{heading.left, HomeListTop() + D(12), heading.right, HomeListTop() + D(48)};
                SelectObject(dc, font);
                DrawTextW(dc, L"Todavía no hay PDF recientes. Abrí uno con la carpeta o Ctrl+O.", -1, &hint, DT_WORDBREAK);
            }
        }
        if (loading) DrawLoading(dc);
        HPEN separator = CreatePen(PS_SOLID, 1, UiTheme().line); auto oldPen = SelectObject(dc, separator);
        MoveToEx(dc, 0, BarPixels() - 1, nullptr); LineTo(dc, rc.right, BarPixels() - 1);
        SelectObject(dc, oldPen); DeleteObject(separator);
        DrawTabs(dc, rc);
        SelectObject(dc, font); SetTextColor(dc, UiTheme().ink);
        for (int x : {D(60), D(164), D(384), D(600)}) {
            RECT rule{x, D(17), x + std::max(1, D(1)), D(35)}; SetDCBrushColor(dc, UiTheme().line);
            FillRect(dc, &rule, reinterpret_cast<HBRUSH>(GetStockObject(DC_BRUSH)));
        }
        // About sits on the far right; give it its own separator before the field.
        RECT aboutRule{rc.right - D(62), D(17), rc.right - D(62) + std::max(1, D(1)), D(35)};
        SetDCBrushColor(dc, UiTheme().line);
        FillRect(dc, &aboutRule, reinterpret_cast<HBRUSH>(GetStockObject(DC_BRUSH)));
        RECT pageField = PageFieldRect(); UiRoundRect(dc, pageField, UiTheme().field, UiTheme().field, D(10));
        auto pageLabel = pages ? std::to_wstring(page + 1) + L" / " + std::to_wstring(pages) : L"— / —";
        SelectObject(dc, strongFont); DrawTextW(dc, pageLabel.c_str(), -1, &pageField, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
        RECT zoomField{D(440), 0, D(500), BarPixels()};
        auto zoomLabel = zoomMode == ZoomMode::FitWidth ? std::wstring(L"Ancho") :
            zoomMode == ZoomMode::FitPage ? std::wstring(L"Página") :
            zoomMode == ZoomMode::FitHeight ? std::wstring(L"Alto") :
            zoom > 0 ? std::to_wstring(static_cast<int>(std::round(zoom * 100))) + L"%" : std::wstring(L"Ancho");
        SelectObject(dc, font); SetTextColor(dc, UiTheme().muted);
        DrawTextW(dc, zoomLabel.c_str(), -1, &zoomField, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        if (searchVisible) {
            RECT findBar{ViewLeft(), ContentTop() + ViewHeight(), rc.right, rc.bottom - D(26)};
            SetDCBrushColor(dc, UiTheme().bar);
            FillRect(dc, &findBar, reinterpret_cast<HBRUSH>(GetStockObject(DC_BRUSH)));
            int width = std::max(D(120), std::min(D(320), static_cast<int>(rc.right) - ViewLeft() - D(320)));
            RECT field{ViewLeft() + D(12), findBar.top + D(6), ViewLeft() + D(60) + width, findBar.top + D(38)};
            UiRoundRect(dc, field, UiTheme().field, GetFocus() == query ? UiTheme().accent : UiTheme().line, D(10));
            {
                Gdiplus::Graphics graphics(dc);
                graphics.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
                Gdiplus::Pen iconPen(UiDraw::Color(UiTheme().muted), static_cast<float>(std::max(1, D(2))));
                iconPen.SetLineCap(Gdiplus::LineCapRound, Gdiplus::LineCapRound, Gdiplus::DashCapRound);
                float x = static_cast<float>(field.left + D(12)), y = static_cast<float>(field.top + D(8));
                graphics.DrawEllipse(&iconPen, x, y, static_cast<float>(D(12)), static_cast<float>(D(12)));
                graphics.DrawLine(&iconPen, x + D(10), y + D(10), x + D(17), y + D(17));
            }
            RECT rule{findBar.left, findBar.top, findBar.right, findBar.top + std::max(1, D(1))};
            SetDCBrushColor(dc, UiTheme().line); FillRect(dc, &rule, reinterpret_cast<HBRUSH>(GetStockObject(DC_BRUSH)));
        }
        RECT footerBar{0, rc.bottom - D(26), rc.right, rc.bottom};
        SetDCBrushColor(dc, UiTheme().bar); FillRect(dc, &footerBar, reinterpret_cast<HBRUSH>(GetStockObject(DC_BRUSH)));
        RECT footer{D(16), rc.bottom - D(24), rc.right - D(16), rc.bottom};
        SelectObject(dc, font); SetTextColor(dc, UiTheme().muted);
        if (!statusBar) DrawTextW(dc, status.c_str(), -1, &footer, DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS);
        RestoreDC(dc, savedClip);
        if (dirty.right > dirty.left && dirty.bottom > dirty.top)
            BitBlt(target, dirty.left, dirty.top, dirty.right - dirty.left, dirty.bottom - dirty.top,
                dc, dirty.left, dirty.top, SRCCOPY);
    }
    void Paint() {
        PAINTSTRUCT ps{}; HDC target = BeginPaint(window, &ps); RECT rc{}; GetClientRect(window, &rc);
        Draw(target, rc, ps.rcPaint); EndPaint(window, &ps);
    }
    void SaveTestImage(const std::wstring& filename, HWND control = nullptr) {
        RECT rc{}; GetClientRect(control ? control : window, &rc);
        BITMAPINFO info{}; info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        info.bmiHeader.biWidth = rc.right; info.bmiHeader.biHeight = -rc.bottom;
        info.bmiHeader.biPlanes = 1; info.bmiHeader.biBitCount = 32; info.bmiHeader.biCompression = BI_RGB;
        void* bits = nullptr; HDC dc = CreateCompatibleDC(nullptr);
        HBITMAP bitmap = CreateDIBSection(dc, &info, DIB_RGB_COLORS, &bits, nullptr, 0);
        if (!dc || !bitmap) { if (dc) DeleteDC(dc); if (bitmap) DeleteObject(bitmap); return; }
        auto old = SelectObject(dc, bitmap);
        // Owner-drawn buttons ignore WM_PRINT; render them explicitly so the
        // preview images show the same chrome as the visible window.
        auto drawButtons = [&](HDC target) {
            for (HWND child = GetWindow(window, GW_CHILD); child; child = GetWindow(child, GW_HWNDNEXT)) {
                LONG style = static_cast<LONG>(GetWindowLongPtrW(child, GWL_STYLE));
                if ((style & WS_VISIBLE) == 0 || (style & BS_TYPEMASK) != BS_OWNERDRAW) continue;
                RECT bounds{}; GetWindowRect(child, &bounds); MapWindowPoints(nullptr, window, reinterpret_cast<POINT*>(&bounds), 2);
                DRAWITEMSTRUCT item{}; item.CtlID = GetDlgCtrlID(child); item.hwndItem = child;
                item.rcItem = bounds; item.hDC = target;
                PaintButton(item);
            }
        };
        if (control) {
            if (control == window) Draw(dc, rc, rc);
            else { SetDCBrushColor(dc, UiTheme().bar); FillRect(dc, &rc, reinterpret_cast<HBRUSH>(GetStockObject(DC_BRUSH))); }
            PaintUiChildren(dc, control);
            if (control == window) drawButtons(dc);
        }
        else { Draw(dc, rc, rc); PaintUiChildren(dc, window); drawButtons(dc); }
        GdiFlush();
        BITMAPFILEHEADER header{}; header.bfType = 0x4d42;
        header.bfOffBits = sizeof(header) + sizeof(BITMAPINFOHEADER);
        DWORD bytes = rc.right * rc.bottom * 4; header.bfSize = header.bfOffBits + bytes;
        HANDLE output = CreateFileW(filename.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (output != INVALID_HANDLE_VALUE) {
            DWORD written = 0; WriteFile(output, &header, sizeof(header), &written, nullptr);
            WriteFile(output, &info.bmiHeader, sizeof(info.bmiHeader), &written, nullptr);
            WriteFile(output, bits, bytes, &written, nullptr); CloseHandle(output);
        }
        SelectObject(dc, old); DeleteObject(bitmap); DeleteDC(dc);
    }
};
} // namespace
