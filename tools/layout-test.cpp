#include "../src/document-layout.h"
#include "../src/text-geometry.h"
#include <cstdio>
#include <stdexcept>

void Check(bool condition, const char* message) { if (!condition) throw std::runtime_error(message); }
int main() {
    try {
        DocumentLayout layout;
        layout.Reset(0); layout.Rebuild(1000, 0);
        Check(layout.PageAt(0) == 0, "empty layout");
        layout.Reset(3); layout.Rebuild(1000, 0);
        Check(layout.PageAt(-100) == 0 && layout.PageAt(layout.Total()) == 2, "document bounds");
        Check(layout.PageAt(layout.Top(1)) == 1, "page boundary");
        auto oldHeight = layout.Height(1);
        layout.Learn(0, 600, 400); layout.Learn(1, 300, 900); layout.Rebuild(1000, 0);
        Check(layout.Height(1) > oldHeight && layout.Height(2) == layout.Height(0), "mixed sizes and estimates");
        Check(layout.Width(0) == layout.Width(1), "fit width");
        auto position = layout.Top(1) + layout.Height(1) / 3;
        double fraction = (position - layout.Top(1)) / static_cast<double>(layout.Height(1));
        layout.Rebuild(1000, 2);
        auto restored = layout.Top(1) + static_cast<std::int64_t>(fraction * layout.Height(1));
        Check(layout.PageAt(restored) == 1, "zoom anchor page");
        Check(std::abs((restored-layout.Top(1))/static_cast<double>(layout.Height(1))-fraction) < 0.001, "zoom anchor fraction");
        layout.Reset(200000); layout.Learn(0, 1, 10000000); layout.Rebuild(1000, 0);
        Check(layout.Total() > 2147483647LL, "64-bit scroll extent");
        Check(layout.PageAt(layout.Top(199999)) == 199999, "last page in large layout");
        Check(layout.Width(0) * static_cast<long long>(layout.Height(0)) <= 12000000, "bitmap bound");
        std::vector<TextBox> boxes{{.1, .1, .2, .2}, {.21, .1, .3, .2}, {}, {.1, .3, .2, .4}};
        Check(NearestCharacter(boxes, .25, .15, 1) == 1, "character hit");
        Check(NearestCharacter(boxes, .15, .35, 1) == 3, "line hit");
        Check(NearestCharacter({}, .1, .1, 1) == -1, "empty text page");
        Check(SelectionBoxes(boxes, 0, 3).size() == 2, "line selection merging");
        Check(SelectionBoxes(boxes, 3, 1).empty(), "empty selection interval");
        std::puts("layout: bounds, mixed pages, estimates, zoom anchor and 64-bit extent OK"); return 0;
    } catch (const std::exception& error) { std::fprintf(stderr, "%s\n", error.what()); return 1; }
}
