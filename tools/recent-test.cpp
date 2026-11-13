#include "../src/recent-files.h"
#include <cstdio>
#include <stdexcept>
void Check(bool ok, const char* message) { if (!ok) throw std::runtime_error(message); }
int wmain(int argc, wchar_t** argv) {
    if (argc != 2) return 2;
    try {
        std::vector<ReadingState> entries;
        for (int i = 0; i < 15; ++i) entries.push_back({L"C:\\NoExiste\\" + std::to_wstring(i) + L".pdf", 2});
        auto recent = RecentFiles(entries);
        Check(recent.size() == 10 && recent.front().path == entries.back().path && recent.back().path == entries[5].path, "newest ten");
        ReadingState current{L"c:/noexiste/1.pdf", 2};
        recent = RecentFiles(entries, current);
        Check(recent.front().path == L"c:\\noexiste\\1.pdf" && recent.size() == 10, "active document first, normalized");
        entries = {{L"C:\\A\\libro.pdf", 2}, {L"C:\\B\\libro.pdf", 2}};
        Check(RecentFiles(entries, entries[0]).size() == 2, "active document deduplicated");
        Check(RecentLabel(entries[0].path, 0) != RecentLabel(entries[1].path, 0), "same name distinguished by folder");
        Check(RecentLabel(L"C:\\A&B\\café.pdf", 9).find(L"1&0  café.pdf  —  C:\\A&&B") == 0, "Unicode, accelerator and escaped ampersand");
        auto label = RecentLabel(L"C:\\" + std::wstring(24, L'x') + L"\U0001F426" + std::wstring(70, L'y') + L".pdf", 0);
        for (size_t i = 0; i < label.size(); ++i) {
            if (label[i] >= 0xd800 && label[i] <= 0xdbff) { Check(i + 1 < label.size() && label[i+1] >= 0xdc00 && label[i+1] <= 0xdfff, "no split surrogate"); ++i; }
            else Check(label[i] < 0xdc00 || label[i] > 0xdfff, "no orphan surrogate");
        }
        std::wstring folder = argv[1];
        ReadingStateStore store(folder + L"\\reading-state.dat");
        Check(store.Clear() && store.Read().empty(), "clear profile");
        for (const auto& path : {folder + L"\\A\\libro.pdf", folder + L"\\B\\libro.pdf", folder + L"\\NoDisponible\\ausente.pdf"})
            Check(store.Update({path, 2}), "seed metadata without opening PDFs");
        Check(store.Read().size() == 3, "reusable after clear");
        std::puts("recents: ordering, bounds, deduplication, labels, Unicode and clear OK"); return 0;
    } catch (const std::exception& error) { std::fprintf(stderr, "%s\n", error.what()); return 1; }
}
