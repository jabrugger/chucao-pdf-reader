#include "../src/document-layout.h"
#include "../src/text-geometry.h"
#include <cstdio>
#include <string>
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
        Check(layout.MaxHorizontal(1000) == 0 && layout.Left(0, 1000) >= DocumentLayout::Gap, "fit horizontal bounds");
        auto position = layout.Top(1) + layout.Height(1) / 3;
        double fraction = (position - layout.Top(1)) / static_cast<double>(layout.Height(1));
        layout.Rebuild(1000, 2);
        Check(layout.MaxHorizontal(1000) > 0, "zoom horizontal overflow");
        int extent = layout.HorizontalExtent(1000);
        for (int i = 0; i < layout.Count(); ++i) {
            Check(layout.Left(i, 1000) >= DocumentLayout::Gap, "left edge reachable");
            Check(layout.Left(i, 1000, layout.MaxHorizontal(1000)) + layout.Width(i) <= 1000 - DocumentLayout::Gap, "right edge reachable");
            Check(layout.Left(i, 1000) * 2 + layout.Width(i) >= extent - 1, "mixed pages centered");
        }
        auto restored = layout.Top(1) + static_cast<std::int64_t>(fraction * layout.Height(1));
        Check(layout.PageAt(restored) == 1, "zoom anchor page");
        Check(std::abs((restored-layout.Top(1))/static_cast<double>(layout.Height(1))-fraction) < 0.001, "zoom anchor fraction");
        layout.Reset(200000); layout.Learn(0, 1, 10000000); layout.Rebuild(1000, 0);
        Check(layout.Total() > 2147483647LL, "64-bit scroll extent");
        Check(layout.PageAt(layout.Top(199999)) == 199999, "last page in large layout");
        Check(layout.Width(0) * static_cast<long long>(layout.Height(0)) <= 12000000, "bitmap bound");
        DocumentLayout fits; fits.Reset(1); fits.Learn(0, 612, 792);
        auto near = [](int value, int expected) { return std::abs(value - expected) <= 1; };
        fits.Rebuild(1000, 800, 0, ZoomMode::FitWidth); Check(near(fits.Width(0), 936) && near(fits.Height(0), 1211), "fit width page");
        fits.Rebuild(1000, 800, 0, ZoomMode::FitHeight); Check(near(fits.Width(0), 568) && near(fits.Height(0), 736), "fit height page");
        fits.Rebuild(1000, 800, 0, ZoomMode::FitPage); Check(near(fits.Width(0), 568) && near(fits.Height(0), 736), "fit whole page");
        fits.Rebuild(1000, 800, 2, ZoomMode::Manual); Check(near(fits.Width(0), 1224) && near(fits.Height(0), 1584), "manual zoom");
        Check(std::abs(fits.Scale(0) - 2) < 1e-9, "manual scale");
        Check(std::abs(fits.Scale(1) - 0) < 1e-9, "out-of-range scale");
        std::vector<TextBox> boxes{{.1, .1, .2, .2}, {.21, .1, .3, .2}, {}, {.1, .3, .2, .4}};
        Check(NearestCharacter(boxes, .25, .15, 1) == 1, "character hit");
        Check(NearestCharacter(boxes, .15, .35, 1) == 3, "line hit");
        Check(NearestCharacter({}, .1, .1, 1) == -1, "empty text page");
        Check(SelectionBoxes(boxes, 0, 3).size() == 2, "line selection merging");
        Check(SelectionBoxes(boxes, 3, 1).empty(), "empty selection interval");
        std::u32string sample = U"Chucao café\r\nSegunda línea";
        std::vector<unsigned int> text(sample.begin(), sample.end());
        int first = 0, last = 0;
        TextGeometry::MultiClickRange(text, 2, 1, first, last);
        Check(first == 0 && last == 5, "double click selects the word");
        TextGeometry::MultiClickRange(text, 8, 1, first, last);
        Check(first == 7 && last == 10, "accented word stays together");
        TextGeometry::MultiClickRange(text, 2, 2, first, last);
        Check(first == 0 && last == 10, "triple click selects the line");
        TextGeometry::MultiClickRange(text, 13, 2, first, last);
        Check(first == 13 && last == 25, "second line range");
        TextGeometry::MultiClickRange(text, 6, 1, first, last);
        Check(first == 6 && last == 6, "click on a space selects it");
        std::puts("layout: bounds, mixed pages, fit modes, estimates, zoom anchor and 64-bit extent OK"); return 0;
    } catch (const std::exception& error) { std::fprintf(stderr, "%s\n", error.what()); return 1; }
}
