#include <windows.h>
#include <fpdfview.h>
#include <fpdf_text.h>
#include <cstdio>
#include <stdexcept>
#include <string>

void Check(bool ok, const char* message) { if (!ok) throw std::runtime_error(message); }
std::string Utf8(const std::wstring& value) {
    int n = WideCharToMultiByte(CP_UTF8, 0, value.c_str(), -1, nullptr, 0, nullptr, nullptr);
    std::string output(n, 0); WideCharToMultiByte(CP_UTF8, 0, value.c_str(), -1, &output[0], n, nullptr, nullptr); return output;
}
int wmain(int argc, wchar_t** argv) {
    if (argc != 2) return 2;
    FPDF_InitLibrary(); FPDF_DOCUMENT pdf = nullptr;
    try {
        auto path = [&](const wchar_t* name) { return Utf8(std::wstring(argv[1]) + L"\\" + name); };
        for (auto name : {L"aes256-ascii.pdf", L"aes128.pdf", L"rc4.pdf", L"aes256-unicode.pdf"}) {
            const char* badPasswords[] = {nullptr, "", "wrong"};
            for (auto password : badPasswords) {
                pdf = FPDF_LoadDocument(path(name).c_str(), password);
                Check(!pdf && FPDF_GetLastError() == FPDF_ERR_PASSWORD, "missing/empty/wrong password rejected");
            }
            auto password = Utf8(std::wstring(name) == L"aes256-unicode.pdf" ? L"Chucao-café-漢字" : L"Chucao-test");
            pdf = FPDF_LoadDocument(path(name).c_str(), password.c_str());
            Check(pdf && FPDF_GetPageCount(pdf) == 2, "correct password opens pages");
            auto page = FPDF_LoadPage(pdf, 0); Check(page != nullptr, "encrypted page load");
            auto text = FPDFText_LoadPage(page); Check(text != nullptr, "encrypted text load");
            const unsigned short term[] = {'C','h','u','c','a','o',0};
            auto find = FPDFText_FindStart(text, term, 0, 0); bool found = find && FPDFText_FindNext(find);
            if (find) FPDFText_FindClose(find); FPDFText_ClosePage(text);
            auto bitmap = FPDFBitmap_Create(800, 1000, 0); Check(bitmap != nullptr, "encrypted bitmap");
            FPDFBitmap_FillRect(bitmap, 0, 0, 800, 1000, 0xffffffff); FPDF_RenderPageBitmap(bitmap, page, 0, 0, 800, 1000, 0, 0);
            auto pixels = static_cast<unsigned char*>(FPDFBitmap_GetBuffer(bitmap)); int dark = 0, stride = FPDFBitmap_GetStride(bitmap);
            for (int y = 0; y < 1000; ++y) for (int x = 0; x < 800; ++x) if (pixels[y * stride + 4 * x] < 100) ++dark;
            FPDFBitmap_Destroy(bitmap); FPDF_ClosePage(page); FPDF_CloseDocument(pdf); pdf = nullptr;
            Check(found && dark > 100, "encrypted search and nonempty render");
        }
        pdf = FPDF_LoadDocument(path(L"restricted.pdf").c_str(), "reader");
        Check(pdf && !(FPDF_GetDocPermissions(pdf) & 4) && !(FPDF_GetDocPermissions(pdf) & 16), "reader printing/copy restrictions");
        FPDF_CloseDocument(pdf); pdf = FPDF_LoadDocument(path(L"restricted.pdf").c_str(), "owner");
        Check(pdf && (FPDF_GetDocPermissions(pdf) & 4) && (FPDF_GetDocPermissions(pdf) & 16), "owner permissions");
        FPDF_CloseDocument(pdf); pdf = nullptr; FPDF_DestroyLibrary();
        std::puts("passwords: AES-256, AES-128, RC4, Unicode, missing/empty/wrong, render, search and permissions OK"); return 0;
    } catch (const std::exception& error) {
        if (pdf) FPDF_CloseDocument(pdf); FPDF_DestroyLibrary(); std::fprintf(stderr, "%s\n", error.what()); return 1;
    }
}
