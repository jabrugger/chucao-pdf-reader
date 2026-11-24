#include <windows.h>
#include <windowsx.h>
#include <commdlg.h>
#include <commctrl.h>
#include <uxtheme.h>
#include <shellapi.h>
#include <shlobj.h>
#include <dwmapi.h>
#include <oleacc.h>
#include <oleauto.h>
#include <fpdfview.h>
#include <fpdf_text.h>
#include <fpdf_progressive.h>
#include "document-layout.h"
#include "text-geometry.h"
#include "document-outline.h"
#include "document-links.h"
#include "resource.h"
#include "app-info.h"
#include "ui-icons.h"
#include "reading-state.h"
#include "recent-files.h"
#include "settings.h"
#include "session-state.h"
#include "render-budget.h"
#include "file-association.h"
#include "pdf-errors.h"
#include <algorithm>
#include <atomic>
#include <condition_variable>
#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>
#include <cmath>
#include <chrono>
#include <cstdio>
#include <map>
#include <set>

#pragma comment(linker, "\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

// oleacc.h declares these without providing a library definition unless INITGUID
// is defined before every Windows header. Defining the two symbols here keeps
// the rest of the build independent of INITGUID (which breaks known folders on
// older SDKs).
EXTERN_C const CLSID DECLSPEC_SELECTANY CLSID_AccPropServices =
    {0xb5f8350b, 0x0548, 0x48b1, {0xa6, 0xee, 0x88, 0xbd, 0x00, 0xb4, 0xa5, 0xe7}};
EXTERN_C const MSAAPROPID DECLSPEC_SELECTANY PROPID_ACC_NAME =
    {0x608d3df8, 0x8128, 0x4aa7, {0xa4, 0x28, 0xf5, 0x5e, 0x49, 0x26, 0x72, 0x91}};

// Chucao PDF Reader entry point. The application is split into header fragments
// that share one translation unit (and one anonymous namespace), so the build stays
// a single cl invocation. Order matters: each fragment uses the ones before it.
#include "win-helpers.h"
#include "about-dialog.h"
#include "pdf-worker.h"
#include "password-dialog.h"
#include "updater.h"
#include "reader.h"
#include "settings-dialog.h"
#include "chrome-controls.h"
#include "window-proc.h"


