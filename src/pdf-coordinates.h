#pragma once
#include <fpdfview.h>
#include "text-geometry.h"

inline TextBox PageBox(FPDF_PAGE page, double left, double right, double bottom, double top) {
    int x1 = 0, y1 = 0, x2 = 0, y2 = 0;
    if (!FPDF_PageToDevice(page, 0, 0, 100000, 100000, 0, left, bottom, &x1, &y1) ||
        !FPDF_PageToDevice(page, 0, 0, 100000, 100000, 0, right, top, &x2, &y2)) return {};
    auto unit = [](int value) { return std::max(0.0, std::min(1.0, value / 100000.0)); };
    return {unit(std::min(x1, x2)), unit(std::min(y1, y2)), unit(std::max(x1, x2)), unit(std::max(y1, y2))};
}
