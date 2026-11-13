#include <windows.h>
#include "../src/document-links.h"
#include <cstdio>
#include <string>
#include <stdexcept>

void Check(bool ok, const char* message) { if (!ok) throw std::runtime_error(message); }
std::string Utf8(const std::wstring& path) {
    int n = WideCharToMultiByte(CP_UTF8, 0, path.c_str(), -1, nullptr, 0, nullptr, nullptr);
    std::string result(n, 0); WideCharToMultiByte(CP_UTF8, 0, path.c_str(), -1, &result[0], n, nullptr, nullptr); return result;
}
int wmain(int argc, wchar_t** argv) {
    if (argc != 2) return 2;
    FPDF_InitLibrary(); FPDF_DOCUMENT pdf = nullptr; FPDF_PAGE page = nullptr;
    try {
        for (const wchar_t* name : {L"links.pdf", L"rotated-links.pdf"}) {
            pdf = FPDF_LoadDocument(Utf8(std::wstring(argv[1]) + L"\\" + name).c_str(), nullptr);
            Check(pdf != nullptr, "fixture open"); page = FPDF_LoadPage(pdf, 0); Check(page != nullptr, "source page");
            auto links = ReadPageLinks(pdf, page, [] { return false; });
            Check(links.size() == 2, "only internal destinations exposed");
            Check(ReadPageLinks(pdf, page, [] { return true; }).empty(), "cancel link traversal");
            for (const auto& link : links) {
                Check(link.box.Valid(), "link geometry");
                auto target = ResolvePageLink(pdf, 0, (link.box.left + link.box.right) / 2, (link.box.top + link.box.bottom) / 2);
                Check(target.hit && target.page == 1, "direct and named GoTo target");
                auto targetPage = FPDF_LoadPage(pdf, 1); double x = 0, y = 0;
                bool mapped = FPDF_DeviceToPage(targetPage, 0, 0, 100000, 100000, 0, static_cast<int>(std::round(target.x * 100000)), static_cast<int>(std::round(target.y * 100000)), &x, &y) != 0;
                FPDF_ClosePage(targetPage);
                Check(mapped && std::abs(x - 150) < .1 && std::abs(y - 500) < .1, "XYZ target position after rotation and crop");
            }
            for (int y : {650, 610, 570, 530, 490}) {
                auto box = PageBox(page, 90, 110, y - 1, y + 1);
                auto target = ResolvePageLink(pdf, 0, (box.left + box.right) / 2, (box.top + box.bottom) / 2);
                Check(target.hit && target.page == -1, "external/script/launch/invalid/mixed action stays inactive");
            }
            Check(!ResolvePageLink(pdf, 0, 1.5, .5).hit, "invalid normalized position");
            FPDF_ClosePage(page); page = nullptr; FPDF_CloseDocument(pdf); pdf = nullptr;
        }
        FPDF_DestroyLibrary(); std::puts("links: hit testing, destinations, XYZ, rotation/crop, cancellation and inactive actions OK"); return 0;
    } catch (const std::exception& error) {
        if (page) FPDF_ClosePage(page); if (pdf) FPDF_CloseDocument(pdf); FPDF_DestroyLibrary();
        std::fprintf(stderr, "%s\n", error.what()); return 1;
    }
}
