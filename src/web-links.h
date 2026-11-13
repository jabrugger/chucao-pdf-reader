#pragma once
#include <windows.h>
#include <string>

// Accept explicit web URLs only. Never pass PDF actions or file paths to the shell.
inline bool IsWebUrl(const std::wstring& value) {
    if (value.empty() || value.size() > 8192) return false;
    size_t start = 0;
    if (value.size() >= 8 && CompareStringOrdinal(value.c_str(), 8, L"https://", 8, TRUE) == CSTR_EQUAL) start = 8;
    else if (value.size() >= 7 && CompareStringOrdinal(value.c_str(), 7, L"http://", 7, TRUE) == CSTR_EQUAL) start = 7;
    else return false;
    auto end = value.find_first_of(L"/?#", start);
    auto authority = value.substr(start, end == std::wstring::npos ? end : end - start);
    if (authority.empty() || authority.find(L'@') != std::wstring::npos || authority.front() == L':') return false;
    for (size_t i = 0; i < value.size(); ++i) {
        wchar_t c = value[i];
        if (c <= L' ' || c == 0x7f || c == L'\\' || c == L'"' || c == L'<' || c == L'>') return false;
        if (c >= 0xd800 && c <= 0xdbff) {
            if (++i >= value.size() || value[i] < 0xdc00 || value[i] > 0xdfff) return false;
        } else if (c >= 0xdc00 && c <= 0xdfff) return false;
    }
    return true;
}
inline std::wstring DecodeWebUrl(const std::string& bytes) {
    if (bytes.empty() || bytes.size() > 8192 || bytes.find('\0') != std::string::npos) return {};
    int count = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, bytes.data(), static_cast<int>(bytes.size()), nullptr, 0);
    if (count <= 0) return {};
    std::wstring value(count, 0);
    if (!MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, bytes.data(), static_cast<int>(bytes.size()), &value[0], count)) return {};
    return IsWebUrl(value) ? value : L"";
}
