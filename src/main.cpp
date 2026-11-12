#include <windows.h>
#include <windowsx.h>
#include <commdlg.h>
#include <shellapi.h>
#include <fpdfview.h>
#include <fpdf_text.h>
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

namespace {
constexpr UINT Ready = WM_APP + 1;
constexpr int BarHeight = 58;
enum { Open = 100, Previous, Next, Smaller, Larger, Fit, Find, Print, Query, FindNext };

std::string Utf8(const std::wstring& value) {
    int size = WideCharToMultiByte(CP_UTF8, 0, value.data(), static_cast<int>(value.size()), nullptr, 0, nullptr, nullptr);
    std::string result(size, 0);
    if (size) WideCharToMultiByte(CP_UTF8, 0, value.data(), static_cast<int>(value.size()), &result[0], size, nullptr, nullptr);
    return result;
}
std::wstring Text(HWND window) {
    std::wstring value(GetWindowTextLengthW(window) + 1, 0);
    GetWindowTextW(window, &value[0], static_cast<int>(value.size()));
    value.resize(wcslen(value.c_str()));
    return value;
}
struct Result {
    enum Kind { Image, Opened, Password, Failure, Search, Printed } kind = Failure;
    unsigned long long document = 0, revision = 0;
    int page = 0, pages = 0, width = 0, height = 0, match = -1;
    double milliseconds = 0;
    double scale = 1;
    std::wstring message;
    std::vector<unsigned char> pixels;
};

// All PDFium calls and handles live on this single worker thread.
class PdfWorker {
    HWND owner_;
    std::mutex mutex_;
    std::condition_variable wake_;
    std::deque<std::function<void()>> jobs_;
    bool stopping_ = false;
    FPDF_DOCUMENT pdf_ = nullptr;
    unsigned long long document_ = 0;
    std::atomic<unsigned long long> revision_{0};
    std::atomic<bool> cancelled_{false};
    int searchPage_ = -1, searchIndex_ = -1;
    std::wstring lastQuery_;
    std::thread thread_;

