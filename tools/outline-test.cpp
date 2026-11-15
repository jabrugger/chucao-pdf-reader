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
        Check(outline.entries[1].destination.hasX && outline.entries[1].destination.hasY &&
            outline.entries[1].destination.x == 150 && outline.entries[1].destination.y == 200, "XYZ metadata without page loading");
        for (int i = 2; i < 5; ++i) Check(outline.entries[i].page == -1, "external, invalid and script entries inactive");
        int checks = 0;
        auto cancelled = ReadOutline(pdf, [&] { return ++checks > 2; });
        Check(cancelled.entries.size() == 2, "cancel traversal");
        for (const auto* name : {L"outline.pdf", L"rotated-outline.pdf"}) {
            outline = read(name);
            auto target = ResolveDestination(pdf, outline.entries[1].destination);
            Check(target.page == 1 && target.y > 0, "resolved outline position");
            auto page = FPDF_LoadPage(pdf, 1); double x = 0, y = 0;
            bool mapped = FPDF_DeviceToPage(page, 0, 0, 100000, 100000, 0,
                static_cast<int>(std::round(target.x * 100000)), static_cast<int>(std::round(target.y * 100000)), &x, &y) != 0;
            FPDF_ClosePage(page);
            Check(mapped && std::abs(x - 150) < .1 && std::abs(y - 200) < .1, "XYZ after rotation/crop");
            Check(ResolveDestination(pdf, {}).page == -1, "invalid destination rejected");
        }
        outline = read(L"outline-null-200.pdf");
        Check(!outline.entries[1].destination.hasX && outline.entries[1].destination.hasY, "nullable X metadata");
        auto target = ResolveDestination(pdf, outline.entries[1].destination);
        Check(target.page == 1 && target.x == 0 && target.y > 0, "nullable X falls back to page edge");
        outline = read(L"outline-150-null.pdf");
        target = ResolveDestination(pdf, outline.entries[1].destination);
        Check(target.page == 1 && target.x > 0 && target.y == 0, "nullable Y falls back to page edge");
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
        std::puts("outline: Unicode, hierarchy, XYZ/nullable coordinates, rotation/crop, inactive actions and traversal bounds OK");
        return 0;
    } catch (const std::exception& error) {
        if (pdf) FPDF_CloseDocument(pdf);
        FPDF_DestroyLibrary(); std::fprintf(stderr, "%s\n", error.what()); return 1;
    }
}