int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int show) {
    auto started = std::chrono::steady_clock::now();
    int argc = 0; LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    bool navigationTest = argv && argc >= 4 && std::wstring(argv[1]) == L"--benchmark-navigation";
    bool textTest = argv && argc >= 4 && std::wstring(argv[1]) == L"--benchmark-text";
    bool destinationsTest = argv && argc >= 4 && std::wstring(argv[1]) == L"--benchmark-destinations";
    bool outlineTest = destinationsTest || (argv && argc >= 4 && std::wstring(argv[1]) == L"--benchmark-outline");
    bool linksTest = argv && argc >= 4 && std::wstring(argv[1]) == L"--benchmark-links";
    bool passwordTest = argv && argc >= 4 && std::wstring(argv[1]) == L"--benchmark-passwords";
    int stateTest = argv && argc == 5 ? (std::wstring(argv[1]) == L"--benchmark-state-write" ? 1 : std::wstring(argv[1]) == L"--benchmark-state-read" ? 2 : 0) : 0;
    bool recentTest = argv && argc == 5 && std::wstring(argv[1]) == L"--benchmark-recents";
    bool themeTest = argv && argc == 4 && std::wstring(argv[1]) == L"--benchmark-theme";
    bool startupTest = argv && argc >= 4 && std::wstring(argv[1]) == L"--benchmark-startup";
    bool dpiTest = argv && argc == 3 && std::wstring(argv[1]) == L"--benchmark-dpi";
    bool keysTest = argv && argc == 3 && std::wstring(argv[1]) == L"--benchmark-keys";
    bool hcTest = argv && argc == 3 && std::wstring(argv[1]) == L"--benchmark-highcontrast";
    bool rapidTest = argv && argc == 5 && std::wstring(argv[1]) == L"--benchmark-rapid";
    bool tabsTest = argv && argc == 6 && std::wstring(argv[1]) == L"--benchmark-tabs";
    bool outlineFilterTest = argv && argc == 4 && std::wstring(argv[1]) == L"--benchmark-outline-filter";
    bool windowTest = argv && argc == 4 && std::wstring(argv[1]) == L"--benchmark-window";
    bool fitTest = argv && argc == 4 && std::wstring(argv[1]) == L"--benchmark-fit";
    bool updatesTest = argv && argc == 4 && std::wstring(argv[1]) == L"--benchmark-updates";
    int sessionTest = argv ? (argc == 6 && std::wstring(argv[1]) == L"--benchmark-session-write" ? 1 : argc == 4 && std::wstring(argv[1]) == L"--benchmark-session-read" ? 2 : argc == 4 && std::wstring(argv[1]) == L"--benchmark-session-off" ? 3 : 0) : 0;
    bool interfaceTest = argv && argc >= 4 && std::wstring(argv[1]) == L"--benchmark-interface";
    bool benchmark = sessionTest || interfaceTest || recentTest || themeTest || startupTest || dpiTest || keysTest || hcTest || rapidTest || tabsTest || outlineFilterTest || windowTest || fitTest || updatesTest || stateTest || (argv && argc >= 4 && (std::wstring(argv[1]) == L"--benchmark-open" || navigationTest || textTest || outlineTest || linksTest || passwordTest));
    bool previewHome = argv && argc >= 3 && std::wstring(argv[1]) == L"--preview-home";
    bool previewAbout = argv && argc >= 3 && std::wstring(argv[1]) == L"--preview-about";
    bool previewLoading = argv && argc == 4 && std::wstring(argv[1]) == L"--preview-loading";
    bool forceDark = false;
    for (int i = 1; argv && i < argc; ++i) if (std::wstring(argv[i]) == L"--theme-dark") forceDark = true;
    bool forceHighContrast = false;
    for (int i = 1; argv && i < argc; ++i) if (std::wstring(argv[i]) == L"--force-highcontrast") forceHighContrast = true;
    bool shellBenchmark = argv && argc >= 3 && (std::wstring(argv[1]) == L"--benchmark-shell" || previewHome || previewAbout || previewLoading);
    EnablePerMonitorDpi();
    HRESULT comResult = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    Gdiplus::GdiplusStartupInput gdiplusInput;
    ULONG_PTR gdiplusToken = 0;
    Gdiplus::Status gdiplusStatus = Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusInput, nullptr);
    int initialDpi = SystemDpi();
    INITCOMMONCONTROLSEX controls{sizeof(controls), ICC_TREEVIEW_CLASSES | ICC_LINK_CLASS | ICC_LISTVIEW_CLASSES}; InitCommonControlsEx(&controls);
    WNDCLASSW wc{}; wc.hInstance = instance; wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hIcon = LoadIconW(instance, MAKEINTRESOURCEW(IDI_CHUCAO));
    wc.lpszClassName = L"ChucaoReader"; wc.lpfnWndProc = WindowProc; RegisterClassW(&wc);
    wc.lpszClassName = L"ChucaoPassword"; wc.lpfnWndProc = PasswordProc;
    wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1); RegisterClassW(&wc);
    wc.lpszClassName = L"ChucaoOutline"; wc.lpfnWndProc = OutlineProc;
    wc.hbrBackground = nullptr; RegisterClassW(&wc);
    Reader app;
    app.started = started;
    app.navigationTest = navigationTest;
    app.interfaceTest = interfaceTest;
    app.textTest = textTest;
    app.outlineTest = outlineTest;
    app.destinationsTest = destinationsTest;
    app.linksTest = linksTest;
    app.passwordTest = passwordTest;
    app.themeTest = themeTest;
    app.startupTest = startupTest;
    app.dpiTest = dpiTest;
    app.keysTest = keysTest;
    app.hcTest = hcTest;
    app.rapidTest = rapidTest;
    app.tabsTest = tabsTest;
    app.outlineFilterTest = outlineFilterTest;
    app.windowTest = windowTest;
    app.fitTest = fitTest;
    app.updatesTest = updatesTest;
    if (updatesTest) { app.updatesTestFile = argv[2]; app.benchmarkReport = argv[3]; }
    app.readingStateEnabled = !benchmark && !shellBenchmark;
    app.settingsEnabled = !benchmark && !shellBenchmark;
    if (windowTest) { app.settingsEnabled = true; app.settingsFile = argv[2]; app.settingsConfigured = true; }
    if (dpiTest || keysTest || hcTest) app.benchmarkReport = argv[2];
    if (themeTest) { app.settingsFile = argv[2]; app.settingsConfigured = true; app.settingsEnabled = true; }
    if (sessionTest) {
        std::wstring folder = argv[sessionTest == 1 ? 4 : 2];
        app.sessionTest = sessionTest; app.settingsEnabled = true; app.settingsFile = folder + L"\\settings.dat";
        app.readingStateEnabled = true; app.readingStateFile = folder + L"\\reading-state.dat";
        app.readingStateWrites = sessionTest == 1; app.sessionWrites = sessionTest == 1;
        if (sessionTest == 1) { app.sessionTestA = argv[2]; app.sessionTestB = argv[3]; }
    }
    if (forceDark) { app.forceDarkTheme = true; app.settingsEnabled = false; }
    if (forceHighContrast) { app.forceHighContrast = true; app.settingsEnabled = false; }
    if (recentTest) { app.recentTest = true; app.readingStateFile = argv[4]; app.readingStateEnabled = true; }
    if (stateTest) {
        app.readingStateTest = stateTest; app.readingStateFile = argv[4];
        app.readingStateEnabled = true; app.readingStateWrites = stateTest == 1;
    }
    if (passwordTest) { std::wstring file = argv[2]; app.passwordTestRoot = file.substr(0, file.find_last_of(L"\\/") + 1); }
    if (benchmark && !dpiTest && !keysTest && !hcTest && !rapidTest && !tabsTest) app.benchmarkReport = argv[3];
    if (rapidTest) app.benchmarkReport = argv[4];
    if (tabsTest) { app.tabsTestA = argv[2]; app.tabsTestB = argv[3]; app.tabsTestProtected = argv[4]; app.benchmarkReport = argv[5]; }
    if (sessionTest) app.benchmarkReport = argv[sessionTest == 1 ? 5 : 3];
    if (shellBenchmark) app.benchmarkReport = argv[2];
    HWND window = CreateWindowExW(WS_EX_CONTROLPARENT, L"ChucaoReader", L"Chucao PDF Reader",
        WS_OVERLAPPEDWINDOW | WS_VSCROLL | WS_HSCROLL | WS_CLIPCHILDREN,
        CW_USEDEFAULT, CW_USEDEFAULT, MulDiv(1080, initialDpi, 96), MulDiv(800, initialDpi, 96), nullptr, nullptr, instance, &app);
    if (!window) {
        if (gdiplusStatus == Gdiplus::Ok) Gdiplus::GdiplusShutdown(gdiplusToken);
        if (SUCCEEDED(comResult)) CoUninitialize();
        return 1;
    }
    bool startMaximized = false;
    if (!benchmark && !shellBenchmark) startMaximized = app.PrepareWindowPlacement();
    if (!benchmark && !shellBenchmark) {
        ShowWindow(window, (startMaximized || show == SW_SHOWMAXIMIZED) ? SW_SHOWMAXIMIZED : show);
        UpdateWindow(window);
        app.StyleNativeBars();
    }
    app.shellMs = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - started).count();
    if (shellBenchmark) {
        bool success = true;
        if (previewHome) app.SaveTestImage(app.benchmarkReport + L".home.bmp");
        if (previewAbout) {
            HWND dialog = CreateDialogParamW(instance, MAKEINTRESOURCEW(IDD_ABOUT), window, AboutProc, 0);
            success = dialog != nullptr;
            if (dialog) { app.SaveTestImage(app.benchmarkReport + L".about.bmp", dialog); DestroyWindow(dialog); }
        }
        if (previewLoading) {
            app.benchmarkReport = argv[3];
            app.Load(argv[2]);
            app.SaveTestImage(app.benchmarkReport + L".loading.bmp");
        }
        app.BenchmarkResult(success);
    }
    else if (sessionTest) { app.RunSessionTest(); SetTimer(window, 2, 30000, nullptr); }
    else if (themeTest) app.RunThemeTest();
    else if (dpiTest) app.RunDpiTest();
    else if (keysTest) app.RunKeysTest();
    else if (hcTest) app.RunHcTest();
    else if (rapidTest) { app.Load(argv[2]); app.Load(argv[3]); SetTimer(window, 2, 30000, nullptr); }
    else if (tabsTest) { app.RunTabsTest(); SetTimer(window, 2, 30000, nullptr); }
    else if (outlineFilterTest) { app.Load(argv[2]); SetTimer(window, 2, 30000, nullptr); }
    else if (windowTest) app.RunWindowTest();
    else if (fitTest) { app.Load(argv[2]); SetTimer(window, 2, 30000, nullptr); }
    else if (updatesTest) app.RunUpdatesTest();
    else if (benchmark) { app.Load(argv[2]); SetTimer(window, 2, 30000, nullptr); }
    else if (argv && argc > 1) {
        // Diagnostic switches are not document paths; every other argument opens a tab.
        for (int i = 1; i < argc; ++i) {
            std::wstring argument = argv[i];
            if (argument != L"--theme-dark" && argument != L"--force-highcontrast") app.OpenDocument(argument);
        }
    }
    if (!benchmark && !shellBenchmark && app.tabs.empty()) app.RestoreSession();
    app.RefreshHomeRecents();
    if (!benchmark && !shellBenchmark) app.StartUpdateCheck();
    if (argv) LocalFree(argv);
    MSG msg{};
    while (GetMessageW(&msg, nullptr, 0, 0) > 0) {
        if (msg.message == WM_KEYDOWN && msg.wParam == VK_ESCAPE && app.searchVisible) { app.HideSearch(); continue; }
        if (msg.message == WM_KEYDOWN && msg.wParam == VK_RETURN && GetFocus() == app.homeList) {
            int selected = static_cast<int>(SendMessageW(app.homeList, LB_GETCURSEL, 0, 0));
            if (selected >= 0) app.Command(RecentBase + selected); continue;
        }
        if (msg.message == WM_KEYDOWN && msg.wParam == VK_F1) { app.Command(About); continue; }
        if (msg.message == WM_KEYDOWN && msg.wParam == VK_F6) { app.CycleFocus(); continue; }
        if (app.outlineWindow && (msg.hwnd == app.outlineWindow || IsChild(app.outlineWindow, msg.hwnd))) {
            if (msg.message == WM_KEYDOWN && GetFocus() == app.outlineFilter) {
                if (msg.wParam == VK_ESCAPE && app.ClearOutlineFilterText()) continue;
                if (msg.wParam == VK_RETURN) { app.NavigateFirstOutlineMatch(); continue; }
                if (msg.wParam == VK_DOWN) { app.FocusOutlineTree(); continue; }
            }
            if (msg.message == WM_KEYDOWN && msg.wParam == 'F' && GetKeyState(VK_CONTROL) < 0) { app.ShowSearch(); continue; }
            if (msg.message == WM_KEYDOWN && msg.wParam == VK_RETURN && GetFocus() == app.outlineTree) { app.NavigateOutline(); continue; }
            if (msg.message == WM_KEYDOWN && (msg.wParam == VK_ESCAPE || (msg.wParam == 'B' && GetKeyState(VK_CONTROL) < 0))) {
                SendMessageW(app.outlineWindow, WM_CLOSE, 0, 0); continue;
            }
            if (!IsDialogMessageW(app.outlineWindow, &msg)) { TranslateMessage(&msg); DispatchMessageW(&msg); }
            continue;
        }
        if (msg.message == WM_KEYDOWN) {
            SendMessageW(window, WM_CHANGEUISTATE, MAKEWPARAM(UIS_CLEAR, UISF_HIDEFOCUS), 0);
            bool ctrl = GetKeyState(VK_CONTROL) < 0, edit = GetFocus() == app.query;
            HWND focused = GetFocus();
            if (!edit && focused == window &&
                (msg.wParam == VK_APPS || (msg.wParam == VK_F10 && GetKeyState(VK_SHIFT) < 0))) {
                app.ShowTextMenuAtSelection(); continue;
            }
            if (!edit && GetKeyState(VK_MENU) < 0 && msg.wParam == VK_HOME) {
                app.ShowHome(); continue;
            }
            if (!edit && focused == window && msg.wParam == VK_SPACE) {
                app.pendingReadingState.reset(); ++app.linkEpoch;
                int amount = app.ViewHeight() - app.D(40);
                app.ScrollTo(app.scroll + (GetKeyState(VK_SHIFT) < 0 ? -amount : amount));
                continue;
            }
            if (!edit && (focused == window || focused == app.outlineTree) &&
                (msg.wParam == VK_ADD || msg.wParam == VK_OEM_PLUS || msg.wParam == '=')) { app.Command(Larger); continue; }
            if (!edit && (focused == window || focused == app.outlineTree) &&
                (msg.wParam == VK_SUBTRACT || msg.wParam == VK_OEM_MINUS || msg.wParam == '-')) { app.Command(Smaller); continue; }
            if (!edit && GetKeyState(VK_MENU) < 0 && msg.wParam == VK_LEFT) { app.BackFromLink(); continue; }
            if (!edit && focused != app.homeList && (msg.wParam == VK_NEXT || msg.wParam == VK_PRIOR || msg.wParam == VK_DOWN || msg.wParam == VK_UP || msg.wParam == VK_HOME || msg.wParam == VK_END)) {
                app.pendingReadingState.reset();
                ++app.linkEpoch;
                if (msg.wParam == VK_HOME) app.ScrollTo(0);
                else if (msg.wParam == VK_END) app.ScrollTo(app.MaxScroll());
                else app.ScrollTo(app.scroll + (msg.wParam == VK_UP ? -app.D(40) : msg.wParam == VK_DOWN ? app.D(40) :
                    msg.wParam == VK_PRIOR ? -app.ViewHeight() + app.D(40) : app.ViewHeight() - app.D(40)));
                continue;
            }
            int command = 0;
            if (!edit && msg.wParam == VK_ESCAPE) { app.EndSelection(); app.ClearSelection(); SetFocus(window); continue; }
            if (ctrl && msg.wParam == 'O') command = Open;
            else if (ctrl && msg.wParam == 'B') command = Outline;
            else if (ctrl && msg.wParam == 'F') command = Find;
            else if (ctrl && msg.wParam == 'P') command = Print;
            else if (ctrl && !edit && msg.wParam == 'C') command = Copy;
            else if (ctrl && msg.wParam == 'D') { app.ToggleTheme(); continue; }
            else if (ctrl && msg.wParam == '0') command = Fit;
            else if (ctrl && msg.wParam == '1') command = FitPageCmd;
            else if (ctrl && msg.wParam == '2') command = FitHeightCmd;
            else if (ctrl && msg.wParam == VK_TAB) command = GetKeyState(VK_SHIFT) < 0 ? PrevTabCmd : NextTabCmd;
            else if (ctrl && !edit && msg.wParam == 'W') command = CloseTabCmd;
            else if (ctrl && !edit && msg.wParam == 'T') command = NewTabCmd;
            else if (msg.wParam == VK_F3 || (edit && msg.wParam == VK_RETURN)) command = GetKeyState(VK_SHIFT) < 0 ? FindPrevious : FindNext;
            else if (!edit && msg.wParam == VK_RIGHT) command = Next;
            else if (!edit && msg.wParam == VK_LEFT) command = Previous;
            if (command) { app.Command(command); continue; }
        }
        if (!IsDialogMessageW(window, &msg)) { TranslateMessage(&msg); DispatchMessageW(&msg); }
    }
    if (gdiplusStatus == Gdiplus::Ok) Gdiplus::GdiplusShutdown(gdiplusToken);
    if (SUCCEEDED(comResult)) CoUninitialize();
    return static_cast<int>(msg.wParam);
}
