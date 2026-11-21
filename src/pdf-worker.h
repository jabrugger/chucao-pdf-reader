#pragma once
// Background PDF worker thread and its result messages.
// Internal fragment of main.cpp: include it only from there, after the Windows/PDFium headers.
namespace {
struct Result {
    enum Kind { Image, Opened, Password, Failure, Search, Printed, Geometry, Copied, OutlineReady, LinksReady, LinkFollowed, OutlineFollowed, Closed } kind = Failure;
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
                result->message = PdfOpenErrorMessage(error, suppliedPassword);
            } else {
                result->kind = Result::Opened; result->pages = FPDF_GetPageCount(pdf_);
                auto permissions = FPDF_GetDocPermissions(pdf_);
                result->copyAllowed = (permissions & 16) != 0; result->printAllowed = (permissions & 4) != 0;
                if (result->pages <= 0) { result->kind = Result::Failure; result->message = L"El PDF no tiene páginas válidas."; }
            }
            Send(std::move(result));
        });
    }
    void Close(unsigned long long document) {
        wantedDocument_ = document; ++revision_; ++searchRevision_;
        Queue([this, document] {
            if (cancelled_ || document != wantedDocument_) return;
            if (pdf_) { FPDF_CloseDocument(pdf_); pdf_ = nullptr; }
            document_ = document; searchPage_ = searchIndex_ = -1; lastQuery_.clear();
            auto result = std::make_unique<Result>(); result->kind = Result::Closed;
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
    void FollowDestination(PdfDestination destination, unsigned long long document, unsigned long long interaction) {
        Queue([this, destination, document, interaction] {
            if (!pdf_ || document != document_ || document != wantedDocument_ || cancelled_) return;
            auto result = std::make_unique<Result>(); result->kind = Result::OutlineFollowed; result->revision = interaction;
            static_cast<DestinationLocation&>(result->linkTarget) = ResolveDestination(pdf_, destination);
            result->linkTarget.hit = true;
            if (!cancelled_ && document == wantedDocument_) Send(std::move(result));
        });
    }
    unsigned long long Render(std::vector<int> requested, int viewportWidth, int viewportHeight, double zoom, ZoomMode mode,
        unsigned long long document, unsigned long long viewVersion) {
        auto revision = ++revision_;
        Queue([this, requested, viewportWidth, viewportHeight, zoom, mode, revision, document, viewVersion] {
          for (int page : requested) {
            if (cancelled_ || !pdf_ || document != document_ || revision != revision_) return;
            LARGE_INTEGER start{}, end{}, frequency{};
            QueryPerformanceFrequency(&frequency); QueryPerformanceCounter(&start);
            auto result = std::make_unique<Result>(); result->revision = revision; result->viewVersion = viewVersion; result->page = page;
            FPDF_PAGE pdfPage = FPDF_LoadPage(pdf_, page);
            if (!pdfPage) { result->message = PageFailureMessage(page, PageFailure::Read); Send(std::move(result)); continue; }
            double width = FPDF_GetPageWidth(pdfPage), height = FPDF_GetPageHeight(pdfPage);
            if (!(width > 0 && height > 0 && std::isfinite(width) && std::isfinite(height))) {
                FPDF_ClosePage(pdfPage); result->message = PageFailureMessage(page, PageFailure::Dimensions); Send(std::move(result)); continue;
            }
            double scale = zoom;
            if (mode != ZoomMode::Manual || !(scale > 0)) {
                double widthScale = std::max(100, viewportWidth - 64) / width;
                double heightScale = viewportHeight > 0 ? std::max(100, viewportHeight - 64) / height : widthScale;
                scale = mode == ZoomMode::FitHeight ? heightScale :
                    mode == ZoomMode::FitPage ? std::min(widthScale, heightScale) : widthScale;
            }
            result->pageWidth = width; result->pageHeight = height;
            // Bound allocations before conversion to integers; never render giant pages unchecked.
            auto target = LimitRenderSize(width, height, scale);
            if (target.width <= 0 || target.height <= 0) {
                FPDF_ClosePage(pdfPage); result->message = PageFailureMessage(page, PageFailure::Dimensions);
                Send(std::move(result)); continue;
            }
            result->width = target.width; result->height = target.height; result->scale = target.scale;
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
                else result->message = PageFailureMessage(page, PageFailure::Render);
            } else result->message = PageFailureMessage(page, PageFailure::Memory);
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
            DeleteDC(dc); result->message = ok ? L"Documento enviado a la impresora." :
                L"No se pudo completar la impresión. Revisá la impresora y la cola de impresión.";
            Send(std::move(result));
        });
    }
};

} // namespace
