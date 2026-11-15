#pragma once
#include <fpdf_doc.h>
#include <functional>
#include "pdf-coordinates.h"
#include "web-links.h"
#include "pdf-destinations.h"

struct PageLink { TextBox box; int target = -1; };
struct LinkTarget : DestinationLocation {
    bool hit = false;
    std::wstring webUrl;
};
inline std::wstring WebLinkUrl(FPDF_DOCUMENT pdf, FPDF_LINK link) {
    auto action = FPDFLink_GetAction(link);
    if (!action || FPDFAction_GetType(action) != PDFACTION_URI) return {};
    auto length = FPDFAction_GetURIPath(pdf, action, nullptr, 0);
    if (length < 2 || length > 8193) return {};
    std::string bytes(length, 0);
    if (FPDFAction_GetURIPath(pdf, action, &bytes[0], length) != length || bytes.back() != '\0') return {};
    bytes.pop_back(); return DecodeWebUrl(bytes);
}
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
        if (((target >= 0 && target < count) || !WebLinkUrl(pdf, link).empty()) && FPDFLink_GetAnnotRect(link, &rect)) {
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
    if (link) result.webUrl = WebLinkUrl(pdf, link);
    auto destination = link ? InternalLinkDestination(pdf, link) : nullptr;
    auto target = ReadDestination(pdf, destination);
    FPDF_ClosePage(page);
    static_cast<DestinationLocation&>(result) = ResolveDestination(pdf, target);
    return result;
}