    void Send(std::unique_ptr<Result> result) {
        result->document = document_;
        if (PostMessageW(owner_, Ready, 0, reinterpret_cast<LPARAM>(result.get()))) result.release();
    }
    void Queue(std::function<void()> job) {
        { std::lock_guard<std::mutex> lock(mutex_); jobs_.push_back(std::move(job)); }
        wake_.notify_one();
    }
    void Run() {
        FPDF_LIBRARY_CONFIG config{}; config.version = 2;
        FPDF_InitLibraryWithConfig(&config);
        for (;;) {
            std::function<void()> job;
            {
                std::unique_lock<std::mutex> lock(mutex_);
                wake_.wait(lock, [this] { return stopping_ || !jobs_.empty(); });
                if (stopping_ && jobs_.empty()) break;
                job = std::move(jobs_.front()); jobs_.pop_front();
            }
            job();
        }
        if (pdf_) FPDF_CloseDocument(pdf_);
        FPDF_DestroyLibrary();
    }
public:
    explicit PdfWorker(HWND owner) : owner_(owner), thread_([this] { Run(); }) {}
    ~PdfWorker() {
        cancelled_ = true;
        { std::lock_guard<std::mutex> lock(mutex_); stopping_ = true; }
        wake_.notify_one(); thread_.join();
    }
    void Load(std::wstring path, std::wstring password, unsigned long long document) {
        ++revision_;
        Queue([this, path, password, document] {
            if (cancelled_) return;
            if (pdf_) { FPDF_CloseDocument(pdf_); pdf_ = nullptr; }
            document_ = document; searchPage_ = searchIndex_ = -1; lastQuery_.clear();
            pdf_ = FPDF_LoadDocument(Utf8(path).c_str(), Utf8(password).c_str());
            auto result = std::make_unique<Result>();
            if (!pdf_) {
                auto error = FPDF_GetLastError();
                result->kind = error == FPDF_ERR_PASSWORD ? Result::Password : Result::Failure;
                result->message = error == FPDF_ERR_FORMAT ? L"El archivo no es un PDF válido o está dañado." : L"No se pudo abrir el PDF.";
            } else {
                result->kind = Result::Opened; result->pages = FPDF_GetPageCount(pdf_);
                if (result->pages <= 0) { result->kind = Result::Failure; result->message = L"El PDF no contiene páginas."; }
            }
            Send(std::move(result));
        });
    }
    unsigned long long Render(int page, int viewport, double zoom, unsigned long long document) {
        auto revision = ++revision_;
        Queue([this, page, viewport, zoom, revision, document] {
            if (cancelled_ || !pdf_ || document != document_ || revision != revision_) return;
            LARGE_INTEGER start{}, end{}, frequency{};
            QueryPerformanceFrequency(&frequency); QueryPerformanceCounter(&start);
            auto result = std::make_unique<Result>(); result->revision = revision; result->page = page;
            FPDF_PAGE pdfPage = FPDF_LoadPage(pdf_, page);
            if (!pdfPage) { result->message = L"No se pudo leer la página."; Send(std::move(result)); return; }
            double width = FPDF_GetPageWidth(pdfPage), height = FPDF_GetPageHeight(pdfPage);
            if (!(width > 0 && height > 0 && std::isfinite(width) && std::isfinite(height))) {
                FPDF_ClosePage(pdfPage); result->message = L"Dimensiones de página inválidas."; Send(std::move(result)); return;
            }
            double scale = zoom > 0 ? zoom : std::max(100, viewport - 64) / width;
            double w = std::max(1.0, width * scale), h = std::max(1.0, height * scale);
            // Bound allocations before conversion to integers; never render giant pages unchecked.
            double reduction = std::min(1.0, std::min(16000.0 / std::max(w, h), std::sqrt(20000000.0 / (w * h))));
            result->width = std::max(1, static_cast<int>(w * reduction));
            result->height = std::max(1, static_cast<int>(h * reduction));
            result->scale = result->width / width;
            result->pixels.resize(static_cast<size_t>(result->width) * result->height * 4);
            FPDF_BITMAP bitmap = FPDFBitmap_CreateEx(result->width, result->height, FPDFBitmap_BGRA, result->pixels.data(), result->width * 4);
            if (bitmap) {
                FPDFBitmap_FillRect(bitmap, 0, 0, result->width, result->height, 0xffffffff);
                FPDF_RenderPageBitmap(bitmap, pdfPage, 0, 0, result->width, result->height, 0, FPDF_ANNOT);
                FPDFBitmap_Destroy(bitmap); result->kind = Result::Image;
            } else result->message = L"No hay memoria para dibujar la página.";
            FPDF_ClosePage(pdfPage); QueryPerformanceCounter(&end);
            result->milliseconds = 1000.0 * (end.QuadPart - start.QuadPart) / frequency.QuadPart;
            if (revision == revision_) Send(std::move(result));
        });
        return revision;
    }
    void Search(std::wstring query, int page, unsigned long long document) {
        Queue([this, query, page, document] {
            if (cancelled_ || !pdf_ || document != document_ || query.empty()) return;
            auto result = std::make_unique<Result>(); result->kind = Result::Search;
            int count = FPDF_GetPageCount(pdf_);
            int first = query == lastQuery_ && searchPage_ == page ? searchIndex_ + 1 : 0;
            for (int step = 0; step <= count && !cancelled_; ++step) {
                int index = (page + step) % count;
                FPDF_PAGE pdfPage = FPDF_LoadPage(pdf_, index);
                if (!pdfPage) continue;
                FPDF_TEXTPAGE text = FPDFText_LoadPage(pdfPage);
                auto find = text ? FPDFText_FindStart(text, reinterpret_cast<FPDF_WIDESTRING>(query.c_str()), 0, step == 0 ? first : 0) : nullptr;
                bool found = find && FPDFText_FindNext(find);
                int match = found ? FPDFText_GetSchResultIndex(find) : -1;
                if (find) FPDFText_FindClose(find);
                if (text) FPDFText_ClosePage(text);
                FPDF_ClosePage(pdfPage);
                if (found) {
                    result->page = index; result->match = match;
                    searchPage_ = index; searchIndex_ = match; lastQuery_ = query; break;
                }
            }
            Send(std::move(result));
        });
    }
    void PrintDocument(HDC dc, int first, int last, unsigned long long document) {
        Queue([this, dc, first, last, document] {
            auto result = std::make_unique<Result>(); result->kind = Result::Printed;
            bool ok = !cancelled_ && pdf_ && document == document_;
            if (ok && !(FPDF_GetDocPermissions(pdf_) & 4)) {
                DeleteDC(dc); result->message = L"El documento no permite imprimir con esta contraseña.";
                Send(std::move(result)); return;
            }
            bool started = false;
            DOCINFOW info{}; info.cbSize = sizeof(info); info.lpszDocName = L"Chucao PDF Reader";
            if (ok) { started = StartDocW(dc, &info) > 0; ok = started; }
            for (int index = first; ok && index <= last; ++index) {
                if (cancelled_) { ok = false; break; }
                FPDF_PAGE page = FPDF_LoadPage(pdf_, index);
                if (!page) { ok = false; break; }
                double w = FPDF_GetPageWidth(page), h = FPDF_GetPageHeight(page);
                int availableW = GetDeviceCaps(dc, HORZRES), availableH = GetDeviceCaps(dc, VERTRES);
                double scale = (w > 0 && h > 0) ? std::min(availableW / w, availableH / h) : 0;
                ok = scale > 0 && StartPage(dc) > 0;
                if (ok) {
                    int outW = static_cast<int>(w * scale), outH = static_cast<int>(h * scale);
                    ok = FPDF_RenderPage(dc, page, (availableW - outW) / 2, (availableH - outH) / 2, outW, outH, 0, FPDF_PRINTING | FPDF_ANNOT) != 0;
                    if (EndPage(dc) <= 0) ok = false;
                }
                FPDF_ClosePage(page);
            }
            if (started) { if (ok) ok = EndDoc(dc) > 0; else AbortDoc(dc); }
            DeleteDC(dc); result->message = ok ? L"Documento enviado a la impresora." : L"No se pudo completar la impresión.";
            Send(std::move(result));
        });
    }
};

struct PasswordState { HWND edit = nullptr; bool done = false, accepted = false; std::wstring value; };
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
    if (msg == WM_CLOSE) { state->done = true; return 0; }
    return DefWindowProcW(window, msg, wp, lp);
}
bool PasswordDialog(HWND owner, std::wstring& password) {
    PasswordState state;
    RECT bounds{}; GetWindowRect(owner, &bounds);
    HWND dialog = CreateWindowExW(WS_EX_DLGMODALFRAME, L"ChucaoPassword", L"Contraseña del PDF", WS_CAPTION | WS_SYSMENU | WS_POPUP,
        bounds.left + 100, bounds.top + 120, 380, 160, owner, nullptr, GetModuleHandleW(nullptr), &state);
    if (!dialog) return false;
    CreateWindowW(L"STATIC", L"Ingresá la contraseña para abrir el documento:", WS_CHILD | WS_VISIBLE, 18, 14, 340, 22, dialog, nullptr, nullptr, nullptr);
    state.edit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_PASSWORD | ES_AUTOHSCROLL, 18, 42, 330, 25, dialog, nullptr, nullptr, nullptr);
    SendMessageW(state.edit, EM_SETLIMITTEXT, 256, 0);
    CreateWindowW(L"BUTTON", L"Abrir", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_DEFPUSHBUTTON, 168, 82, 85, 28, dialog, reinterpret_cast<HMENU>(IDOK), nullptr, nullptr);
    CreateWindowW(L"BUTTON", L"Cancelar", WS_CHILD | WS_VISIBLE | WS_TABSTOP, 263, 82, 85, 28, dialog, reinterpret_cast<HMENU>(IDCANCEL), nullptr, nullptr);
    EnableWindow(owner, FALSE); ShowWindow(dialog, SW_SHOW); SetFocus(state.edit);
    MSG message{};
    while (!state.done) {
        int status = GetMessageW(&message, nullptr, 0, 0);
        if (status <= 0) { if (!status) PostQuitMessage(static_cast<int>(message.wParam)); break; }
        if (message.message == WM_KEYDOWN && message.wParam == VK_RETURN) SendMessageW(dialog, WM_COMMAND, IDOK, 0);
        else if (message.message == WM_KEYDOWN && message.wParam == VK_ESCAPE) SendMessageW(dialog, WM_COMMAND, IDCANCEL, 0);
        else if (!IsDialogMessageW(dialog, &message)) { TranslateMessage(&message); DispatchMessageW(&message); }
    }
    password = std::move(state.value);
    SetWindowTextW(state.edit, L""); DestroyWindow(dialog); EnableWindow(owner, TRUE); SetActiveWindow(owner);
    return state.accepted;
}

