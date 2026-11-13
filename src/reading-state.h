#pragma once
#include <windows.h>
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <locale>
#include <optional>
#include <sstream>
#include <string>
#include <vector>
#include <functional>

struct ReadingState {
    std::wstring path;
    int pages = 0, page = 0;
    double zoom = 0, vertical = 0, horizontal = .5;
    int gap = 0;
};

// Local metadata only: never document contents, search queries or passwords.
class ReadingStateStore {
    std::wstring filename_;
    static constexpr DWORD FileLimit = 4u * 1024u * 1024u;
    static bool Valid(const ReadingState& s) {
        return !s.path.empty() && s.path.size() <= 32767 && s.path.find(L'\0') == std::wstring::npos &&
            s.pages > 0 && s.pages <= 10000000 && s.page >= 0 && s.page < s.pages &&
            std::isfinite(s.zoom) && (s.zoom == 0 || (s.zoom >= .25 && s.zoom <= 4)) &&
            std::isfinite(s.vertical) && s.vertical >= 0 && s.vertical <= 1 &&
            std::isfinite(s.horizontal) && s.horizontal >= 0 && s.horizontal <= 1 && s.gap >= -24 && s.gap <= 24 &&
            (s.gap == 0 || s.vertical == 0 || s.vertical == 1);
    }
    static std::string Encode(const std::wstring& value) {
        int count = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, value.data(), static_cast<int>(value.size()), nullptr, 0, nullptr, nullptr);
        if (!count) return {};
        std::string bytes(count, 0), hex; hex.reserve(bytes.size() * 2);
        WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, value.data(), static_cast<int>(value.size()), &bytes[0], count, nullptr, nullptr);
        const char digits[] = "0123456789abcdef";
        for (unsigned char c : bytes) { hex += digits[c >> 4]; hex += digits[c & 15]; }
        return hex;
    }
    static std::wstring Decode(const std::string& hex) {
        if (hex.empty() || hex.size() > 262136 || hex.size() % 2) return {};
        auto digit = [](char c) { return c >= '0' && c <= '9' ? c - '0' : c >= 'a' && c <= 'f' ? c - 'a' + 10 : -1; };
        std::string bytes; bytes.reserve(hex.size() / 2);
        for (size_t i = 0; i < hex.size(); i += 2) {
            int high = digit(hex[i]), low = digit(hex[i + 1]); if (high < 0 || low < 0) return {};
            bytes += static_cast<char>(high * 16 + low);
        }
        int count = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, bytes.data(), static_cast<int>(bytes.size()), nullptr, 0);
        if (count <= 0 || count > 32767) return {};
        std::wstring result(count, 0); MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, bytes.data(), static_cast<int>(bytes.size()), &result[0], count);
        return result;
    }
    static bool SamePath(const std::wstring& a, const std::wstring& b) {
        return CompareStringOrdinal(a.c_str(), static_cast<int>(a.size()), b.c_str(), static_cast<int>(b.size()), TRUE) == CSTR_EQUAL;
    }
    static std::string Serialize(const std::vector<ReadingState>& entries) {
        std::ostringstream output; output.imbue(std::locale::classic()); output << "ChucaoReadingState 1\n" << std::setprecision(17);
        for (const auto& state : entries) output << Encode(state.path) << '\t' << state.pages << '\t' << state.page << '\t' << state.zoom << '\t'
            << state.vertical << '\t' << state.horizontal << '\t' << state.gap << '\n';
        return output.str();
    }
