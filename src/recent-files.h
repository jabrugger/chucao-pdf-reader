#pragma once
#include "reading-state.h"

inline std::vector<ReadingState> RecentFiles(std::vector<ReadingState> entries, const std::optional<ReadingState>& current = {}) {
    if (current) {
        auto value = *current; value.path = ReadingStateStore::NormalizePath(value.path);
        if (!value.path.empty()) {
            entries.erase(std::remove_if(entries.begin(), entries.end(), [&](const ReadingState& other) {
                return CompareStringOrdinal(other.path.c_str(), -1, value.path.c_str(), -1, TRUE) == CSTR_EQUAL;
            }), entries.end());
            entries.push_back(std::move(value));
        }
    }
    std::reverse(entries.begin(), entries.end());
    if (entries.size() > 10) entries.resize(10);
    return entries;
}
inline std::wstring RecentLabel(const std::wstring& path, size_t index) {
    auto compact = [](const std::wstring& value, size_t limit) {
        if (value.size() <= limit) return value;
        size_t left = limit / 2, right = value.size() - (limit - left - 1);
        if (left && value[left - 1] >= 0xd800 && value[left - 1] <= 0xdbff) --left;
        if (right < value.size() && value[right] >= 0xdc00 && value[right] <= 0xdfff) ++right;
        return value.substr(0, left) + L"…" + value.substr(right);
    };
    auto safe = [](const std::wstring& value) {
        std::wstring result;
        for (wchar_t c : value) { if (c == L'&') result += L'&'; result += c < L' ' ? L' ' : c; }
        return result;
    };
    auto slash = path.find_last_of(L"\\/");
    auto name = slash == std::wstring::npos ? path : path.substr(slash + 1);
    auto folder = slash == std::wstring::npos ? L"" : path.substr(0, slash);
    auto key = index == 9 ? L"1&0" : L"&" + std::to_wstring(index + 1);
    return key + L"  " + safe(compact(name, 50)) + (folder.empty() ? L"" : L"  —  " + safe(compact(folder, 55)));
}
