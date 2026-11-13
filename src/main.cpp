#include <windows.h>
#include <windowsx.h>
#include <commdlg.h>
#include <commctrl.h>
#include <shellapi.h>
#include <shlobj.h>
#include <fpdfview.h>
#include <fpdf_text.h>
#include <fpdf_progressive.h>
#include "document-layout.h"
#include "text-geometry.h"
#include "document-outline.h"
#include "document-links.h"
#include "resource.h"
#include "reading-state.h"
#include "recent-files.h"
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

namespace {
constexpr UINT Ready = WM_APP + 1;
constexpr int BarHeight = 58;
enum { Open = 100, Previous, Next, Smaller, Larger, Fit, Find, Print, Query, FindNext, FindPrevious, Copy, Outline, OpenMenu, ForgetHistory };
constexpr int RecentBase = 4000;

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
struct Result {
    enum Kind { Image, Opened, Password, Failure, Search, Printed, Geometry, Copied, OutlineReady, LinksReady, LinkFollowed } kind = Failure;
    DocumentOutline outline;
    std::vector<PageLink> links;
    LinkTarget linkTarget;
    unsigned long long document = 0, revision = 0, viewVersion = 0;
    int page = 0, pages = 0, width = 0, height = 0, match = -1;
    double milliseconds = 0;
    double scale = 1;
    double pageWidth = 0, pageHeight = 0;
    bool copyAllowed = false;
    bool printAllowed = false;
    std::vector<TextBox> boxes;
    std::wstring text;
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
    std::atomic<unsigned long long> wantedDocument_{0};
    std::atomic<unsigned long long> searchRevision_{0};
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
    void Load(std::wstring path, std::wstring password, unsigned long long document, bool suppliedPassword = false) {
        wantedDocument_ = document;
        ++searchRevision_;
        ++revision_;
        Queue([this, path, password = std::move(password), document, suppliedPassword]() mutable {
            EraseSecretOnExit<wchar_t> wipePassword{password};
            if (cancelled_ || document != wantedDocument_) return;
            if (pdf_) { FPDF_CloseDocument(pdf_); pdf_ = nullptr; }
            document_ = document; searchPage_ = searchIndex_ = -1; lastQuery_.clear();
            auto encodedPassword = Utf8(password); EraseSecretOnExit<char> wipeEncoded{encodedPassword};
            pdf_ = FPDF_LoadDocument(Utf8(path).c_str(), encodedPassword.c_str());
            auto result = std::make_unique<Result>();
            if (!pdf_) {
                auto error = FPDF_GetLastError();
                result->kind = error == FPDF_ERR_PASSWORD ? Result::Password : Result::Failure;
                result->message = error == FPDF_ERR_FORMAT ? L"El archivo no es un PDF válido o está dañado." : L"No se pudo abrir el PDF.";
                if (error == FPDF_ERR_PASSWORD) result->message = suppliedPassword ? L"La contraseña es incorrecta. Volvé a intentarlo." : L"Este documento está protegido. Ingresá la contraseña.";
                if (error == FPDF_ERR_FILE) result->message = L"No se pudo abrir el archivo. Puede haberse movido o no estar disponible.";
            } else {
                result->kind = Result::Opened; result->pages = FPDF_GetPageCount(pdf_);
                auto permissions = FPDF_GetDocPermissions(pdf_);
                result->copyAllowed = (permissions & 16) != 0; result->printAllowed = (permissions & 4) != 0;
                if (result->pages <= 0) { result->kind = Result::Failure; result->message = L"El PDF no contiene páginas."; }
            }
            Send(std::move(result));
        });
    }
    void LoadOutline(unsigned long long document) {
        Queue([this, document] {
            if (!pdf_ || document != document_ || document != wantedDocument_ || cancelled_) return;
            auto result = std::make_unique<Result>(); result->kind = Result::OutlineReady;
            result->outline = ReadOutline(pdf_, [this, document] { return cancelled_ || document != wantedDocument_; });
            if (!cancelled_ && document == wantedDocument_) Send(std::move(result));
        });
    }
    void LoadLinks(int page, unsigned long long document) {
        Queue([this, page, document] {
            if (!pdf_ || document != document_ || document != wantedDocument_ || cancelled_) return;
            auto result = std::make_unique<Result>(); result->kind = Result::LinksReady; result->page = page;
            auto pdfPage = FPDF_LoadPage(pdf_, page);
            if (pdfPage) {
                result->links = ReadPageLinks(pdf_, pdfPage, [this, document] { return cancelled_ || document != wantedDocument_; });
                FPDF_ClosePage(pdfPage);
            }
            if (!cancelled_ && document == wantedDocument_) Send(std::move(result));
        });
    }
    void FollowLink(int page, double x, double y, unsigned long long document, unsigned long long selection) {
        Queue([this, page, x, y, document, selection] {
            if (!pdf_ || document != document_ || document != wantedDocument_ || cancelled_) return;
            auto result = std::make_unique<Result>(); result->kind = Result::LinkFollowed; result->revision = selection;
            result->linkTarget = ResolvePageLink(pdf_, page, x, y);
            if (!cancelled_ && document == wantedDocument_) Send(std::move(result));
        });
    }
    unsigned long long Render(std::vector<int> requested, int viewport, double zoom, unsigned long long document, unsigned long long viewVersion) {
        auto revision = ++revision_;
        Queue([this, requested, viewport, zoom, revision, document, viewVersion] {
          for (int page : requested) {
            if (cancelled_ || !pdf_ || document != document_ || revision != revision_) return;
            LARGE_INTEGER start{}, end{}, frequency{};
            QueryPerformanceFrequency(&frequency); QueryPerformanceCounter(&start);
            auto result = std::make_unique<Result>(); result->revision = revision; result->viewVersion = viewVersion; result->page = page;
            FPDF_PAGE pdfPage = FPDF_LoadPage(pdf_, page);
            if (!pdfPage) { result->message = L"No se pudo leer la página."; Send(std::move(result)); continue; }
            double width = FPDF_GetPageWidth(pdfPage), height = FPDF_GetPageHeight(pdfPage);
            if (!(width > 0 && height > 0 && std::isfinite(width) && std::isfinite(height))) {
                FPDF_ClosePage(pdfPage); result->message = L"Dimensiones de página inválidas."; Send(std::move(result)); continue;
            }
            double scale = zoom > 0 ? zoom : std::max(100, viewport - 64) / width;
            result->pageWidth = width; result->pageHeight = height;
            double w = std::max(1.0, width * scale), h = std::max(1.0, height * scale);
            // Bound allocations before conversion to integers; never render giant pages unchecked.
            double reduction = std::min(1.0, std::min(16000.0 / std::max(w, h), std::sqrt(12000000.0 / (w * h))));
            result->width = std::max(1, static_cast<int>(w * reduction));
            result->height = std::max(1, static_cast<int>(h * reduction));
            result->scale = result->width / width;
            result->pixels.resize(static_cast<size_t>(result->width) * result->height * 4);
            FPDF_BITMAP bitmap = FPDFBitmap_CreateEx(result->width, result->height, FPDFBitmap_BGRA, result->pixels.data(), result->width * 4);
            if (bitmap) {
                FPDFBitmap_FillRect(bitmap, 0, 0, result->width, result->height, 0xffffffff);
                struct PauseContext { PdfWorker* worker; unsigned long long revision; } context{this, revision};
                IFSDK_PAUSE pause{}; pause.version = 1; pause.user = &context;
                pause.NeedToPauseNow = [](IFSDK_PAUSE* value) -> FPDF_BOOL {
                    auto state = static_cast<PauseContext*>(value->user);
                    return state->worker->cancelled_ || state->revision != state->worker->revision_;
                };
                int state = FPDF_RenderPageBitmap_Start(bitmap, pdfPage, 0, 0, result->width, result->height, 0, FPDF_ANNOT, &pause);
                while (state == FPDF_RENDER_TOBECONTINUED && !cancelled_ && revision == revision_)
                    state = FPDF_RenderPage_Continue(pdfPage, &pause);
                FPDF_RenderPage_Close(pdfPage);
                FPDFBitmap_Destroy(bitmap);
                if (state == FPDF_RENDER_DONE) result->kind = Result::Image;
                else result->message = L"No se pudo dibujar la página.";
            } else result->message = L"No hay memoria para dibujar la página.";
            FPDF_ClosePage(pdfPage); QueryPerformanceCounter(&end);
            result->milliseconds = 1000.0 * (end.QuadPart - start.QuadPart) / frequency.QuadPart;
            if (revision == revision_) Send(std::move(result));
          }
        });
        return revision;
    }
    void CancelSearch() { ++searchRevision_; }
    void Search(std::wstring query, int page, unsigned long long document, bool backwards = false) {
        auto searchRevision = ++searchRevision_;
        Queue([this, query, page, document, backwards, searchRevision] {
            if (cancelled_ || !pdf_ || document != document_ || query.empty()) return;
            auto result = std::make_unique<Result>(); result->kind = Result::Search;
            result->text = query;
            int count = FPDF_GetPageCount(pdf_);
            int first = query == lastQuery_ && searchPage_ == page ? searchIndex_ + (backwards ? -1 : 1) : (backwards ? -1 : 0);
            bool beforeStart = backwards && query == lastQuery_ && searchPage_ == page && searchIndex_ == 0;
            for (int step = 0; step <= count && !cancelled_ && document == wantedDocument_ && searchRevision == searchRevision_; ++step) {
                if (step == 0 && beforeStart) continue;
                int index = (page + (backwards ? -step : step) + count) % count;
                FPDF_PAGE pdfPage = FPDF_LoadPage(pdf_, index);
                if (!pdfPage) continue;
                FPDF_TEXTPAGE text = FPDFText_LoadPage(pdfPage);
                auto find = text ? FPDFText_FindStart(text, reinterpret_cast<FPDF_WIDESTRING>(query.c_str()), 0, step == 0 ? first : (backwards ? -1 : 0)) : nullptr;
                bool found = find && (backwards ? FPDFText_FindPrev(find) : FPDFText_FindNext(find));
                int match = found ? FPDFText_GetSchResultIndex(find) : -1;
                if (found) {
                    result->pageWidth = FPDF_GetPageWidth(pdfPage); result->pageHeight = FPDF_GetPageHeight(pdfPage);
                    int rectangles = FPDFText_CountRects(text, match, FPDFText_GetSchCount(find));
                    for (int i = 0; i < rectangles; ++i) {
                        double left, right, top, bottom;
                        if (FPDFText_GetRect(text, i, &left, &top, &right, &bottom)) result->boxes.push_back(PageBox(pdfPage, left, right, bottom, top));
                    }
                }
                if (find) FPDFText_FindClose(find);
                if (text) FPDFText_ClosePage(text);
                FPDF_ClosePage(pdfPage);
                if (found) {
                    result->page = index; result->match = match;
                    searchPage_ = index; searchIndex_ = match; lastQuery_ = query; break;
                }
            }
            if (searchRevision == searchRevision_) Send(std::move(result));
        });
    }
    void TextGeometry(int page, unsigned long long document) {
        Queue([this, page, document] {
            if (cancelled_ || !pdf_ || document != document_ || document != wantedDocument_) return;
            auto result = std::make_unique<Result>(); result->kind = Result::Geometry; result->page = page;
            result->copyAllowed = (FPDF_GetDocPermissions(pdf_) & 16) != 0;
            FPDF_PAGE pdfPage = FPDF_LoadPage(pdf_, page);
            FPDF_TEXTPAGE text = pdfPage ? FPDFText_LoadPage(pdfPage) : nullptr;
            int count = text ? FPDFText_CountChars(text) : 0;
            if (count > 200000) result->message = L"Esta página tiene demasiado texto para seleccionar.";
            else if (count > 0) {
                result->boxes.resize(count);
                for (int i = 0; i < count && !cancelled_ && document == wantedDocument_; ++i) {
                    double left, right, bottom, top;
                    if (FPDFText_GetCharBox(text, i, &left, &right, &bottom, &top)) result->boxes[i] = PageBox(pdfPage, left, right, bottom, top);
                }
            }
            if (text) FPDFText_ClosePage(text);
            if (pdfPage) FPDF_ClosePage(pdfPage);
            if (!cancelled_ && document == wantedDocument_) Send(std::move(result));
        });
    }
    void CopyText(int firstPage, int firstChar, int lastPage, int lastChar, unsigned long long document, unsigned long long selection) {
        Queue([this, firstPage, firstChar, lastPage, lastChar, document, selection] {
            if (cancelled_ || !pdf_ || document != document_ || document != wantedDocument_) return;
            auto result = std::make_unique<Result>(); result->kind = Result::Copied; result->revision = selection;
            if (!(FPDF_GetDocPermissions(pdf_) & 16)) result->message = L"El documento no permite copiar texto con esta contraseña.";
            else {
                for (int i = firstPage; i <= lastPage && !cancelled_ && document == wantedDocument_; ++i) {
                    FPDF_PAGE pdfPage = FPDF_LoadPage(pdf_, i);
                    FPDF_TEXTPAGE text = pdfPage ? FPDFText_LoadPage(pdfPage) : nullptr;
                    if (!text) result->message = L"No se pudo leer todo el texto seleccionado.";
                    int total = text ? FPDFText_CountChars(text) : 0;
                    int start = i == firstPage ? firstChar : 0;
                    int end = i == lastPage ? std::min(total, lastChar + 1) : total;
                    int count = std::max(0, end - start);
                    if (count > 1000000 || result->text.size() + count > 1000000) result->message = L"La selección es demasiado grande para copiar de una vez.";
                    if (result->message.empty() && count) {
                        std::vector<unsigned short> buffer(static_cast<size_t>(count) + 1);
                        int written = FPDFText_GetText(text, start, count, buffer.data());
                        if (written > 1) {
                            if (!result->text.empty()) result->text += L"\r\n";
                            result->text.append(reinterpret_cast<wchar_t*>(buffer.data()), written - 1);
                        }
                    }
                    if (text) FPDFText_ClosePage(text);
                    if (pdfPage) FPDF_ClosePage(pdfPage);
                    if (!result->message.empty()) { result->text.clear(); break; }
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
    HWND dialog = CreateWindowExW(WS_EX_DLGMODALFRAME, L"ChucaoPassword", L"Contraseña del PDF", WS_CAPTION | WS_SYSMENU | WS_POPUP,
        bounds.left + 100, bounds.top + 120, 410, 210, owner, nullptr, GetModuleHandleW(nullptr), &state);
    if (!dialog) return false;
    HWND label = CreateWindowW(L"STATIC", prompt.c_str(), WS_CHILD | WS_VISIBLE, 18, 14, 360, 42, dialog, nullptr, nullptr, nullptr);
    state.edit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_PASSWORD | ES_AUTOHSCROLL, 18, 64, 360, 28, dialog, nullptr, nullptr, nullptr);
    SendMessageW(state.edit, EM_SETLIMITTEXT, 256, 0);
    CreateWindowW(L"BUTTON", L"Abrir", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_DEFPUSHBUTTON, 198, 112, 85, 30, dialog, reinterpret_cast<HMENU>(IDOK), nullptr, nullptr);
    CreateWindowW(L"BUTTON", L"Cancelar", WS_CHILD | WS_VISIBLE | WS_TABSTOP, 293, 112, 85, 30, dialog, reinterpret_cast<HMENU>(IDCANCEL), nullptr, nullptr);
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

LRESULT CALLBACK OutlineProc(HWND, UINT, WPARAM, LPARAM);

class Reader {
public:
    HWND window = nullptr, query = nullptr, outlineWindow = nullptr, outlineTree = nullptr, outlineLabel = nullptr;
    DocumentOutline outline;
    bool outlineLoaded = false, outlinePending = false, outlineTest = false;
    int outlineChecks = 0;
    bool linksTest = false;
    int linkChecks = 0, linkStep = 0;
    int webOpenAttempts = 0;
    std::int64_t linkTestScroll = 0;
    int linkTestHorizontal = 0;
    bool passwordTest = false, canCopy = false, canPrint = false;
    int passwordStage = 0, passwordAttempts = 0, passwordChecks = 0;
    std::wstring passwordTestRoot;
    HFONT font = nullptr;
    HFONT welcomeFont = nullptr;
    HBITMAP welcomeImage = nullptr;
    bool readingStateEnabled = true, readingStateWrites = true, stateConfigured = false, viewReady = false;
    std::wstring readingStateFile;
    std::optional<ReadingState> pendingReadingState;
    int readingStateTest = 0, readingStateStep = 0, readingStateChecks = 0;
    bool readingStateFound = false;
    int stateAuthenticationAttempts = 0;
    bool savedOnClose = false;
    bool suppressCurrentState = false;
    std::vector<ReadingState> recentEntries;
    bool recentTest = false;
    int recentStep = 0, recentChecks = 0;
    std::wstring recentOther, recentMissing;
    std::unique_ptr<PdfWorker> worker;
    struct CachedPage { std::unique_ptr<Result> image; unsigned long long version = 0, used = 0; };
    std::map<int, CachedPage> cache;
    DocumentLayout layout;
    std::vector<int> planned;
    unsigned long long viewVersion = 0, plannedVersion = 0, useClock = 0;
    int renderWidth = 0;
    double renderZoom = -1;
    size_t cacheBytes = 0;
    static constexpr size_t CacheLimit = 96u * 1024u * 1024u;
    unsigned long long document = 0, revision = 0;
    int page = 0, pages = 0;
    std::int64_t scroll = 0;
    int horizontal = 0;
    double zoom = 0;
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
    std::wstring benchmarkReport;
    bool navigationTest = false;
    bool textTest = false;
    int textStep = 0, textChecks = 0;
    std::wstring crossPageExpected;
    int navigationStep = 0;
    size_t navigationHits = 0;
    std::chrono::steady_clock::time_point started;
    double shellMs = 0, openedMs = 0;
    void PopulateOutline() {
        if (!outlineTree) return;
        SendMessageW(outlineTree, WM_SETREDRAW, FALSE, 0);
        TreeView_DeleteAllItems(outlineTree);
        std::vector<HTREEITEM> items;
        for (const auto& entry : outline.entries) {
            auto label = entry.title + (entry.page >= 0 ? L"  ·  " + std::to_wstring(entry.page + 1) : L"  (sin destino interno)");
            TVINSERTSTRUCTW item{};
            item.hParent = entry.parent >= 0 ? items[entry.parent] : TVI_ROOT;
            item.hInsertAfter = TVI_LAST; item.item.mask = TVIF_TEXT | TVIF_PARAM;
            item.item.pszText = &label[0]; item.item.lParam = entry.page;
            items.push_back(TreeView_InsertItem(outlineTree, &item));
        }
        for (size_t i = 0; i < items.size(); ++i) if (outline.entries[i].parent < 0) TreeView_Expand(outlineTree, items[i], TVE_EXPAND);
        SendMessageW(outlineTree, WM_SETREDRAW, TRUE, 0); InvalidateRect(outlineTree, nullptr, TRUE);
        const wchar_t* label = !outlineLoaded ? (outlinePending ? L"Cargando índice…" : L"Abrí un PDF para consultar su índice.") :
            outline.entries.empty() ? L"Este documento no tiene índice." :
            outline.limited ? L"Índice limitado. Enter o doble clic para ir." : L"Enter o doble clic para ir al capítulo.";
        SetWindowTextW(outlineLabel, label);
    }
    void RequestOutline() {
        if (pages && !outlineLoaded && !outlinePending) {
            outlinePending = true; PopulateOutline(); worker->LoadOutline(document);
        }
    }
    void ToggleOutline() {
        if (outlineWindow && IsWindowVisible(outlineWindow)) { ShowWindow(outlineWindow, SW_HIDE); SetFocus(window); return; }
        if (!outlineWindow) {
            outlineWindow = CreateWindowExW(WS_EX_TOOLWINDOW, L"ChucaoOutline", L"Índice del documento", WS_OVERLAPPEDWINDOW,
                CW_USEDEFAULT, CW_USEDEFAULT, 360, 550, window, nullptr, GetModuleHandleW(nullptr), this);
            if (!outlineWindow) { status = L"No se pudo abrir el índice."; Redraw(); return; }
            SendMessageW(outlineWindow, WM_SIZE, 0, 0);
        }
        RequestOutline(); PopulateOutline();
        if (benchmarkReport.empty()) { ShowWindow(outlineWindow, SW_SHOW); SetFocus(outlineTree); }
    }
    void NavigateOutline() {
        auto selectedItem = TreeView_GetSelection(outlineTree);
        TVITEMW item{}; item.mask = TVIF_PARAM; item.hItem = selectedItem;
        if (!selectedItem || !TreeView_GetItem(outlineTree, &item)) return;
        int target = static_cast<int>(item.lParam);
        if (target >= 0 && target < pages) { ClearSelection(); ChangePage(target); }
        else { status = L"Esta entrada no tiene un destino dentro del documento."; Redraw(); }
    }
    void CheckOutline() {
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
        NMHDR activation{outlineTree, 0, NM_DBLCLK};
        SendMessageW(outlineWindow, WM_NOTIFY, 0, reinterpret_cast<LPARAM>(&activation));
        if (page != 1) { BenchmarkResult(false); return; }
        TreeView_SelectItem(outlineTree, root);
        NMTVKEYDOWN enter{}; enter.hdr.hwndFrom = outlineTree; enter.hdr.code = TVN_KEYDOWN; enter.wVKey = VK_RETURN;
        SendMessageW(outlineWindow, WM_NOTIFY, 0, reinterpret_cast<LPARAM>(&enter));
        if (page != 0) { BenchmarkResult(false); return; }
        TreeView_SelectItem(outlineTree, child); NavigateOutline();
        TreeView_SelectItem(outlineTree, TreeView_GetNextSibling(outlineTree, root)); NavigateOutline();
        if (page != 1) { BenchmarkResult(false); return; }
        ++outlineChecks;
        if (outlineChecks == 1) {
            Load(path); // Clear the old tree and reject late outline results on reopen.
            if (TreeView_GetCount(outlineTree) != 0) { BenchmarkResult(false); return; }
            auto stale = std::make_unique<Result>(); stale->kind = Result::OutlineReady; stale->document = document - 1;
            Receive(std::move(stale)); RequestOutline();
        } else {
            Load(path.substr(0, path.find_last_of(L"\\/") + 1) + L"sample.pdf");
        }
    }
    void BenchmarkResult(bool success) {
        if (benchmarkReport.empty()) return;
        double readyMs = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - started).count();
        char data[768]{};
        Endpoint a, b; SelectionRange(a, b);
        int size = sprintf_s(data, "{\"success\":%s,\"shell_ms\":%.3f,\"opened_ms\":%.3f,\"ready_ms\":%.3f,\"pages\":%d,\"navigation_steps\":%d,\"cache_hits\":%zu,\"cache_bytes\":%zu,\"outline_checks\":%d,\"link_checks\":%d,\"password_checks\":%d,\"state_checks\":%d,\"recent_checks\":%d,\"text_checks\":%d,\"text_step\":%d,\"selection_first\":%d,\"selection_last\":%d}\n", success ? "true" : "false", shellMs, openedMs, readyMs, pages, navigationStep, navigationHits, cacheBytes, outlineChecks, linkChecks, passwordChecks, readingStateChecks, recentChecks, textChecks, textStep, a.character, b.character);
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
            zoom = 3; Reflow(); Render();
            ScrollHorizontal(static_cast<int>(layout.Left(0, ViewWidth()) + point.x * layout.Width(0) - ViewWidth() / 2.0));
            int sx = layout.Left(0, ViewWidth(), horizontal) + static_cast<int>(point.x * layout.Width(0));
            int sy = BarHeight + static_cast<int>(layout.Top(0) - scroll + point.y * layout.Height(0)); TextPoint hit;
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
            int x = layout.Left(0, ViewWidth(), horizontal) + static_cast<int>(selectionStart.x * layout.Width(0));
            int y = BarHeight + static_cast<int>(layout.Top(0) - scroll + selectionStart.y * layout.Height(0));
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
                textStep = -1; zoom = 3; Reflow(); Render(); return;
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
        case 2: navigationStep = 3; zoom = 4; Reflow(); Render(); break;
        case 3: {
            auto before = revision;
            SendMessageW(window, WM_HSCROLL, SB_RIGHT, 0);
            int right = layout.Left(page, ViewWidth(), horizontal) + layout.Width(page);
            if (horizontal <= 0 || horizontal != layout.MaxHorizontal(ViewWidth()) || right > ViewWidth() - DocumentLayout::Gap) { BenchmarkResult(false); return; }
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
        case 7:
            navigationStep = 8;
            // Reopen with queued work and verify that old-document results are ignored.
            Load(path); break;
        case 8: navigationStep = 9; BenchmarkResult(true); break;
        }
        if (navigationStep < 9) PostMessageW(window, WM_TIMER, 3, 0);
    }

    void Redraw() { InvalidateRect(window, nullptr, FALSE); }
    HMENU BuildOpenMenu() {
        recentEntries.clear();
        if (ConfigureReadingState()) {
            std::optional<ReadingState> current;
            if (pages && viewReady && !pendingReadingState && !suppressCurrentState) current = CurrentReadingState();
            recentEntries = RecentFiles(ReadingStateStore(readingStateFile).Read(), current);
        }
        HMENU menu = CreatePopupMenu(); if (!menu) return nullptr;
        AppendMenuW(menu, MF_STRING, Open, L"Abrir PDF…\tCtrl+O");
        AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
        AppendMenuW(menu, MF_STRING | MF_DISABLED, 0, recentEntries.empty() ? L"No hay archivos recientes" : L"Recientes");
        for (size_t i = 0; i < recentEntries.size(); ++i) AppendMenuW(menu, MF_STRING, RecentBase + i, RecentLabel(recentEntries[i].path, i).c_str());
        AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
        AppendMenuW(menu, MF_STRING | (recentEntries.empty() ? MF_DISABLED : 0), ForgetHistory, L"Olvidar historial y posiciones…");
        return menu;
    }
    void ShowOpenMenu() {
        HMENU menu = BuildOpenMenu();
        if (!menu) { status = L"No se pudo abrir el menú."; Redraw(); return; }
        RECT button{}; GetWindowRect(GetDlgItem(window, OpenMenu), &button);
        UINT command = TrackPopupMenuEx(menu, TPM_RETURNCMD | TPM_NONOTIFY | TPM_LEFTALIGN | TPM_TOPALIGN, button.left, button.bottom, window, nullptr);
        DestroyMenu(menu); if (command) Command(static_cast<int>(command));
    }
    bool ClearReadingHistory(bool confirm = true) {
        if (!readingStateWrites || !ConfigureReadingState() || (!confirm && !recentTest)) return false;
        if (confirm && MessageBoxW(window, L"¿Olvidar la lista de recientes y las posiciones guardadas?", L"Historial de Chucao", MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2) != IDYES) return false;
        if (!ReadingStateStore(readingStateFile).Clear()) { status = L"No se pudo guardar el cambio del historial."; Redraw(); return false; }
        recentEntries.clear(); suppressCurrentState = true; pendingReadingState.reset();
        status = L"Historial y posiciones olvidados."; Redraw(); return true;
    }
    void CheckRecents() {
        if (!recentTest || !pages || pendingReadingState || !cache.count(layout.PageAt(scroll))) return;
        HMENU menu = BuildOpenMenu();
        bool ok = menu && GetMenuItemCount(menu) == static_cast<int>(recentEntries.size()) + 5;
        if (recentStep == 0) {
            ok = ok && recentEntries.size() == 3 && recentEntries.front().path == ReadingStateStore::NormalizePath(path);
            for (size_t i = 0; i < recentEntries.size(); ++i) {
                ok = ok && GetMenuItemID(menu, static_cast<int>(i) + 3) == RecentBase + i;
                if (recentEntries[i].path != ReadingStateStore::NormalizePath(path)) {
                    if (recentEntries[i].path.find(L"ausente.pdf") != std::wstring::npos) recentMissing = recentEntries[i].path;
                    else recentOther = recentEntries[i].path;
                }
            }
            ok = ok && !recentOther.empty() && !recentMissing.empty();
        } else ok = ok && path == recentOther;
        if (menu) DestroyMenu(menu);
        if (!ok) { BenchmarkResult(false); return; }
        ++recentChecks;
        if (recentStep == 0) { recentStep = 1; OpenRecentForTest(recentOther); }
        else if (recentStep == 1) { recentStep = 2; OpenRecentForTest(recentMissing); }
        else {
            SaveTestImage(benchmarkReport + L".bmp", window);
            if (!ClearReadingHistory(false) || !ReadingStateStore(readingStateFile).Read().empty() || SaveReadingState() || !pages) { BenchmarkResult(false); return; }
            menu = BuildOpenMenu(); ok = menu && recentEntries.empty() && (GetMenuState(menu, ForgetHistory, MF_BYCOMMAND) & MF_DISABLED);
            if (menu) DestroyMenu(menu);
            ++recentChecks; recentStep = 4; BenchmarkResult(ok);
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
        zoom = state.zoom; layout.Rebuild(ViewWidth(), zoom);
        ScrollTo(layout.Top(state.page) + static_cast<std::int64_t>(state.vertical * layout.Height(state.page)) + state.gap, false);
        ScrollHorizontal(static_cast<int>(layout.Left(state.page, ViewWidth()) + state.horizontal * layout.Width(state.page) - ViewWidth() / 2.0));
        PageStatus();
    }
    bool PointOnPage(int x, int y, TextPoint& point, bool clamp = false) {
        if (!pages || (!clamp && (y < BarHeight || y >= BarHeight + ViewHeight()))) return false;
        auto documentY = scroll + y - BarHeight;
        int index = layout.PageAt(documentY);
        int left = layout.Left(index, ViewWidth(), horizontal);
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
        ClearSelection(); zoom = previous.zoom; Reflow();
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
    void EndSelection() { dragging = false; KillTimer(window, 4); if (GetCapture() == window) ReleaseCapture(); }
    void FinishSelection(int x, int y) {
        if (!dragging) return;
        MoveSelection(x, y); bool clicked = !selected; TextPoint point;
        bool onPage = PointOnPage(x, y, point); EndSelection();
        if (clicked && onPage) ActivateLink(point);
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
    int ViewHeight() const { RECT rc{}; GetClientRect(window, &rc); return std::max<int>(1, rc.bottom - BarHeight - 26); }
    int ViewWidth() const { RECT rc{}; GetClientRect(window, &rc); return rc.right; }
    std::int64_t MaxScroll() const { return std::max<std::int64_t>(0, layout.Total() - ViewHeight()); }
    void ScrollHorizontal(int value) {
        horizontal = std::max(0, std::min(value, layout.MaxHorizontal(ViewWidth())));
        SCROLLINFO info{sizeof(info), SIF_RANGE | SIF_PAGE | SIF_POS | SIF_DISABLENOSCROLL};
        info.nMax = layout.HorizontalExtent(ViewWidth()) - 1;
        info.nPage = ViewWidth(); info.nPos = horizontal;
        SetScrollInfo(window, SB_HORZ, &info, TRUE); Redraw();
    }
    void PageStatus() { if (pages) status = L"Página " + std::to_wstring(page + 1) + L" de " + std::to_wstring(pages); }
    void ScrollTo(std::int64_t value, bool request = true) {
        RECT rc{}; GetClientRect(window, &rc);
        ScrollHorizontal(horizontal);
        scroll = std::max<std::int64_t>(0, std::min(value, MaxScroll()));
        int previous = page;
        if (pages) page = layout.PageAt(scroll + std::min(40, ViewHeight() / 2));
        if (page != previous) PageStatus();
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
    void Reflow(bool keepAnchor = true) {
        int anchor = pages ? layout.PageAt(scroll) : 0;
        double fraction = pages && layout.Count() ? (scroll - layout.Top(anchor)) / static_cast<double>(layout.Height(anchor)) : 0;
        double center = pages && layout.Count() ? (ViewWidth() / 2.0 - layout.Left(anchor, ViewWidth(), horizontal)) / layout.Width(anchor) : 0.5;
        layout.Rebuild(ViewWidth(), zoom);
        if (keepAnchor && pages) scroll = layout.Top(anchor) + static_cast<std::int64_t>(fraction * layout.Height(anchor));
        if (keepAnchor && pages) horizontal = static_cast<int>(layout.Left(anchor, ViewWidth()) + center * layout.Width(anchor) - ViewWidth() / 2.0);
        ScrollTo(scroll, false);
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
        int width = ViewWidth();
        if (renderWidth != width || renderZoom != zoom) {
            renderWidth = width; renderZoom = zoom; ++viewVersion;
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
        revision = worker->Render(std::move(missing), width, zoom, document, viewVersion);
        Redraw();
    }
    void Load(const std::wstring& file, const std::wstring& password = L"", bool retry = false) {
        if (!worker) worker = std::make_unique<PdfWorker>(window);
        if (!retry) {
            SaveReadingState(); viewReady = false; pendingReadingState.reset();
            suppressCurrentState = false;
            pageLinks.clear(); pendingLinks.clear(); linkHistory.clear();
            outline = {}; outlineLoaded = outlinePending = false; PopulateOutline();
            EndSelection(); ClearSelection(); textPages.clear(); pendingText.clear(); searchPage = -1; searchBoxes.clear();
            path = file; ++document; pages = 0; canCopy = canPrint = false; page = 0; scroll = 0; horizontal = 0; zoom = 0; searching = false;
            cache.clear(); cacheBytes = 0; layout.Reset(0); layout.Rebuild(ViewWidth(), zoom);
            planned.clear(); renderWidth = 0; renderZoom = -1; ++viewVersion;
            ScrollHorizontal(0);
        }
        status = L"Abriendo documento…"; Redraw(); worker->Load(path, password, document, retry);
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
        page = target; PageStatus(); ScrollTo(std::max<std::int64_t>(0, layout.Top(target) - DocumentLayout::Gap));
    }
    void Search(bool backwards = false) {
        if (!pages || searching) return;
        auto value = Text(query); if (value.empty()) { SetFocus(query); return; }
        ClearSelection(); searching = true; status = L"Buscando…"; Redraw(); worker->Search(value, page, document, backwards);
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
    void Command(int id) {
        if (id == OpenMenu) { ShowOpenMenu(); return; }
        if (id == ForgetHistory) { ClearReadingHistory(); return; }
        pendingReadingState.reset();
        ++linkEpoch;
        if (id >= RecentBase && id < RecentBase + static_cast<int>(recentEntries.size())) {
            auto selectedPath = recentEntries[id - RecentBase].path; Load(selectedPath); return;
        }
        switch (id) {
        case Open: ChooseFile(); break;
        case Previous: ChangePage(page - 1); break;
        case Next: ChangePage(page + 1); break;
        case Fit: zoom = 0; Reflow(); Render(); break;
        case Larger: case Smaller: {
            // Start manual zoom from the current fit-width image scale.
            if (!zoom) {
                auto found = cache.find(page);
                zoom = found != cache.end() ? found->second.image->scale : 1.0;
            }
            zoom = std::max(0.25, std::min(4.0, zoom * (id == Larger ? 1.2 : 1 / 1.2)));
            Reflow(); Render(); break;
        }
        case Find: SetFocus(query); SendMessageW(query, EM_SETSEL, 0, -1); break;
        case FindNext: Search(); break;
        case FindPrevious: Search(true); break;
        case Copy: CopySelection(); break;
        case Outline: ToggleOutline(); break;
        case Print: PrintFile(); break;
        }
    }
    void Receive(std::unique_ptr<Result> result) {
        if (result->kind == Result::Printed) printing = false;
        if (result->document != document) return;
        if (result->kind == Result::Failure && result->revision && result->revision != revision) return;
        switch (result->kind) {
        case Result::Opened:
            canCopy = result->copyAllowed; canPrint = result->printAllowed;
            openedMs = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - started).count();
            pages = result->pages; layout.Reset(pages);
            if (ConfigureReadingState()) pendingReadingState = ReadingStateStore(readingStateFile).Find(path, pages);
            readingStateFound = pendingReadingState.has_value();
            if (pendingReadingState) ApplyReadingState(*pendingReadingState);
            else { layout.Rebuild(ViewWidth(), zoom); ScrollTo(0, false); }
            SetWindowTextW(window, (L"Chucao — " + path.substr(path.find_last_of(L"\\/") + 1)).c_str()); PageStatus(); Render();
            if (outlineWindow && (IsWindowVisible(outlineWindow) || outlineTest)) RequestOutline();
            break;
        case Result::OutlineReady:
            outlinePending = false; outlineLoaded = true; outline = std::move(result->outline); PopulateOutline();
            if (outlineTest) CheckOutline();
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
        case Result::LinkFollowed:
            if (result->revision != linkEpoch || selected || dragging) break;
            if (result->linkTarget.page >= 0) {
                int anchor = layout.PageAt(scroll);
                if (linkHistory.size() == 64) linkHistory.erase(linkHistory.begin());
                linkHistory.push_back({anchor, (scroll - layout.Top(anchor)) / static_cast<double>(layout.Height(anchor)),
                    (ViewWidth() / 2.0 - layout.Left(anchor, ViewWidth(), horizontal)) / layout.Width(anchor), zoom});
                auto target = result->linkTarget;
                ClearSelection(); layout.Learn(target.page, target.width, target.height); Reflow();
                ScrollTo(layout.Top(target.page) + static_cast<std::int64_t>(target.y * layout.Height(target.page)) - DocumentLayout::Gap);
                ScrollHorizontal(static_cast<int>(layout.Left(target.page, ViewWidth()) + target.x * layout.Width(target.page) - DocumentLayout::Gap));
                PageStatus();
            } else if (!result->linkTarget.webUrl.empty()) OpenWebLink(result->linkTarget.webUrl);
            else if (result->linkTarget.hit) { status = L"Este enlace no es compatible. Se admiten destinos internos y direcciones HTTP/HTTPS."; Redraw(); }
            CheckLinks(Result::LinkFollowed, result->linkTarget);
            break;
        case Result::Image:
            // A completed bitmap from the same zoom/width is useful even if the
            // scroll position has changed since it was queued.
            if (result->viewVersion != viewVersion) return;
            {
                int index = result->page;
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
                if (textTest) CheckText(Result::Image);
                else if (navigationTest) { PostMessageW(window, WM_TIMER, 3, 0); }
                else if (outlineTest) { if (!outlineWindow) ToggleOutline(); }
                else if (linksTest) CheckLinks(Result::Image);
                else if (passwordTest) CheckPasswordImage();
                else if (readingStateTest) CheckReadingState();
                else if (recentTest) CheckRecents();
                else BenchmarkResult(true);
            }
            break;
        case Result::Password: {
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
            else { status = L"Apertura cancelada."; Redraw(); }
            if (!password.empty()) SecureZeroMemory(&password[0], password.size() * sizeof(wchar_t));
            break;
        }
        case Result::Search:
            searching = false;
            if (result->text != Text(query)) break;
            searchBoxes.clear(); searchPage = -1;
            if (result->match >= 0) {
                layout.Learn(result->page, result->pageWidth, result->pageHeight); Reflow();
                searchPage = result->page; searchMatch = result->match; searchBoxes = std::move(result->boxes);
                double top = searchBoxes.empty() ? 0 : searchBoxes.front().top;
                ScrollTo(layout.Top(searchPage) + static_cast<std::int64_t>(top * layout.Height(searchPage)) - ViewHeight() / 3);
                if (!searchBoxes.empty()) ScrollHorizontal(static_cast<int>(layout.Left(searchPage, ViewWidth()) + searchBoxes.front().left * layout.Width(searchPage) - ViewWidth() / 3.0));
                status = L"Coincidencia en página " + std::to_wstring(result->page + 1) + L". F3: siguiente.";
            } else status = L"No se encontró el texto.";
            Redraw(); CheckText(Result::Search); break;
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
        case Result::Failure:
            status = result->message; Redraw();
            if (recentTest && recentStep == 2 && path == recentMissing) {
                HMENU menu = BuildOpenMenu();
                bool retained = false;
                for (const auto& entry : recentEntries) if (entry.path == recentMissing) retained = true;
                if (menu) DestroyMenu(menu);
                if (!retained) { BenchmarkResult(false); break; }
                ++recentChecks; recentStep = 3; OpenRecentForTest(recentOther);
            } else BenchmarkResult(false);
            break;
        }
    }
    void Draw(HDC target, RECT rc) {
        HDC dc = CreateCompatibleDC(target);
        HBITMAP buffer = CreateCompatibleBitmap(target, std::max<LONG>(1, rc.right), std::max<LONG>(1, rc.bottom));
        HGDIOBJ oldBuffer = SelectObject(dc, buffer);
        HBRUSH background = CreateSolidBrush(RGB(238, 240, 243)); FillRect(dc, &rc, background); DeleteObject(background);
        RECT bar{0, 0, rc.right, BarHeight}; FillRect(dc, &bar, reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1));
        SelectObject(dc, font); SetBkMode(dc, TRANSPARENT); SetTextColor(dc, RGB(72, 79, 92));
        if (pages) {
            IntersectClipRect(dc, 0, BarHeight, rc.right, rc.bottom - 26);
            int first = layout.PageAt(scroll), last = layout.PageAt(scroll + ViewHeight());
            SetStretchBltMode(dc, COLORONCOLOR); // Quick preview while a new zoom is rendered.
            for (int i = first; i <= last; ++i) {
                int width = layout.Width(i), height = layout.Height(i);
                int x = layout.Left(i, ViewWidth(), horizontal);
                int y = BarHeight + static_cast<int>(layout.Top(i) - scroll);
                RECT sheet{x, y, x + width, y + height};
                FillRect(dc, &sheet, reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1));
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
                    auto label = L"Página " + std::to_wstring(i + 1) + L" · Cargando…";
                    sheet.top += 24; DrawTextW(dc, label.c_str(), -1, &sheet, DT_CENTER | DT_TOP | DT_SINGLELINE);
                }
            }
            SelectClipRgn(dc, nullptr);
        } else if (!pages) {
            RECT empty{0, BarHeight, rc.right, rc.bottom - 26};
            FillRect(dc, &empty, reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1));
            if (!welcomeImage) welcomeImage = static_cast<HBITMAP>(LoadImageW(GetModuleHandleW(nullptr), MAKEINTRESOURCEW(IDB_CHUCAO_HOME), IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION));
            int available = std::max<LONG>(1, empty.bottom - empty.top);
            int size = std::min(224, std::max(100, available - 130));
            int top = empty.top + std::max(8, (available - size - 108) / 2);
            if (welcomeImage) {
                HDC picture = CreateCompatibleDC(dc); auto oldPicture = SelectObject(picture, welcomeImage);
                BITMAP bitmap{}; GetObjectW(welcomeImage, sizeof(bitmap), &bitmap);
                SetStretchBltMode(dc, HALFTONE); SetBrushOrgEx(dc, 0, 0, nullptr);
                StretchBlt(dc, (rc.right - size) / 2, top, size, size, picture, 0, 0, bitmap.bmWidth, bitmap.bmHeight, SRCCOPY);
                SelectObject(picture, oldPicture); DeleteDC(picture);
            }
            RECT title{24, top + size + 8, rc.right - 24, top + size + 42};
            SelectObject(dc, welcomeFont); SetTextColor(dc, RGB(22, 34, 49));
            DrawTextW(dc, L"Chucao", -1, &title, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
            SelectObject(dc, font); SetTextColor(dc, RGB(86, 97, 112));
            RECT tagline{24, title.bottom + 2, rc.right - 24, title.bottom + 28};
            DrawTextW(dc, L"Tu documento, sin distracciones.", -1, &tagline, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
            RECT action{24, tagline.bottom + 12, rc.right - 24, tagline.bottom + 38};
            DrawTextW(dc, L"Abrir PDF · Ctrl+O", -1, &action, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        }
        RECT footer{16, rc.bottom - 24, rc.right - 16, rc.bottom};
        FillRect(dc, &footer, background = CreateSolidBrush(RGB(238, 240, 243))); DeleteObject(background);
        DrawTextW(dc, status.c_str(), -1, &footer, DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS);
        BitBlt(target, 0, 0, rc.right, rc.bottom, dc, 0, 0, SRCCOPY);
        SelectObject(dc, oldBuffer); DeleteObject(buffer); DeleteDC(dc);
    }
    void Paint() {
        PAINTSTRUCT ps{}; HDC target = BeginPaint(window, &ps); RECT rc{}; GetClientRect(window, &rc);
        Draw(target, rc); EndPaint(window, &ps);
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
        if (control) {
            if (control == window) Draw(dc, rc);
            else FillRect(dc, &rc, reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1));
            for (HWND child = GetWindow(control, GW_CHILD); child; child = GetWindow(child, GW_HWNDNEXT)) {
                RECT bounds{}; GetWindowRect(child, &bounds);
                MapWindowPoints(nullptr, control, reinterpret_cast<POINT*>(&bounds), 2);
                int saved = SaveDC(dc); SetViewportOrgEx(dc, bounds.left, bounds.top, nullptr);
                IntersectClipRect(dc, 0, 0, bounds.right - bounds.left, bounds.bottom - bounds.top);
                SendMessageW(child, WM_PRINT, reinterpret_cast<WPARAM>(dc), PRF_CLIENT | PRF_NONCLIENT | PRF_ERASEBKGND);
                RestoreDC(dc, saved);
            }
        }
        else Draw(dc, rc);
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

LRESULT CALLBACK OutlineProc(HWND window, UINT message, WPARAM wp, LPARAM lp) {
    auto app = reinterpret_cast<Reader*>(GetWindowLongPtrW(window, GWLP_USERDATA));
    if (message == WM_NCCREATE) {
        app = static_cast<Reader*>(reinterpret_cast<CREATESTRUCTW*>(lp)->lpCreateParams);
        SetWindowLongPtrW(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(app));
    }
    if (!app) return DefWindowProcW(window, message, wp, lp);
    switch (message) {
    case WM_CREATE:
        app->outlineTree = CreateWindowExW(WS_EX_CLIENTEDGE, WC_TREEVIEWW, L"Índice", WS_CHILD | WS_VISIBLE | WS_TABSTOP |
            TVS_HASLINES | TVS_LINESATROOT | TVS_HASBUTTONS | TVS_SHOWSELALWAYS, 0, 0, 1, 1, window, nullptr, nullptr, nullptr);
        app->outlineLabel = CreateWindowW(L"STATIC", L"", WS_CHILD | WS_VISIBLE, 8, 0, 1, 1, window, nullptr, nullptr, nullptr);
        SendMessageW(app->outlineTree, WM_SETFONT, reinterpret_cast<WPARAM>(app->font), TRUE);
        SendMessageW(app->outlineLabel, WM_SETFONT, reinterpret_cast<WPARAM>(app->font), TRUE); return 0;
    case WM_SIZE: {
        RECT rc{}; GetClientRect(window, &rc);
        MoveWindow(app->outlineTree, 0, 0, rc.right, std::max<LONG>(1, rc.bottom - 44), TRUE);
        MoveWindow(app->outlineLabel, 8, std::max<LONG>(0, rc.bottom - 40), std::max<LONG>(1, rc.right - 16), 36, TRUE); return 0;
    }
    case WM_GETMINMAXINFO: reinterpret_cast<MINMAXINFO*>(lp)->ptMinTrackSize = {280, 240}; return 0;
    case WM_NOTIFY: {
        auto notification = reinterpret_cast<NMHDR*>(lp);
        if (notification->hwndFrom == app->outlineTree && (notification->code == NM_DBLCLK ||
            (notification->code == TVN_KEYDOWN && reinterpret_cast<NMTVKEYDOWN*>(lp)->wVKey == VK_RETURN))) app->NavigateOutline();
        return 0;
    }
    case WM_CLOSE: ShowWindow(window, SW_HIDE); SetFocus(app->window); return 0;
    case WM_DESTROY: app->outlineWindow = app->outlineTree = app->outlineLabel = nullptr; return 0;
    }
    return DefWindowProcW(window, message, wp, lp);
}

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
        app->welcomeFont = CreateFontW(-28, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
        SendMessageW(window, WM_SETICON, ICON_SMALL, reinterpret_cast<LPARAM>(LoadImageW(GetModuleHandleW(nullptr), MAKEINTRESOURCEW(IDI_CHUCAO), IMAGE_ICON, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), LR_SHARED)));
        struct Control { const wchar_t* text; int id, width; };
        Control buttons[] = {{L"Abrir ▾", OpenMenu, 82}, {L"‹", Previous, 34}, {L"›", Next, 34}, {L"−", Smaller, 34}, {L"+", Larger, 34}, {L"Ancho", Fit, 64}, {L"Índice", Outline, 64}, {L"Buscar", FindNext, 64}, {L"Imprimir", Print, 78}};
        int x = 14;
        for (const auto& button : buttons) {
            HWND control = CreateWindowW(L"BUTTON", button.text, WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_FLAT, x, 14, button.width, 30, window, reinterpret_cast<HMENU>(static_cast<INT_PTR>(button.id)), nullptr, nullptr);
            SendMessageW(control, WM_SETFONT, reinterpret_cast<WPARAM>(app->font), TRUE); x += button.width + 6;
        }
        app->query = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL, x + 8, 16, 220, 26, window, reinterpret_cast<HMENU>(Query), nullptr, nullptr);
        SendMessageW(app->query, WM_SETFONT, reinterpret_cast<WPARAM>(app->font), TRUE); SendMessageW(app->query, EM_SETLIMITTEXT, 256, 0);
        DragAcceptFiles(window, TRUE); return 0;
    }
    case WM_COMMAND:
        if (LOWORD(wp) == Query && HIWORD(wp) == EN_CHANGE) {
            app->searching = false; app->searchPage = -1; app->searchBoxes.clear();
            if (app->worker) app->worker->CancelSearch(); app->Redraw();
        } else app->Command(LOWORD(wp));
        return 0;
    case WM_LBUTTONDOWN: app->BeginSelection(GET_X_LPARAM(lp), GET_Y_LPARAM(lp)); return 0;
    case WM_MOUSEMOVE: app->MoveSelection(GET_X_LPARAM(lp), GET_Y_LPARAM(lp)); app->HoverLinks(GET_X_LPARAM(lp), GET_Y_LPARAM(lp)); return 0;
    case WM_LBUTTONUP: app->FinishSelection(GET_X_LPARAM(lp), GET_Y_LPARAM(lp)); return 0;
    case WM_CAPTURECHANGED: app->EndSelection(); return 0;
    case WM_SETCURSOR:
        if (reinterpret_cast<HWND>(wp) == window && LOWORD(lp) == HTCLIENT && app->pages) {
            POINT mouse{}; GetCursorPos(&mouse); ScreenToClient(window, &mouse); Reader::TextPoint point;
            if (app->PointOnPage(mouse.x, mouse.y, point)) { SetCursor(LoadCursor(nullptr, app->LinkAt(point) ? IDC_HAND : IDC_IBEAM)); return TRUE; }
        }
        break;
    case WM_DROPFILES: {
        wchar_t path[32768]{}; HDROP drop = reinterpret_cast<HDROP>(wp);
        if (DragQueryFileW(drop, 0, path, 32768)) app->Load(path);
        DragFinish(drop); return 0;
    }
    case Ready: app->Receive(std::unique_ptr<Result>(reinterpret_cast<Result*>(lp))); return 0;
    case WM_SIZE: app->Reflow(); if (wp != SIZE_MINIMIZED) SetTimer(window, 1, 120, nullptr); return 0;
    case WM_TIMER:
        if (wp == 4 && app->dragging) {
            POINT pointer{}; GetCursorPos(&pointer); ScreenToClient(window, &pointer);
            if (pointer.y < BarHeight + 20) { app->ScrollTo(app->scroll - 28); app->MoveSelection(pointer.x, pointer.y); }
            else if (pointer.y > BarHeight + app->ViewHeight() - 20) { app->ScrollTo(app->scroll + 28); app->MoveSelection(pointer.x, pointer.y); }
            if (pointer.x < 20) { app->ScrollHorizontal(app->horizontal - 28); app->MoveSelection(pointer.x, pointer.y); }
            else if (pointer.x > app->ViewWidth() - 20) { app->ScrollHorizontal(app->horizontal + 28); app->MoveSelection(pointer.x, pointer.y); }
        }
        else if (wp == 6) app->CheckReadingState();
        else if (wp == 3) app->CheckNavigation();
        else if (wp == 2) { KillTimer(window, 2); app->BenchmarkResult(false); }
        else { KillTimer(window, 1); app->Render(); }
        return 0;
    case WM_GETMINMAXINFO: reinterpret_cast<MINMAXINFO*>(lp)->ptMinTrackSize = {820, 400}; return 0;
    case WM_MOUSEWHEEL:
        app->pendingReadingState.reset();
        ++app->linkEpoch;
        if (GetKeyState(VK_CONTROL) < 0) app->Command(GET_WHEEL_DELTA_WPARAM(wp) > 0 ? Larger : Smaller);
        else if (GetKeyState(VK_SHIFT) < 0) app->ScrollHorizontal(app->horizontal - GET_WHEEL_DELTA_WPARAM(wp));
        else app->ScrollTo(app->scroll - GET_WHEEL_DELTA_WPARAM(wp)); return 0;
    case WM_MOUSEHWHEEL: app->pendingReadingState.reset(); ++app->linkEpoch; app->ScrollHorizontal(app->horizontal + GET_WHEEL_DELTA_WPARAM(wp)); return 0;
    case WM_HSCROLL: {
        app->pendingReadingState.reset();
        ++app->linkEpoch;
        SCROLLINFO info{sizeof(info), SIF_ALL}; GetScrollInfo(window, SB_HORZ, &info);
        int value = app->horizontal;
        switch (LOWORD(wp)) {
        case SB_LINELEFT: value -= 40; break; case SB_LINERIGHT: value += 40; break;
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
        case SB_LINEUP: value -= 40; break; case SB_LINEDOWN: value += 40; break;
        case SB_PAGEUP: value -= info.nPage; break; case SB_PAGEDOWN: value += info.nPage; break;
        case SB_TOP: value = 0; break; case SB_BOTTOM: value = app->MaxScroll(); break;
        case SB_THUMBPOSITION: case SB_THUMBTRACK:
            value = app->layout.Total() < 1000000000 ? info.nTrackPos : static_cast<std::int64_t>(app->MaxScroll() * (info.nTrackPos / (info.nMax - info.nPage + 1.0))); break;
        }
        app->ScrollTo(value); return 0;
    }
    case WM_ERASEBKGND: return 1;
    case WM_CLOSE:
        app->SaveReadingState(); app->savedOnClose = true; DestroyWindow(window); return 0;
    case WM_ENDSESSION: if (wp) app->SaveReadingState(); return 0;
    case WM_PAINT: app->Paint(); return 0;
    case WM_DESTROY: {
        if (!app->savedOnClose) app->SaveReadingState();
        app->worker.reset();
        if (app->outlineWindow) DestroyWindow(app->outlineWindow);
        MSG pending{}; while (PeekMessageW(&pending, window, Ready, Ready, PM_REMOVE)) delete reinterpret_cast<Result*>(pending.lParam);
        DeleteObject(app->font); DeleteObject(app->welcomeFont);
        if (app->welcomeImage) DeleteObject(app->welcomeImage);
        PostQuitMessage(0); return 0;
    }
    }
    return DefWindowProcW(window, message, wp, lp);
}
} // namespace

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int show) {
    auto started = std::chrono::steady_clock::now();
    int argc = 0; LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    bool navigationTest = argv && argc == 4 && std::wstring(argv[1]) == L"--benchmark-navigation";
    bool textTest = argv && argc == 4 && std::wstring(argv[1]) == L"--benchmark-text";
    bool outlineTest = argv && argc == 4 && std::wstring(argv[1]) == L"--benchmark-outline";
    bool linksTest = argv && argc == 4 && std::wstring(argv[1]) == L"--benchmark-links";
    bool passwordTest = argv && argc == 4 && std::wstring(argv[1]) == L"--benchmark-passwords";
    int stateTest = argv && argc == 5 ? (std::wstring(argv[1]) == L"--benchmark-state-write" ? 1 : std::wstring(argv[1]) == L"--benchmark-state-read" ? 2 : 0) : 0;
    bool recentTest = argv && argc == 5 && std::wstring(argv[1]) == L"--benchmark-recents";
    bool benchmark = recentTest || stateTest || (argv && argc == 4 && (std::wstring(argv[1]) == L"--benchmark-open" || navigationTest || textTest || outlineTest || linksTest || passwordTest));
    bool previewHome = argv && argc == 3 && std::wstring(argv[1]) == L"--preview-home";
    bool shellBenchmark = argv && argc == 3 && (std::wstring(argv[1]) == L"--benchmark-shell" || previewHome);
    SetProcessDPIAware();
    INITCOMMONCONTROLSEX controls{sizeof(controls), ICC_TREEVIEW_CLASSES}; InitCommonControlsEx(&controls);
    WNDCLASSW wc{}; wc.hInstance = instance; wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hIcon = LoadIconW(instance, MAKEINTRESOURCEW(IDI_CHUCAO));
    wc.lpszClassName = L"ChucaoReader"; wc.lpfnWndProc = WindowProc; RegisterClassW(&wc);
    wc.lpszClassName = L"ChucaoPassword"; wc.lpfnWndProc = PasswordProc;
    wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1); RegisterClassW(&wc);
    wc.lpszClassName = L"ChucaoOutline"; wc.lpfnWndProc = OutlineProc; RegisterClassW(&wc);
    Reader app;
    app.started = started;
    app.navigationTest = navigationTest;
    app.textTest = textTest;
    app.outlineTest = outlineTest;
    app.linksTest = linksTest;
    app.passwordTest = passwordTest;
    app.readingStateEnabled = !benchmark && !shellBenchmark;
    if (recentTest) { app.recentTest = true; app.readingStateFile = argv[4]; app.readingStateEnabled = true; }
    if (stateTest) {
        app.readingStateTest = stateTest; app.readingStateFile = argv[4];
        app.readingStateEnabled = true; app.readingStateWrites = stateTest == 1;
    }
    if (passwordTest) { std::wstring file = argv[2]; app.passwordTestRoot = file.substr(0, file.find_last_of(L"\\/") + 1); }
    if (benchmark) app.benchmarkReport = argv[3];
    if (shellBenchmark) app.benchmarkReport = argv[2];
    HWND window = CreateWindowW(L"ChucaoReader", L"Chucao PDF Reader", WS_OVERLAPPEDWINDOW | WS_VSCROLL | WS_HSCROLL | WS_CLIPCHILDREN,
        CW_USEDEFAULT, CW_USEDEFAULT, 1080, 800, nullptr, nullptr, instance, &app);
    if (!window) return 1;
    if (!benchmark && !shellBenchmark) { ShowWindow(window, show); UpdateWindow(window); }
    app.shellMs = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - started).count();
    if (shellBenchmark) { if (previewHome) app.SaveTestImage(app.benchmarkReport + L".home.bmp"); app.BenchmarkResult(true); }
    else if (benchmark) { app.Load(argv[2]); SetTimer(window, 2, 30000, nullptr); }
    else if (argv && argc > 1) app.Load(argv[1]);
    if (argv) LocalFree(argv);
    MSG msg{};
    while (GetMessageW(&msg, nullptr, 0, 0) > 0) {
        if (app.outlineWindow && (msg.hwnd == app.outlineWindow || IsChild(app.outlineWindow, msg.hwnd))) {
            if (msg.message == WM_KEYDOWN && (msg.wParam == VK_ESCAPE || (msg.wParam == 'B' && GetKeyState(VK_CONTROL) < 0))) {
                SendMessageW(app.outlineWindow, WM_CLOSE, 0, 0); continue;
            }
            TranslateMessage(&msg); DispatchMessageW(&msg); continue;
        }
        if (msg.message == WM_KEYDOWN) {
            bool ctrl = GetKeyState(VK_CONTROL) < 0, edit = GetFocus() == app.query;
            if (!edit && GetKeyState(VK_MENU) < 0 && msg.wParam == VK_LEFT) { app.BackFromLink(); continue; }
            if (!edit && (msg.wParam == VK_NEXT || msg.wParam == VK_PRIOR || msg.wParam == VK_DOWN || msg.wParam == VK_UP || msg.wParam == VK_HOME || msg.wParam == VK_END)) {
                app.pendingReadingState.reset();
                ++app.linkEpoch;
                if (msg.wParam == VK_HOME) app.ScrollTo(0);
                else if (msg.wParam == VK_END) app.ScrollTo(app.MaxScroll());
                else app.ScrollTo(app.scroll + (msg.wParam == VK_UP ? -40 : msg.wParam == VK_DOWN ? 40 : msg.wParam == VK_PRIOR ? -app.ViewHeight() + 40 : app.ViewHeight() - 40));
                continue;
            }
            int command = 0;
            if (!edit && msg.wParam == VK_ESCAPE) { app.EndSelection(); app.ClearSelection(); continue; }
            if (ctrl && msg.wParam == 'O') command = GetKeyState(VK_SHIFT) < 0 ? OpenMenu : Open;
            else if (ctrl && msg.wParam == 'B') command = Outline;
            else if (ctrl && msg.wParam == 'F') command = Find;
            else if (ctrl && msg.wParam == 'P') command = Print;
            else if (ctrl && !edit && msg.wParam == 'C') command = Copy;
            else if (ctrl && msg.wParam == '0') command = Fit;
            else if (msg.wParam == VK_F3 || (edit && msg.wParam == VK_RETURN)) command = GetKeyState(VK_SHIFT) < 0 ? FindPrevious : FindNext;
            else if (!edit && msg.wParam == VK_RIGHT) command = Next;
            else if (!edit && msg.wParam == VK_LEFT) command = Previous;
            if (command) { app.Command(command); continue; }
        }
        TranslateMessage(&msg); DispatchMessageW(&msg);
    }
    return static_cast<int>(msg.wParam);
}
