#pragma once
#include "reading-state.h"
#include <iomanip>

struct SavedSession { std::vector<std::wstring> paths; int active = 0; };
class SessionStore {
    std::wstring filename_;
    static std::string Encode(const std::wstring& value) {
        int size = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, value.data(), static_cast<int>(value.size()), nullptr, 0, nullptr, nullptr);
        if (!size) return {};
        std::string bytes(size, 0); WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, value.data(), static_cast<int>(value.size()), &bytes[0], size, nullptr, nullptr); return bytes;
    }
    static std::wstring Decode(const std::string& bytes) {
        if (bytes.empty() || bytes.size() > 131068 || bytes.find('\0') != std::string::npos) return {};
        int size = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, bytes.data(), static_cast<int>(bytes.size()), nullptr, 0);
        if (size <= 0 || size > 32767) return {};
        std::wstring path(size, 0); MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, bytes.data(), static_cast<int>(bytes.size()), &path[0], size);
        for (wchar_t c : path) if (c < 32) return {};
        return ReadingStateStore::NormalizePath(path);
    }
public:
    explicit SessionStore(std::wstring filename) : filename_(std::move(filename)) {}
    SavedSession Read() const {
        SavedSession result;
        HANDLE file = CreateFileW(filename_.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (file == INVALID_HANDLE_VALUE) return result;
        LARGE_INTEGER size{}; DWORD read = 0;
        if (!GetFileSizeEx(file, &size) || size.QuadPart <= 0 || size.QuadPart > 1024 * 1024) { CloseHandle(file); return result; }
        std::string bytes(static_cast<size_t>(size.QuadPart), 0);
        bool ok = ReadFile(file, &bytes[0], static_cast<DWORD>(bytes.size()), &read, nullptr) != 0; CloseHandle(file);
        if (!ok || read != bytes.size()) return result;
        std::istringstream input(bytes); std::string header, line;
        if (!std::getline(input, header) || header != "ChucaoSession 1" || !std::getline(input, line)) return result;
        std::istringstream number(line); std::string extra;
        if (!(number >> result.active) || (number >> extra) || result.active < 0 || result.active >= 64) return {};
        while (std::getline(input, line)) {
            if (result.paths.size() >= 64) return {};
            std::istringstream field(line); std::string encoded;
            if (!(field >> std::quoted(encoded)) || (field >> extra)) return {};
            auto path = Decode(encoded); if (path.empty()) return {};
            for (const auto& existing : result.paths) if (CompareStringOrdinal(existing.c_str(), -1, path.c_str(), -1, TRUE) == CSTR_EQUAL) return {};
            result.paths.push_back(std::move(path));
        }
        if (!result.paths.empty() && result.active >= static_cast<int>(result.paths.size())) return {};
        return result;
    }
    bool Write(const SavedSession& session) const {
        if (filename_.empty() || session.paths.size() > 64 || session.active < 0 || session.active >= 64 ||
            (session.paths.empty() ? session.active != 0 : session.active >= static_cast<int>(session.paths.size()))) return false;
        std::ostringstream output; output << "ChucaoSession 1\n" << session.active << '\n';
        for (const auto& path : session.paths) {
            auto encoded = Encode(ReadingStateStore::NormalizePath(path));
            if (encoded.empty() || Decode(encoded).empty()) return false;
            output << std::quoted(encoded) << '\n';
        }
        auto data = output.str(); if (data.size() > 1024 * 1024) return false;
        auto slash = filename_.find_last_of(L"\\/"); if (slash == std::wstring::npos) return false;
        if (!CreateDirectoryW(filename_.substr(0, slash).c_str(), nullptr) && GetLastError() != ERROR_ALREADY_EXISTS) return false;
        auto temporary = filename_ + L"." + std::to_wstring(GetCurrentProcessId()) + L"." + std::to_wstring(GetTickCount64()) + L".tmp";
        HANDLE file = CreateFileW(temporary.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (file == INVALID_HANDLE_VALUE) return false;
        DWORD written = 0; bool ok = WriteFile(file, data.data(), static_cast<DWORD>(data.size()), &written, nullptr) && written == data.size() && FlushFileBuffers(file);
        CloseHandle(file);
        if (ok) ok = MoveFileExW(temporary.c_str(), filename_.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != 0;
        if (!ok) DeleteFileW(temporary.c_str()); return ok;
    }
};