class Reader {
public:
    HWND window = nullptr, query = nullptr;
    HFONT font = nullptr;
    std::unique_ptr<PdfWorker> worker;
    std::unique_ptr<Result> image;
    unsigned long long document = 0, revision = 0;
    int page = 0, pages = 0, scroll = 0;
    double zoom = 0;
    bool searching = false, printing = false;
    std::wstring path, status = L"Abrí un PDF para empezar · Ctrl+O";
    std::wstring benchmarkReport;
    std::chrono::steady_clock::time_point started;
    double shellMs = 0, openedMs = 0;
    void BenchmarkResult(bool success) {
        if (benchmarkReport.empty()) return;
        double readyMs = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - started).count();
        char data[512]{};
        int size = sprintf_s(data, "{\"success\":%s,\"shell_ms\":%.3f,\"opened_ms\":%.3f,\"ready_ms\":%.3f,\"pages\":%d}\n", success ? "true" : "false", shellMs, openedMs, readyMs, pages);
        HANDLE output = CreateFileW(benchmarkReport.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (output != INVALID_HANDLE_VALUE) { DWORD written = 0; WriteFile(output, data, size, &written, nullptr); CloseHandle(output); }
        PostMessageW(window, WM_CLOSE, 0, 0);
    }

    void Redraw() { InvalidateRect(window, nullptr, FALSE); }
    void ScrollTo(int value) {
        RECT rc{}; GetClientRect(window, &rc);
        scroll = std::max(0, std::min(value, image ? std::max<int>(0, image->height - (rc.bottom - BarHeight - 48)) : 0));
        SCROLLINFO info{sizeof(info), SIF_RANGE | SIF_PAGE | SIF_POS};
        info.nMin = 0; info.nMax = image ? image->height + 47 : 0;
        info.nPage = std::max<int>(1, rc.bottom - BarHeight); info.nPos = scroll;
        SetScrollInfo(window, SB_VERT, &info, TRUE); Redraw();
    }
    void Render() {
        KillTimer(window, 1); // An immediate render supersedes a pending resize render.
        if (!pages) return;
        RECT rc{}; GetClientRect(window, &rc);
        revision = worker->Render(page, rc.right, zoom, document);
        status = L"Cargando página " + std::to_wstring(page + 1) + L"…"; Redraw();
    }
    void Load(const std::wstring& file, const std::wstring& password = L"", bool retry = false) {
        if (!worker) worker = std::make_unique<PdfWorker>(window);
        if (!retry) { path = file; ++document; pages = 0; page = 0; scroll = 0; image.reset(); zoom = 0; searching = false; }
        status = L"Abriendo documento…"; Redraw(); worker->Load(path, password, document);
    }
    void ChooseFile() {
        wchar_t filename[32768]{};
        OPENFILENAMEW ofn{}; ofn.lStructSize = sizeof(ofn); ofn.hwndOwner = window;
        ofn.lpstrFilter = L"Documentos PDF\0*.pdf\0Todos los archivos\0*.*\0";
        ofn.lpstrFile = filename; ofn.nMaxFile = 32768; ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;
        if (GetOpenFileNameW(&ofn)) Load(filename);
    }
    void ChangePage(int target) {
        if (!pages) return;
        target = std::max(0, std::min(pages - 1, target));
        if (page != target) { page = target; scroll = 0; image.reset(); Render(); }
    }
    void Search() {
        if (!pages || searching) return;
        auto value = Text(query); if (value.empty()) { SetFocus(query); return; }
        searching = true; status = L"Buscando…"; Redraw(); worker->Search(value, page, document);
    }
    void PrintFile() {
        if (!pages || printing) return;
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
    void Command(int id) {
        switch (id) {
        case Open: ChooseFile(); break;
        case Previous: ChangePage(page - 1); break;
        case Next: ChangePage(page + 1); break;
        case Fit: zoom = 0; scroll = 0; Render(); break;
        case Larger: case Smaller: {
            // Start manual zoom from the current fit-width image scale.
            if (!zoom) zoom = image ? image->scale : 1.0;
            zoom = std::max(0.25, std::min(4.0, zoom * (id == Larger ? 1.2 : 1 / 1.2)));
            Render(); break;
        }
        case Find: SetFocus(query); SendMessageW(query, EM_SETSEL, 0, -1); break;
        case FindNext: Search(); break;
        case Print: PrintFile(); break;
        }
    }
    void Receive(std::unique_ptr<Result> result) {
        if (result->kind == Result::Printed) printing = false;
        if (result->document != document) return;
        switch (result->kind) {
        case Result::Opened:
            openedMs = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - started).count();
            pages = result->pages; SetWindowTextW(window, (L"Chucao — " + path.substr(path.find_last_of(L"\\/") + 1)).c_str()); Render(); break;
        case Result::Image:
            if (result->revision != revision || result->page != page) return;
            status = L"Página " + std::to_wstring(page + 1) + L" de " + std::to_wstring(pages);
            image = std::move(result); ScrollTo(scroll); BenchmarkResult(true); break;
        case Result::Password: {
            if (!benchmarkReport.empty()) { BenchmarkResult(false); break; }
            std::wstring password;
            if (PasswordDialog(window, password)) Load(path, password, true);
            else { status = L"Apertura cancelada."; Redraw(); }
            if (!password.empty()) SecureZeroMemory(&password[0], password.size() * sizeof(wchar_t));
            break;
        }
        case Result::Search:
            searching = false;
            if (result->match >= 0) {
                if (page != result->page) ChangePage(result->page);
                status = L"Coincidencia en página " + std::to_wstring(result->page + 1) + L". F3: siguiente.";
            } else status = L"No se encontró el texto.";
            Redraw(); break;
        case Result::Failure: case Result::Printed: status = result->message; Redraw(); if (result->kind == Result::Failure) BenchmarkResult(false); break;
        }
    }
    void Paint() {
        PAINTSTRUCT ps{}; HDC dc = BeginPaint(window, &ps); RECT rc{}; GetClientRect(window, &rc);
        HBRUSH background = CreateSolidBrush(RGB(238, 240, 243)); FillRect(dc, &rc, background); DeleteObject(background);
        RECT bar{0, 0, rc.right, BarHeight}; FillRect(dc, &bar, reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1));
        SelectObject(dc, font); SetBkMode(dc, TRANSPARENT); SetTextColor(dc, RGB(72, 79, 92));
        if (image) {
            BITMAPINFO info{}; info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
            info.bmiHeader.biWidth = image->width; info.bmiHeader.biHeight = -image->height;
            info.bmiHeader.biPlanes = 1; info.bmiHeader.biBitCount = 32; info.bmiHeader.biCompression = BI_RGB;
            int x = std::max<int>(24, (rc.right - image->width) / 2), y = BarHeight + 24 - scroll;
            IntersectClipRect(dc, 0, BarHeight, rc.right, rc.bottom - 26);
            SetDIBitsToDevice(dc, x, y, image->width, image->height, 0, 0, 0, image->height, image->pixels.data(), &info, DIB_RGB_COLORS);
            SelectClipRgn(dc, nullptr);
        } else if (!pages) {
            RECT empty{24, BarHeight, rc.right - 24, rc.bottom - 40};
            DrawTextW(dc, L"Chucao\n\nTu documento, sin distracciones.\n\nAbrir PDF · Ctrl+O", -1, &empty, DT_CENTER | DT_VCENTER | DT_WORDBREAK);
        }
        RECT footer{16, rc.bottom - 24, rc.right - 16, rc.bottom};
        FillRect(dc, &footer, background = CreateSolidBrush(RGB(238, 240, 243))); DeleteObject(background);
        DrawTextW(dc, status.c_str(), -1, &footer, DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS);
        EndPaint(window, &ps);
    }
};

