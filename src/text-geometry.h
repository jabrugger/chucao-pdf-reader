#pragma once
#include <algorithm>
#include <cmath>
#include <limits>
#include <vector>

struct TextBox {
    double left = 0, top = 0, right = 0, bottom = 0;
    bool Valid() const { return std::isfinite(left) && std::isfinite(top) && std::isfinite(right) && std::isfinite(bottom) && right > left && bottom > top; }
};
inline int NearestCharacter(const std::vector<TextBox>& boxes, double x, double y, double aspect) {
    int best = -1; double distance = (std::numeric_limits<double>::max)();
    for (size_t i = 0; i < boxes.size(); ++i) {
        const auto& box = boxes[i]; if (!box.Valid()) continue;
        double dx = std::max(box.left - x, std::max(0.0, x - box.right));
        double dy = std::max(box.top - y, std::max(0.0, y - box.bottom)) * aspect;
        double score = dx * dx + dy * dy;
        if (score < distance) { distance = score; best = static_cast<int>(i); }
    }
    return best;
}
inline std::vector<TextBox> SelectionBoxes(const std::vector<TextBox>& boxes, int first, int last) {
    std::vector<TextBox> result;
    first = std::max(0, first); last = std::min(last, static_cast<int>(boxes.size()) - 1);
    for (int i = first; i <= last; ++i) {
        const auto& box = boxes[i]; if (!box.Valid()) continue;
        if (!result.empty()) {
            auto& previous = result.back();
            if (std::abs(previous.top - box.top) < 0.002 && std::abs(previous.bottom - box.bottom) < 0.002 && box.left >= previous.left && box.left - previous.right < 0.015) {
                previous.right = std::max(previous.right, box.right); continue;
            }
        }
        result.push_back(box);
    }
    return result;
}
