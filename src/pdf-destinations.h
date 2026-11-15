#pragma once
#include <fpdf_doc.h>
#include <algorithm>
#include <cmath>

// Plain metadata: no PDFium handles cross the worker boundary.
struct PdfDestination {
    int page = -1;
    bool hasX = false, hasY = false;
    double x = 0, y = 0;
};
struct DestinationLocation {
    int page = -1;
    double width = 0, height = 0, x = 0, y = 0;
};
inline PdfDestination ReadDestination(FPDF_DOCUMENT pdf, FPDF_DEST destination) {
    PdfDestination result;
    if (!destination) return result;
    int page = FPDFDest_GetDestPageIndex(pdf, destination);
    if (page < 0 || page >= FPDF_GetPageCount(pdf)) return result;
    result.page = page;
    FPDF_BOOL hasX = false, hasY = false, hasZoom = false;
    FS_FLOAT x = 0, y = 0, zoom = 0;
    if (FPDFDest_GetLocationInPage(destination, &hasX, &hasY, &hasZoom, &x, &y, &zoom)) {
        result.hasX = hasX && std::isfinite(x); result.hasY = hasY && std::isfinite(y);
        result.x = x; result.y = y;
    }
    return result;
}
// Only the chosen target page is loaded. Conversion includes CropBox and rotation.
inline DestinationLocation ResolveDestination(FPDF_DOCUMENT pdf, const PdfDestination& destination) {
    DestinationLocation result;
    if (destination.page < 0 || destination.page >= FPDF_GetPageCount(pdf)) return result;
    auto page = FPDF_LoadPage(pdf, destination.page); if (!page) return result;
    result.width = FPDF_GetPageWidth(page); result.height = FPDF_GetPageHeight(page);
    if (result.width > 0 && result.height > 0 && std::isfinite(result.width) && std::isfinite(result.height)) {
        result.page = destination.page;
        double defaultX = 0, defaultY = 0; int x = 0, y = 0;
        if ((destination.hasX || destination.hasY) &&
            FPDF_DeviceToPage(page, 0, 0, 100000, 100000, 0, 0, 0, &defaultX, &defaultY) &&
            FPDF_PageToDevice(page, 0, 0, 100000, 100000, 0,
                destination.hasX && std::isfinite(destination.x) ? destination.x : defaultX,
                destination.hasY && std::isfinite(destination.y) ? destination.y : defaultY, &x, &y)) {
            result.x = std::max(0.0, std::min(1.0, x / 100000.0));
            result.y = std::max(0.0, std::min(1.0, y / 100000.0));
        }
    }
    FPDF_ClosePage(page); return result;
}
