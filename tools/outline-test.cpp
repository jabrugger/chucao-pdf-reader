#include <windows.h>
#include "../src/document-outline.h"
#include <cstdio>
#include <stdexcept>

void Check(bool ok, const char* message) { if (!ok) throw std::runtime_error(message); }
std::string Utf8(const std::wstring& path) {
    int size = WideCharToMultiByte(CP_UTF8, 0, path.data(), static_cast<int>(path.size()), nullptr, 0, nullptr, nullptr);
    std::string result(size, 0);
    WideCharToMultiByte(CP_UTF8, 0, path.data(), static_cast<int>(path.size()), &result[0], size, nullptr, nullptr);
    return result;
}
int wmain(int argc, wchar_t** argv) {
    if (argc != 2) return 2;
    FPDF_InitLibrary();
    FPDF_DOCUMENT pdf = nullptr;
    try {
        auto read = [&](const wchar_t* name) {
            if (pdf) FPDF_CloseDocument(pdf);
            pdf = FPDF_LoadDocument(Utf8(std::wstring(argv[1]) + L"\\" + name).c_str(), nullptr);
            Check(pdf != nullptr, "fixture open");
            return ReadOutline(pdf, [] { return false; });
        };
        auto outline = read(L"outline.pdf");
        Check(outline.entries.size() == 5 && !outline.limited, "outline count");
        Check(outline.entries[0].title == L"Capítulo café" && outline.entries[0].page == 0, "Unicode and named destination");
        Check(outline.entries[1].parent == 0 && outline.entries[1].page == 1, "hierarchy and GoTo action");
        for (int i = 2; i < 5; ++i) Check(outline.entries[i].page == -1, "external, invalid and script entries inactive");
        int checks = 0;
        auto cancelled = ReadOutline(pdf, [&] { return ++checks > 2; });
        Check(cancelled.entries.size() == 2, "cancel traversal");
        Check(read(L"sample.pdf").entries.empty(), "document without outline");
        outline = read(L"outline-cycle.pdf");
        Check(outline.limited && outline.entries.size() == 3, "circular reference stops");
        outline = read(L"outline-long-title.pdf");
        Check(outline.limited && outline.entries[0].title == L"Título demasiado largo", "title allocation bound");
        outline = read(L"outline-deep.pdf");
        Check(outline.limited && outline.entries.size() == 64, "depth bound");
        outline = read(L"outline-wide.pdf");
        Check(outline.limited && outline.entries.size() == 4096, "entry bound");
        FPDF_CloseDocument(pdf); FPDF_DestroyLibrary();
        std::puts("outline: Unicode, hierarchy, destinations, inactive actions, empty, cancellation and malformed bounds OK");
        return 0;
    } catch (const std::exception& error) {
        if (pdf) FPDF_CloseDocument(pdf);
        FPDF_DestroyLibrary(); std::fprintf(stderr, "%s\n", error.what()); return 1;
    }
}
