#pragma once
#include <fpdf_doc.h>
#include <functional>
#include <cwchar>
#include <set>
#include <string>
#include <vector>
#include <utility>

struct OutlineEntry { std::wstring title; int parent = -1, page = -1; };
struct DocumentOutline { std::vector<OutlineEntry> entries; bool limited = false; };

// Called only by the PDFium worker. No page loading or external actions.
inline DocumentOutline ReadOutline(FPDF_DOCUMENT pdf, const std::function<bool()>& cancelled) {
    DocumentOutline result;
    struct Pending { FPDF_BOOKMARK bookmark; int parent, depth; };
    std::vector<Pending> pending{{FPDFBookmark_GetFirstChild(pdf, nullptr), -1, 0}};
    std::set<FPDF_BOOKMARK> visited;
    const int pages = FPDF_GetPageCount(pdf);
    while (!pending.empty() && !cancelled()) {
        auto item = pending.back(); pending.pop_back();
        if (!item.bookmark) continue;
        if (result.entries.size() >= 4096) { result.limited = true; break; }
        if (!visited.insert(item.bookmark).second) { result.limited = true; continue; }
        auto bytes = FPDFBookmark_GetTitle(item.bookmark, nullptr, 0);
        std::wstring title = L"Sin título";
        if (bytes >= 2 && bytes <= 2048 && bytes % 2 == 0) {
            title.assign(bytes / 2, L'\0');
            FPDFBookmark_GetTitle(item.bookmark, &title[0], bytes);
            title.resize(wcsnlen(title.c_str(), title.size()));
            for (auto& c : title) if (c < L' ') c = L' ';
            if (title.empty()) title = L"Sin título";
        } else if (bytes > 2048) { title = L"Título demasiado largo"; result.limited = true; }
        FPDF_DEST destination = nullptr;
        auto action = FPDFBookmark_GetAction(item.bookmark);
        if (action) {
            if (FPDFAction_GetType(action) == PDFACTION_GOTO) destination = FPDFAction_GetDest(pdf, action);
        } else destination = FPDFBookmark_GetDest(pdf, item.bookmark);
        int page = destination ? FPDFDest_GetDestPageIndex(pdf, destination) : -1;
        if (page < 0 || page >= pages) page = -1;
        int index = static_cast<int>(result.entries.size());
        result.entries.push_back({std::move(title), item.parent, page});
        pending.push_back({FPDFBookmark_GetNextSibling(pdf, item.bookmark), item.parent, item.depth});
        auto child = FPDFBookmark_GetFirstChild(pdf, item.bookmark);
        if (child && item.depth < 63) pending.push_back({child, index, item.depth + 1});
        else if (child) result.limited = true;
    }
    return result;
}
