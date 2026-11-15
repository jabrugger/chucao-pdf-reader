#pragma once
#include <fpdf_doc.h>
#include <algorithm>
#include <cmath>

// Plain metadata: no PDFium handles cross the worker boundary.
struct PdfDestination {
    int page = -1;
    bool hasX = false, hasY = false;
    double x = 0, y = 0;
    unsigned long view = PDFDEST_VIEW_UNKNOWN_MODE;
    bool hasZoom = false;
    double zoom = 0;
};
struct DestinationLocation {
    int page = -1;
    double width = 0, height = 0, x = 0, y = 0;
    unsigned long view = PDFDEST_VIEW_UNKNOWN_MODE;
    double zoom = 0;
};
inline PdfDestination ReadDestination(FPDF_DOCUMENT pdf, FPDF_DEST destination) {
    PdfDestination result;
    if (!destination) return result;
    int page = FPDFDest_GetDestPageIndex(pdf, destination);
    if (page < 0 || page >= FPDF_GetPageCount(pdf)) return result;
    result.page = page;
    unsigned long count = 0; FS_FLOAT parameters[4]{};
    result.view = FPDFDest_GetView(destination, &count, parameters);
    FPDF_BOOL hasX = false, hasY = false, hasZoom = false;
    FS_FLOAT x = 0, y = 0, zoom = 0;
    if (FPDFDest_GetLocationInPage(destination, &hasX, &hasY, &hasZoom, &x, &y, &zoom)) {
        result.hasX = hasX && std::isfinite(x); result.hasY = hasY && std::isfinite(y);
        result.x = x; result.y = y;
        result.hasZoom = hasZoom && std::isfinite(zoom) && zoom > 0; result.zoom = zoom;
    }
    if (count >= 1 && std::isfinite(parameters[0])) {
        if (result.view == PDFDEST_VIEW_FITH) { result.hasY = true; result.y = parameters[0]; }
        if (result.view == PDFDEST_VIEW_FITV) { result.hasX = true; result.x = parameters[0]; }
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
        result.view = destination.view;
        result.zoom = destination.hasZoom && std::isfinite(destination.zoom) && destination.zoom > 0 ? destination.zoom : 0;
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
inline double DestinationZoom(const DestinationLocation& target, int width, int height, double current) {
    if (!std::isfinite(target.width) || !std::isfinite(target.height) || target.width <= 0 || target.height <= 0) return current;
    double availableWidth = std::max(1, width - 48), availableHeight = std::max(1, height - 48);
    double value = current;
    if (target.view == PDFDEST_VIEW_XYZ && target.zoom > 0 && std::isfinite(target.zoom)) value = target.zoom;
    else if (target.view == PDFDEST_VIEW_FIT) value = std::min(availableWidth / target.width, availableHeight / target.height);
    else if (target.view == PDFDEST_VIEW_FITH) value = availableWidth / target.width;
    else if (target.view == PDFDEST_VIEW_FITV) value = availableHeight / target.height;
    else return current;
    return std::max(.25, std::min(4.0, value));
}
