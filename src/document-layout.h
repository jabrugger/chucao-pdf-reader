#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

// Geometry only. Unknown pages use an estimate until their first render; opening
// a document never walks its full PDF page tree just to obtain dimensions.
class DocumentLayout {
    struct Size { double width = 0, height = 0; };
    std::vector<Size> sizes_;
    std::vector<std::int64_t> tops_;
    std::vector<int> widths_, heights_;
    double estimateWidth_ = 612, estimateHeight_ = 792;
    bool estimated_ = false;
    int maxWidth_ = 0;
    int gap_ = Gap;
public:
    static constexpr int Gap = 24;
    void Reset(int pages) {
        sizes_.assign(pages, {}); tops_.clear(); widths_.clear(); heights_.clear();
        estimateWidth_ = 612; estimateHeight_ = 792; estimated_ = false;
        maxWidth_ = 0;
    }
    void Learn(int page, double width, double height) {
        if (page < 0 || page >= static_cast<int>(sizes_.size()) || width <= 0 || height <= 0) return;
        sizes_[page] = {width, height};
        if (!estimated_) { estimateWidth_ = width; estimateHeight_ = height; estimated_ = true; }
    }
    void Rebuild(int viewportWidth, double zoom, int gap = Gap) {
        gap_ = gap > 0 ? gap : Gap;
        tops_.resize(sizes_.size() + 1); widths_.resize(sizes_.size()); heights_.resize(sizes_.size());
        tops_[0] = gap_;
        maxWidth_ = 0;
        for (size_t i = 0; i < sizes_.size(); ++i) {
            double pw = sizes_[i].width > 0 ? sizes_[i].width : estimateWidth_;
            double ph = sizes_[i].height > 0 ? sizes_[i].height : estimateHeight_;
            double scale = zoom > 0 ? zoom : std::max(100, viewportWidth - 2 * gap_ - 16) / pw;
            double w = std::max(1.0, pw * scale), h = std::max(1.0, ph * scale);
            double reduction = std::min(1.0, std::min(16000.0 / std::max(w, h), std::sqrt(12000000.0 / (w * h))));
            widths_[i] = std::max(1, static_cast<int>(w * reduction));
            heights_[i] = std::max(1, static_cast<int>(h * reduction));
            maxWidth_ = std::max(maxWidth_, widths_[i]);
            tops_[i + 1] = tops_[i] + heights_[i] + gap_;
        }
    }
    int Count() const { return static_cast<int>(sizes_.size()); }
    int PageAt(std::int64_t y) const {
        if (sizes_.empty() || tops_.empty()) return 0;
        auto i = std::upper_bound(tops_.begin(), tops_.end(), y) - tops_.begin() - 1;
        return std::max(0, std::min(Count() - 1, static_cast<int>(i)));
    }
    std::int64_t Top(int page) const { return tops_[page]; }
    int Width(int page) const { return widths_[page]; }
    int Height(int page) const { return heights_[page]; }
    int HorizontalExtent(int viewport) const { return std::max(viewport, maxWidth_ + 2 * gap_); }
    int MaxHorizontal(int viewport) const { return HorizontalExtent(viewport) - viewport; }
    int Left(int page, int viewport, int offset = 0) const { return (HorizontalExtent(viewport) - widths_[page]) / 2 - offset; }
    std::int64_t Total() const { return tops_.empty() ? 0 : tops_.back(); }
};
