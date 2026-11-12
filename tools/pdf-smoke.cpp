#include <fpdfview.h>
#include <fpdf_text.h>
#include <chrono>
#include <cstdio>
#include <vector>
#include <string>

int main(int argc, char** argv) {
    if (argc < 2) { std::fprintf(stderr, "Usage: pdf-smoke file.pdf [password] [--expect-password]\n"); return 2; }
    FPDF_LIBRARY_CONFIG config{}; config.version = 2; FPDF_InitLibraryWithConfig(&config);
    auto start = std::chrono::steady_clock::now();
    FPDF_DOCUMENT doc = FPDF_LoadDocument(argv[1], argc > 2 ? argv[2] : nullptr);
    bool expectPassword = argc > 3 && std::string(argv[3]) == "--expect-password";
    if (!doc) {
        auto error = FPDF_GetLastError(); FPDF_DestroyLibrary();
        std::printf("open_error=%lu\n", error);
        return expectPassword && error == FPDF_ERR_PASSWORD ? 0 : 1;
    }
    if (expectPassword) { FPDF_CloseDocument(doc); FPDF_DestroyLibrary(); return 1; }
    auto opened = std::chrono::steady_clock::now();
    FPDF_PAGE page = FPDF_LoadPage(doc, 0);
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
    std::printf("pages=%d open_ms=%.3f first_page_ms=%.3f dark_pixels=%d search=%s\n", FPDF_GetPageCount(doc),
        std::chrono::duration<double, std::milli>(opened-start).count(),
        std::chrono::duration<double, std::milli>(rendered-start).count(), dark, found ? "ok" : "missing");
    if (search) FPDFText_FindClose(search);
    if (text) FPDFText_ClosePage(text);
    FPDFBitmap_Destroy(bitmap); FPDF_ClosePage(page); FPDF_CloseDocument(doc); FPDF_DestroyLibrary();
    return dark > 10 && found ? 0 : 1;
}
