#pragma once
#include <algorithm>
#include <cmath>

// Bitmap size for a PDF page under the reader's fixed budget. The width and
// height caps bound both the allocation and the per-render pixel count. Returns
// zeroed fields for invalid input so callers can report a page error.
struct RenderTarget {
    int width = 0, height = 0;
    double scale = 0;
};

inline RenderTarget LimitRenderSize(double pageWidth, double pageHeight, double scale) {
    if (!(pageWidth > 0) || !(pageHeight > 0) || !(scale > 0) ||
        !std::isfinite(pageWidth) || !std::isfinite(pageHeight) || !std::isfinite(scale)) return {};
    double w = std::max(1.0, pageWidth * scale);
    double h = std::max(1.0, pageHeight * scale);
    if (!std::isfinite(w) || !std::isfinite(h) || !std::isfinite(w * h)) return {};
    double reduction = std::min(1.0, std::min(16000.0 / std::max(w, h), std::sqrt(12000000.0 / (w * h))));
    RenderTarget target;
    target.width = std::max(1, static_cast<int>(w * reduction));
    target.height = std::max(1, static_cast<int>(h * reduction));
    target.scale = target.width / pageWidth;
    return target;
}
