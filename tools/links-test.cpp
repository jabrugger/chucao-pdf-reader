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
        for (const auto& url : {"https://example.invalid/a%20b?q=1&x=2#part", "HTTP://example.invalid:8080/", "https://[::1]/", "https://example.invalid/café"})
            Check(!DecodeWebUrl(url).empty(), "web URL accepted");
        for (const auto& url : {"", "javascript:alert(1)", "file:///C:/x.pdf", "mailto:a@example.invalid", "https:///x", "https://user:pass@example.invalid/", "https://example.invalid/ bad", "https://example.invalid/\r\nx", "https://example.invalid/\"x", "https://example.invalid\\x", "//example.invalid", "https://:123/"})
            Check(DecodeWebUrl(url).empty(), "unsupported or ambiguous URL rejected");
        Check(DecodeWebUrl(std::string("https://example.invalid/\0x", 26)).empty(), "embedded NUL rejected");
        Check(DecodeWebUrl("https://example.invalid/\xff").empty(), "invalid UTF-8 rejected");
        Check(DecodeWebUrl("https://example.invalid/" + std::string(8192, 'x')).empty(), "URL length bounded");
        for (const wchar_t* name : {L"links.pdf", L"rotated-links.pdf"}) {
            pdf = FPDF_LoadDocument(Utf8(std::wstring(argv[1]) + L"\\" + name).c_str(), nullptr);
            Check(pdf != nullptr, "fixture open"); page = FPDF_LoadPage(pdf, 0); Check(page != nullptr, "source page");
            auto links = ReadPageLinks(pdf, page, [] { return false; });
            Check(links.size() == 4, "only internal and HTTP/HTTPS destinations exposed");
            Check(ReadPageLinks(pdf, page, [] { return true; }).empty(), "cancel link traversal");
            for (const auto& link : links) {
                Check(link.box.Valid(), "link geometry");
                auto target = ResolvePageLink(pdf, 0, (link.box.left + link.box.right) / 2, (link.box.top + link.box.bottom) / 2);
                if (link.target < 0) {
                    Check(target.hit && target.page == -1 && target.webUrl == L"https://example.invalid", "URI resolved with precedence over conflicting destination");
                    continue;
                }
                Check(target.hit && target.page == 1 && target.webUrl.empty(), "direct and named GoTo target");
                auto targetPage = FPDF_LoadPage(pdf, 1); double x = 0, y = 0;
                bool mapped = FPDF_DeviceToPage(targetPage, 0, 0, 100000, 100000, 0, static_cast<int>(std::round(target.x * 100000)), static_cast<int>(std::round(target.y * 100000)), &x, &y) != 0;
                FPDF_ClosePage(targetPage);
                Check(mapped && std::abs(x - 150) < .1 && std::abs(y - 500) < .1, "XYZ target position after rotation and crop");
            }
            for (int y : {610, 570, 530, 450, 410, 370, 330, 290, 250}) {
                auto box = PageBox(page, 90, 110, y - 1, y + 1);
                auto target = ResolvePageLink(pdf, 0, (box.left + box.right) / 2, (box.top + box.bottom) / 2);
                Check(target.hit && target.page == -1 && target.webUrl.empty(), "unsupported action or malformed URI stays inactive");
            }
            Check(!ResolvePageLink(pdf, 0, 1.5, .5).hit, "invalid normalized position");
            FPDF_ClosePage(page); page = nullptr; FPDF_CloseDocument(pdf); pdf = nullptr;
        }
        FPDF_DestroyLibrary(); std::puts("links: internal/HTTP/HTTPS, URI validation, XYZ, rotation/crop, cancellation and inactive actions OK"); return 0;
    } catch (const std::exception& error) {
        if (page) FPDF_ClosePage(page); if (pdf) FPDF_CloseDocument(pdf); FPDF_DestroyLibrary();
        std::fprintf(stderr, "%s\n", error.what()); return 1;
    }
}
