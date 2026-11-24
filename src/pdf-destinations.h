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
    // FitR carries an explicit rectangle in PDF coordinates.
    bool hasRectangle = false;
    double left = 0, bottom = 0, right = 0, top = 0;
};
struct DestinationLocation {
    int page = -1;
    double width = 0, height = 0, x = 0, y = 0;
    unsigned long view = PDFDEST_VIEW_UNKNOWN_MODE;
    double zoom = 0;
    // Normalized rectangle used by FitR and the content-box modes (FitB/FitBH/FitBV).
    bool hasBox = false;
    double boxLeft = 0, boxTop = 0, boxRight = 1, boxBottom = 1;
    // FitH/FitBH without a top coordinate keep the reader's vertical scroll;
    // FitV/FitBV without a left coordinate keep the horizontal one.
    bool preserveY = false, preserveX = false;
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
    // FitH/FitBH carry an optional top coordinate and FitV/FitBV an optional left
    // one. FPDFDest_GetView reports the parameter only while it is present in the
    // array (count > 0): an absent coordinate leaves hasX/hasY false, so the reader
    // can keep its current scroll on that axis. A null coordinate is reported as 0
    // and cannot be told apart from an explicit zero, so both land on the edge.
    if (count >= 1 && std::isfinite(parameters[0])) {
        if (result.view == PDFDEST_VIEW_FITH || result.view == PDFDEST_VIEW_FITBH) { result.hasY = true; result.y = parameters[0]; }
        if (result.view == PDFDEST_VIEW_FITV || result.view == PDFDEST_VIEW_FITBV) { result.hasX = true; result.x = parameters[0]; }
    }
    if (result.view == PDFDEST_VIEW_FITR && count >= 4 &&
        std::isfinite(parameters[0]) && std::isfinite(parameters[1]) &&
        std::isfinite(parameters[2]) && std::isfinite(parameters[3])) {
        result.hasRectangle = true;
        result.left = std::min(parameters[0], parameters[2]);
        result.right = std::max(parameters[0], parameters[2]);
        result.bottom = std::min(parameters[1], parameters[3]);
        result.top = std::max(parameters[1], parameters[3]);
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
        auto normalize = [&](double pdfX, double pdfY, double& nx, double& ny) {
            int x = 0, y = 0;
            if (!FPDF_PageToDevice(page, 0, 0, 100000, 100000, 0, pdfX, pdfY, &x, &y)) return false;
            nx = std::max(0.0, std::min(1.0, x / 100000.0));
            ny = std::max(0.0, std::min(1.0, y / 100000.0));
            return true;
        };
        if (destination.view == PDFDEST_VIEW_FITR && destination.hasRectangle) {
            double ax = 0, ay = 0, bx = 0, by = 0;
            if (normalize(destination.left, destination.top, ax, ay) && normalize(destination.right, destination.bottom, bx, by)) {
                result.hasBox = true;
                result.boxLeft = std::min(ax, bx); result.boxRight = std::max(ax, bx);
                result.boxTop = std::min(ay, by); result.boxBottom = std::max(ay, by);
                result.x = result.boxLeft; result.y = result.boxTop;
            }
        } else if (destination.view == PDFDEST_VIEW_FITB || destination.view == PDFDEST_VIEW_FITBH || destination.view == PDFDEST_VIEW_FITBV) {
            FS_RECTF box{static_cast<float>(destination.left), static_cast<float>(destination.top),
                static_cast<float>(destination.right), static_cast<float>(destination.bottom)};
            if (!FPDF_GetPageBoundingBox(page, &box))
                box = FS_RECTF{0, static_cast<float>(result.height), static_cast<float>(result.width), 0};
            double ax = 0, ay = 0, bx = 0, by = 0;
            if (normalize(box.left, box.top, ax, ay) && normalize(box.right, box.bottom, bx, by)) {
                result.hasBox = true;
                result.boxLeft = std::min(ax, bx); result.boxRight = std::max(ax, bx);
                result.boxTop = std::min(ay, by); result.boxBottom = std::max(ay, by);
            }
            // FitBH fits the box width, FitBV its height and FitB both.
            if (destination.view == PDFDEST_VIEW_FITBH) {
                result.x = .5; result.y = result.boxTop;
            } else {
                result.x = result.boxLeft; result.y = result.boxTop;
            }
        }
        double defaultX = 0, defaultY = 0; int x = 0, y = 0;
        bool hasX = destination.hasX && std::isfinite(destination.x);
        bool hasY = destination.hasY && std::isfinite(destination.y);
        if ((hasX || hasY) &&
            FPDF_DeviceToPage(page, 0, 0, 100000, 100000, 0, 0, 0, &defaultX, &defaultY) &&
            FPDF_PageToDevice(page, 0, 0, 100000, 100000, 0,
                hasX ? destination.x : defaultX, hasY ? destination.y : defaultY, &x, &y)) {
            // Explicit FitH/FitBH/FitV/FitBV coordinates take precedence over the box edge.
            if (destination.view != PDFDEST_VIEW_FITR) {
                if (hasX) result.x = std::max(0.0, std::min(1.0, x / 100000.0));
                if (hasY) result.y = std::max(0.0, std::min(1.0, y / 100000.0));
            }
        }
        // FitH/FitBH omit their top coordinate and FitV/FitBV their left one when
        // the destination array carries no parameter for it; the reader then keeps
        // its current scroll on that axis instead of jumping to the edge.
        result.preserveY = (destination.view == PDFDEST_VIEW_FITH || destination.view == PDFDEST_VIEW_FITBH) && !destination.hasY;
        result.preserveX = (destination.view == PDFDEST_VIEW_FITV || destination.view == PDFDEST_VIEW_FITBV) && !destination.hasX;
    }
    FPDF_ClosePage(page); return result;
}
inline double DestinationZoom(const DestinationLocation& target, int width, int height, double current, int margin = 48) {
    if (!std::isfinite(target.width) || !std::isfinite(target.height) || target.width <= 0 || target.height <= 0) return current;
    double availableWidth = std::max(1, width - margin), availableHeight = std::max(1, height - margin);
    double value = current;
    double boxWidth = target.hasBox ? (target.boxRight - target.boxLeft) * target.width : target.width;
    double boxHeight = target.hasBox ? (target.boxBottom - target.boxTop) * target.height : target.height;
    if (target.view == PDFDEST_VIEW_XYZ && target.zoom > 0 && std::isfinite(target.zoom)) value = target.zoom;
    else if (target.view == PDFDEST_VIEW_FIT) value = std::min(availableWidth / target.width, availableHeight / target.height);
    else if (target.view == PDFDEST_VIEW_FITH) value = availableWidth / target.width;
    else if (target.view == PDFDEST_VIEW_FITV) value = availableHeight / target.height;
    else if (target.view == PDFDEST_VIEW_FITR || target.view == PDFDEST_VIEW_FITB || target.view == PDFDEST_VIEW_FITBH || target.view == PDFDEST_VIEW_FITBV) {
        // A degenerate rectangle cannot define a useful zoom; keep the current one.
        if (!(boxWidth > 0 && boxHeight > 0)) return current;
        if (target.view == PDFDEST_VIEW_FITBH) value = availableWidth / boxWidth;
        else if (target.view == PDFDEST_VIEW_FITBV) value = availableHeight / boxHeight;
        else value = std::min(availableWidth / boxWidth, availableHeight / boxHeight);
    }
    else return current;
    return std::max(.25, std::min(4.0, value));
}