LRESULT CALLBACK WindowProc(HWND window, UINT message, WPARAM wp, LPARAM lp) {
    auto app = reinterpret_cast<Reader*>(GetWindowLongPtrW(window, GWLP_USERDATA));
    if (message == WM_NCCREATE) {
        app = static_cast<Reader*>(reinterpret_cast<CREATESTRUCTW*>(lp)->lpCreateParams);
        app->window = window; SetWindowLongPtrW(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(app));
    }
    if (!app) return DefWindowProcW(window, message, wp, lp);
    switch (message) {
    case WM_CREATE: {
        app->font = CreateFontW(-15, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
        struct Control { const wchar_t* text; int id, width; };
        Control buttons[] = {{L"Abrir", Open, 64}, {L"‹", Previous, 34}, {L"›", Next, 34}, {L"−", Smaller, 34}, {L"+", Larger, 34}, {L"Ancho", Fit, 64}, {L"Buscar", FindNext, 64}, {L"Imprimir", Print, 78}};
        int x = 14;
        for (const auto& button : buttons) {
            HWND control = CreateWindowW(L"BUTTON", button.text, WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_FLAT, x, 14, button.width, 30, window, reinterpret_cast<HMENU>(static_cast<INT_PTR>(button.id)), nullptr, nullptr);
            SendMessageW(control, WM_SETFONT, reinterpret_cast<WPARAM>(app->font), TRUE); x += button.width + 6;
        }
        app->query = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL, x + 8, 16, 220, 26, window, reinterpret_cast<HMENU>(Query), nullptr, nullptr);
        SendMessageW(app->query, WM_SETFONT, reinterpret_cast<WPARAM>(app->font), TRUE); SendMessageW(app->query, EM_SETLIMITTEXT, 256, 0);
        DragAcceptFiles(window, TRUE); return 0;
    }
    case WM_COMMAND: app->Command(LOWORD(wp)); return 0;
    case WM_DROPFILES: {
        wchar_t path[32768]{}; HDROP drop = reinterpret_cast<HDROP>(wp);
        if (DragQueryFileW(drop, 0, path, 32768)) app->Load(path);
        DragFinish(drop); return 0;
    }
    case Ready: app->Receive(std::unique_ptr<Result>(reinterpret_cast<Result*>(lp))); return 0;
    case WM_SIZE: app->ScrollTo(app->scroll); if (wp != SIZE_MINIMIZED) SetTimer(window, 1, 120, nullptr); return 0;
    case WM_TIMER:
        if (wp == 2) { KillTimer(window, 2); app->BenchmarkResult(false); }
        else { KillTimer(window, 1); app->Render(); }
        return 0;
    case WM_GETMINMAXINFO: reinterpret_cast<MINMAXINFO*>(lp)->ptMinTrackSize = {820, 400}; return 0;
    case WM_MOUSEWHEEL:
        if (GetKeyState(VK_CONTROL) < 0) app->Command(GET_WHEEL_DELTA_WPARAM(wp) > 0 ? Larger : Smaller);
        else app->ScrollTo(app->scroll - GET_WHEEL_DELTA_WPARAM(wp)); return 0;
    case WM_VSCROLL: {
        SCROLLINFO info{sizeof(info), SIF_ALL}; GetScrollInfo(window, SB_VERT, &info);
        int value = app->scroll;
        switch (LOWORD(wp)) {
        case SB_LINEUP: value -= 40; break; case SB_LINEDOWN: value += 40; break;
        case SB_PAGEUP: value -= info.nPage; break; case SB_PAGEDOWN: value += info.nPage; break;
        case SB_THUMBTRACK: value = info.nTrackPos; break;
        }
        app->ScrollTo(value); return 0;
    }
    case WM_ERASEBKGND: return 1;
    case WM_PAINT: app->Paint(); return 0;
    case WM_DESTROY: {
        app->worker.reset();
        MSG pending{}; while (PeekMessageW(&pending, window, Ready, Ready, PM_REMOVE)) delete reinterpret_cast<Result*>(pending.lParam);
        DeleteObject(app->font); PostQuitMessage(0); return 0;
    }
    }
    return DefWindowProcW(window, message, wp, lp);
}
} // namespace

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int show) {
    auto started = std::chrono::steady_clock::now();
    int argc = 0; LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    bool benchmark = argv && argc == 4 && std::wstring(argv[1]) == L"--benchmark-open";
    bool shellBenchmark = argv && argc == 3 && std::wstring(argv[1]) == L"--benchmark-shell";
    SetProcessDPIAware();
    WNDCLASSW wc{}; wc.hInstance = instance; wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.lpszClassName = L"ChucaoReader"; wc.lpfnWndProc = WindowProc; RegisterClassW(&wc);
    wc.lpszClassName = L"ChucaoPassword"; wc.lpfnWndProc = PasswordProc;
    wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1); RegisterClassW(&wc);
    Reader app;
    app.started = started;
    if (benchmark) app.benchmarkReport = argv[3];
    if (shellBenchmark) app.benchmarkReport = argv[2];
    HWND window = CreateWindowW(L"ChucaoReader", L"Chucao PDF Reader", WS_OVERLAPPEDWINDOW | WS_VSCROLL | WS_CLIPCHILDREN,
        CW_USEDEFAULT, CW_USEDEFAULT, 1080, 800, nullptr, nullptr, instance, &app);
    if (!window) return 1;
    if (!benchmark && !shellBenchmark) { ShowWindow(window, show); UpdateWindow(window); }
    app.shellMs = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - started).count();
    if (shellBenchmark) app.BenchmarkResult(true);
    else if (benchmark) { app.Load(argv[2]); SetTimer(window, 2, 30000, nullptr); }
    else if (argv && argc > 1) app.Load(argv[1]);
    if (argv) LocalFree(argv);
    MSG msg{};
    while (GetMessageW(&msg, nullptr, 0, 0) > 0) {
        if (msg.message == WM_KEYDOWN) {
            bool ctrl = GetKeyState(VK_CONTROL) < 0, edit = GetFocus() == app.query;
            int command = 0;
            if (ctrl && msg.wParam == 'O') command = Open;
            else if (ctrl && msg.wParam == 'F') command = Find;
            else if (ctrl && msg.wParam == 'P') command = Print;
            else if (ctrl && msg.wParam == '0') command = Fit;
            else if (msg.wParam == VK_F3 || (edit && msg.wParam == VK_RETURN)) command = FindNext;
            else if (!edit && (msg.wParam == VK_RIGHT || msg.wParam == VK_NEXT)) command = Next;
            else if (!edit && (msg.wParam == VK_LEFT || msg.wParam == VK_PRIOR)) command = Previous;
            if (command) { app.Command(command); continue; }
        }
        TranslateMessage(&msg); DispatchMessageW(&msg);
    }
    return static_cast<int>(msg.wParam);
}
