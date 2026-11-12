#include <fpdfview.h>
#include <fpdf_text.h>
#include <chrono>
#include <cstdio>
#include <vector>
#include <string>
#include "pdf-source.h"

int wmain(int argc, wchar_t** argv) {
    if (argc < 2) { std::fprintf(stderr, "Usage: pdf-smoke file.pdf [password] [--expect-password]\n"); return 2; }
    auto start = std::chrono::steady_clock::now();
    FPDF_LIBRARY_CONFIG config{}; config.version = 2; FPDF_InitLibraryWithConfig(&config);
    auto initialized = std::chrono::steady_clock::now();
    bool benchmark = argc > 2 && std::wstring(argv[2]) == L"--benchmark";
    bool buffered = argc > 3 && (std::wstring(argv[3]) == L"--buffered" || std::wstring(argv[3]) == L"--buffered-64k");
    auto utf8 = [](const wchar_t* value) { int n = WideCharToMultiByte(CP_UTF8, 0, value, -1, nullptr, 0, nullptr, nullptr); std::string result(n, 0); WideCharToMultiByte(CP_UTF8, 0, value, -1, &result[0], n, nullptr, nullptr); return result; };
    PdfSource source(argc > 3 && std::wstring(argv[3]) == L"--buffered-64k" ? 65536 : 4096);
    FPDF_DOCUMENT doc = buffered ? (source.Open(argv[1]) ? FPDF_LoadCustomDocument(&source.access, nullptr) : nullptr) : FPDF_LoadDocument(utf8(argv[1]).c_str(), argc > 2 && !benchmark ? utf8(argv[2]).c_str() : nullptr);
    bool expectPassword = argc > 3 && std::wstring(argv[3]) == L"--expect-password";
    if (!doc) {
        auto error = FPDF_GetLastError(); FPDF_DestroyLibrary();
        std::printf("open_error=%lu\n", error);
        return expectPassword && error == FPDF_ERR_PASSWORD ? 0 : 1;
    }
    if (expectPassword) { FPDF_CloseDocument(doc); FPDF_DestroyLibrary(); return 1; }
    auto opened = std::chrono::steady_clock::now();
    int pageCount = FPDF_GetPageCount(doc);
    auto counted = std::chrono::steady_clock::now();
    FPDF_PAGE page = FPDF_LoadPage(doc, 0);
    auto loaded = std::chrono::steady_clock::now();
    if (!page) { FPDF_CloseDocument(doc); FPDF_DestroyLibrary(); return 1; }
    FPDF_BITMAP bitmap = FPDFBitmap_Create(800, 1000, 0);
    if (!bitmap) { FPDF_ClosePage(page); FPDF_CloseDocument(doc); FPDF_DestroyLibrary(); return 1; }
    FPDFBitmap_FillRect(bitmap, 0, 0, 800, 1000, 0xffffffff);
    FPDF_RenderPageBitmap(bitmap, page, 0, 0, 800, 1000, 0, 0);
    auto rendered = std::chrono::steady_clock::now();
    auto pixels = static_cast<unsigned char*>(FPDFBitmap_GetBuffer(bitmap));
    int stride = FPDFBitmap_GetStride(bitmap), dark = 0;
    for (int y = 0; y < 1000; ++y) for (int x = 0; x < 800; ++x) if (pixels[y * stride + x * 4] < 100) ++dark;
    FPDF_TEXTPAGE text = FPDFText_LoadPage(page);
    const unsigned short term[] = {'C','h','u','c','a','o',0};
    auto search = text ? FPDFText_FindStart(text, term, 0, 0) : nullptr;
    bool found = search && FPDFText_FindNext(search);
    auto ms = [](auto a, auto b) { return std::chrono::duration<double, std::milli>(b-a).count(); };
    std::printf("io=%s pages=%d init_ms=%.3f open_ms=%.3f count_ms=%.3f page_ms=%.3f render_ms=%.3f first_page_ms=%.3f dark_pixels=%d search=%s requests=%llu reads=%llu bytes=%llu\n", buffered ? "buffered" : "native", pageCount,
        ms(start, initialized), ms(initialized, opened), ms(opened, counted), ms(counted, loaded), ms(loaded, rendered), ms(start, rendered), dark, found ? "ok" : "missing", source.requests, source.reads, source.bytesRead);
    if (search) FPDFText_FindClose(search);
    if (text) FPDFText_ClosePage(text);
    FPDFBitmap_Destroy(bitmap); FPDF_ClosePage(page); FPDF_CloseDocument(doc); FPDF_DestroyLibrary();
    return dark > 10 && (benchmark || found) ? 0 : 1;
}
