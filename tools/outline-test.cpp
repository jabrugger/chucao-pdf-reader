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
        for (const auto* name : {L"destination-Fit.pdf", L"destination-FitH-200.pdf", L"destination-FitV-150.pdf",
            L"destination-XYZ-150-200-2.pdf", L"destination-XYZ-150-200-0.pdf",
            L"destination-FitR-50-100-300-400.pdf", L"destination-FitB.pdf", L"destination-FitBH-200.pdf", L"destination-FitBV-150.pdf"}) {
            outline = read(name);
            auto target = ResolveDestination(pdf, outline.entries[1].destination);
            double scale = DestinationZoom(target, 1048, 848, 3);
            if (target.view == PDFDEST_VIEW_FIT) Check(std::abs(scale - 1000.0 / 792) < 1e-6, "fit whole page");
            else if (target.view == PDFDEST_VIEW_FITH) Check(std::abs(scale - 1000.0 / 792) < 1e-6 && target.y == .5, "fit width at top coordinate");
            else if (target.view == PDFDEST_VIEW_FITV) Check(scale == 2 && target.x > 0, "fit height at left coordinate");
            else if (target.view == PDFDEST_VIEW_XYZ) Check(scale == (target.zoom > 0 ? 2 : 3), "explicit XYZ zoom and zero preserves current zoom");
            else {
                Check(target.hasBox, "box destination resolved");
                double left = 0, top = 0, right = 1, bottom = 1;
                if (target.view == PDFDEST_VIEW_FITR) {
                    left = 50.0 / target.width; right = 300.0 / target.width;
                    top = (target.height - 400.0) / target.height; bottom = (target.height - 100.0) / target.height;
                    Check(std::abs(target.boxLeft - left) < .002 && std::abs(target.boxRight - right) < .002 &&
                        std::abs(target.boxTop - top) < .002 && std::abs(target.boxBottom - bottom) < .002, "FitR rectangle");
                } else {
                    auto page = FPDF_LoadPage(pdf, 1); FS_RECTF box{0, static_cast<float>(target.height), static_cast<float>(target.width), 0};
                    if (page) { if (!FPDF_GetPageBoundingBox(page, &box)) box = FS_RECTF{0, static_cast<float>(target.height), static_cast<float>(target.width), 0}; FPDF_ClosePage(page); }
                    left = std::max(0.0, box.left / target.width); right = std::min(1.0, box.right / target.width);
                    top = std::max(0.0, (target.height - box.top) / target.height); bottom = std::min(1.0, (target.height - box.bottom) / target.height);
                    Check(std::abs(target.boxLeft - left) < .002 && std::abs(target.boxRight - right) < .002 &&
                        std::abs(target.boxTop - top) < .002 && std::abs(target.boxBottom - bottom) < .002, "content box matches the page bounding box");
                }
                double boxWidth = (target.boxRight - target.boxLeft) * target.width;
                double boxHeight = (target.boxBottom - target.boxTop) * target.height;
                double expected = target.view == PDFDEST_VIEW_FITBH ? 1000.0 / boxWidth :
                    target.view == PDFDEST_VIEW_FITBV ? 800.0 / boxHeight :
                    std::min(1000.0 / boxWidth, 800.0 / boxHeight);
                expected = std::max(.25, std::min(4.0, expected));
                Check(std::abs(scale - expected) < 1e-6, "box fit zoom");
                if (target.view == PDFDEST_VIEW_FITR) Check(std::abs(boxWidth - 250) < .5 && std::abs(boxHeight - 300) < .5, "FitR rectangle dimensions");
                if (target.view == PDFDEST_VIEW_FITBH) Check(target.y == .5 && target.x == .5, "FitBH top coordinate and centered width");
                if (target.view == PDFDEST_VIEW_FITBV) Check(target.x > 0 && target.y >= 0, "FitBV left coordinate");
            }
        }
        DestinationLocation bounded; bounded.page = 0; bounded.width = 100; bounded.height = 200;
        bounded.view = PDFDEST_VIEW_XYZ; bounded.zoom = 100;
        Check(DestinationZoom(bounded, 1000, 800, 0) == 4, "zoom upper bound");
        bounded.zoom = .01; Check(DestinationZoom(bounded, 1000, 800, 3) == .25, "zoom lower bound");
        bounded.view = PDFDEST_VIEW_FIT;
        Check(DestinationZoom(bounded, 10, 10, 3) == .25, "tiny viewport stays bounded");
        bounded.view = PDFDEST_VIEW_FITR;
        Check(std::abs(DestinationZoom(bounded, 1000, 800, 0) - 3.76) < 1e-9, "FitR without a rectangle falls back to the page size");
        bounded.hasBox = true;
        Check(std::abs(DestinationZoom(bounded, 1000, 800, 0) - 3.76) < 1e-9, "FitR whole-page box");
        bounded.boxLeft = bounded.boxRight = .5;
        Check(DestinationZoom(bounded, 1000, 800, 3) == 3, "degenerate box preserves the current zoom");
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
