#pragma once
#include <fpdf_doc.h>
#include <functional>
#include "pdf-coordinates.h"

struct PageLink { TextBox box; int target = -1; };
struct LinkTarget {
    bool hit = false;
    int page = -1;
    double width = 0, height = 0, x = 0, y = 0;
};
inline FPDF_DEST InternalLinkDestination(FPDF_DOCUMENT pdf, FPDF_LINK link) {
    auto action = FPDFLink_GetAction(link);
    // Resolve only same-document destinations, never execute actions.
    return action ? (FPDFAction_GetType(action) == PDFACTION_GOTO ? FPDFAction_GetDest(pdf, action) : nullptr) : FPDFLink_GetDest(pdf, link);
}
inline std::vector<PageLink> ReadPageLinks(FPDF_DOCUMENT pdf, FPDF_PAGE page, const std::function<bool()>& cancelled) {
    std::vector<PageLink> result;
    int position = 0, count = FPDF_GetPageCount(pdf); FPDF_LINK link = nullptr;
    for (int i = 0; i < 1024 && !cancelled() && FPDFLink_Enumerate(page, &position, &link); ++i) {
        auto destination = InternalLinkDestination(pdf, link);
        int target = destination ? FPDFDest_GetDestPageIndex(pdf, destination) : -1;
        FS_RECTF rect{};
        if (target >= 0 && target < count && FPDFLink_GetAnnotRect(link, &rect)) {
            auto box = PageBox(page, rect.left, rect.right, rect.bottom, rect.top);
            if (box.Valid()) result.push_back({box, target});
        }
    }
    return result;
}
inline LinkTarget ResolvePageLink(FPDF_DOCUMENT pdf, int source, double x, double y) {
    LinkTarget result;
    if (source < 0 || source >= FPDF_GetPageCount(pdf) || !std::isfinite(x) || !std::isfinite(y) || x < 0 || x > 1 || y < 0 || y > 1) return result;
    auto page = FPDF_LoadPage(pdf, source); if (!page) return result;
    double px = 0, py = 0;
    FPDF_LINK link = FPDF_DeviceToPage(page, 0, 0, 100000, 100000, 0, static_cast<int>(x * 100000), static_cast<int>(y * 100000), &px, &py) ? FPDFLink_GetLinkAtPoint(page, px, py) : nullptr;
    result.hit = link != nullptr;
    auto destination = link ? InternalLinkDestination(pdf, link) : nullptr;
    int target = destination ? FPDFDest_GetDestPageIndex(pdf, destination) : -1;
    FPDF_BOOL hasX = false, hasY = false, hasZoom = false; FS_FLOAT dx = 0, dy = 0, zoom = 0;
    if (destination) FPDFDest_GetLocationInPage(destination, &hasX, &hasY, &hasZoom, &dx, &dy, &zoom);
    FPDF_ClosePage(page);
    if (target < 0 || target >= FPDF_GetPageCount(pdf)) return result;
    auto targetPage = FPDF_LoadPage(pdf, target); if (!targetPage) return result;
    result.width = FPDF_GetPageWidth(targetPage); result.height = FPDF_GetPageHeight(targetPage);
    if (result.width > 0 && result.height > 0 && std::isfinite(result.width) && std::isfinite(result.height)) {
        result.page = target;
        double defaultX = 0, defaultY = 0; int outX = 0, outY = 0;
        if ((hasX || hasY) && FPDF_DeviceToPage(targetPage, 0, 0, 100000, 100000, 0, 0, 0, &defaultX, &defaultY) &&
            FPDF_PageToDevice(targetPage, 0, 0, 100000, 100000, 0, hasX && std::isfinite(dx) ? dx : defaultX, hasY && std::isfinite(dy) ? dy : defaultY, &outX, &outY)) {
            result.x = std::max(0.0, std::min(1.0, outX / 100000.0));
            result.y = std::max(0.0, std::min(1.0, outY / 100000.0));
        }
    }
    FPDF_ClosePage(targetPage); return result;
}