public:
    explicit ReadingStateStore(std::wstring filename) : filename_(std::move(filename)) {}
    static std::wstring NormalizePath(std::wstring path) {
        if (path.empty() || path.size() > 32767 || path.find(L'\0') != std::wstring::npos) return {};
        std::replace(path.begin(), path.end(), L'/', L'\\');
        DWORD needed = GetFullPathNameW(path.c_str(), 0, nullptr, nullptr);
        if (!needed || needed > 32768) return {};
        std::wstring full(needed, 0); DWORD length = GetFullPathNameW(path.c_str(), needed, &full[0], nullptr);
        if (!length || length >= needed) return {}; full.resize(length); return full;
    }
    std::vector<ReadingState> Read() const {
        std::vector<ReadingState> entries;
        HANDLE file = CreateFileW(filename_.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (file == INVALID_HANDLE_VALUE) return entries;
        LARGE_INTEGER size{}; DWORD read = 0;
        if (!GetFileSizeEx(file, &size) || size.QuadPart < 0 || size.QuadPart > FileLimit) { CloseHandle(file); return entries; }
        std::string data(static_cast<size_t>(size.QuadPart), 0);
        bool ok = data.empty() || ReadFile(file, &data[0], static_cast<DWORD>(data.size()), &read, nullptr);
        CloseHandle(file); if (!ok || read != data.size()) return entries;
        std::istringstream lines(data); std::string line;
        if (!std::getline(lines, line) || line != "ChucaoReadingState 1") return entries;
        for (int number = 0; number < 512 && std::getline(lines, line); ++number) {
            if (line.size() > 262400) continue;
            std::istringstream fields(line); fields.imbue(std::locale::classic());
            ReadingState state; std::string key, extra;
            if (!(fields >> key >> state.pages >> state.page >> state.zoom >> state.vertical >> state.horizontal >> state.gap) || (fields >> extra)) continue;
            state.path = Decode(key); if (!Valid(state)) continue;
            state.path = NormalizePath(state.path); if (!Valid(state)) continue;
            entries.erase(std::remove_if(entries.begin(), entries.end(), [&](const ReadingState& old) { return SamePath(old.path, state.path); }), entries.end());
            entries.push_back(std::move(state)); if (entries.size() > 128) entries.erase(entries.begin());
        }
        return entries;
    }
    std::optional<ReadingState> Find(const std::wstring& path, int pages) const {
        auto key = NormalizePath(path); if (key.empty()) return {};
        auto entries = Read();
        for (auto i = entries.rbegin(); i != entries.rend(); ++i) if (SamePath(i->path, key) && i->pages == pages) return *i;
        return {};
    }
private:
    bool Change(const std::function<void(std::vector<ReadingState>&)>& edit) const {
        if (filename_.empty()) return false;
        auto slash = filename_.find_last_of(L"\\/"); if (slash == std::wstring::npos) return false;
        auto folder = filename_.substr(0, slash);
        if (!CreateDirectoryW(folder.c_str(), nullptr) && GetLastError() != ERROR_ALREADY_EXISTS) return false;
        // Merge under an exclusive lock so independent reader windows retain each other's entries.
        HANDLE lock = INVALID_HANDLE_VALUE;
        for (int attempt = 0; attempt < 50; ++attempt) {
            lock = CreateFileW((filename_ + L".lock").c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
            if (lock != INVALID_HANDLE_VALUE) break;
            if (GetLastError() != ERROR_SHARING_VIOLATION) return false;
            Sleep(10);
        }
        if (lock == INVALID_HANDLE_VALUE) return false;
        auto entries = Read();
        edit(entries); while (entries.size() > 128) entries.erase(entries.begin());
        auto data = Serialize(entries);
        while (data.size() > FileLimit && entries.size() > 1) { entries.erase(entries.begin()); data = Serialize(entries); }
        auto temporary = filename_ + L"." + std::to_wstring(GetCurrentProcessId()) + L"." + std::to_wstring(GetTickCount64()) + L".tmp";
        HANDLE file = CreateFileW(temporary.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
        bool ok = file != INVALID_HANDLE_VALUE;
        if (ok) {
            DWORD written = 0; ok = WriteFile(file, data.data(), static_cast<DWORD>(data.size()), &written, nullptr) && written == data.size() && FlushFileBuffers(file);
            CloseHandle(file);
            if (ok) ok = MoveFileExW(temporary.c_str(), filename_.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != 0;
            if (!ok) DeleteFileW(temporary.c_str());
        }
        CloseHandle(lock); return ok;
    }
public:
    bool Update(ReadingState state) const {
        state.path = NormalizePath(state.path); if (!Valid(state) || Encode(state.path).empty()) return false;
        return Change([&](std::vector<ReadingState>& entries) {
            entries.erase(std::remove_if(entries.begin(), entries.end(), [&](const ReadingState& old) { return SamePath(old.path, state.path); }), entries.end());
            entries.push_back(state);
        });
    }
    bool Clear() const { return Change([](std::vector<ReadingState>& entries) { entries.clear(); }); }
};
