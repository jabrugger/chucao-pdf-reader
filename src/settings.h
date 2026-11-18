#pragma once
#include <windows.h>
#include <optional>
#include <string>

// Small, local preferences file. It never contains document contents, paths or
// passwords; today it only remembers the chosen theme.
struct AppSettings {
    bool dark = false;
};

class SettingsStore {
    std::wstring filename_;
    static constexpr DWORD FileLimit = 4096;
public:
    explicit SettingsStore(std::wstring filename) : filename_(std::move(filename)) {}
    std::optional<AppSettings> Read() const {
        if (filename_.empty()) return {};
        HANDLE file = CreateFileW(filename_.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
            nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (file == INVALID_HANDLE_VALUE) return {};
        LARGE_INTEGER size{}; DWORD read = 0;
        if (!GetFileSizeEx(file, &size) || size.QuadPart <= 0 || size.QuadPart > FileLimit) { CloseHandle(file); return {}; }
        std::string data(static_cast<size_t>(size.QuadPart), 0);
        bool ok = ReadFile(file, &data[0], static_cast<DWORD>(data.size()), &read, nullptr) != 0;
        CloseHandle(file);
        if (!ok || read != data.size()) return {};
        const std::string header = "ChucaoSettings 1\n";
        if (data.compare(0, header.size(), header) != 0) return {};
        auto line = data.substr(header.size());
        while (!line.empty() && (line.back() == '\n' || line.back() == '\r')) line.pop_back();
        AppSettings settings;
        if (line == "theme=dark") settings.dark = true;
        else if (line == "theme=light") settings.dark = false;
        else return {};
        return settings;
    }
    bool Write(const AppSettings& settings) const {
        if (filename_.empty()) return false;
        auto slash = filename_.find_last_of(L"\\/");
        if (slash == std::wstring::npos) return false;
        if (!CreateDirectoryW(filename_.substr(0, slash).c_str(), nullptr) && GetLastError() != ERROR_ALREADY_EXISTS) return false;
        std::string data = std::string("ChucaoSettings 1\n") + (settings.dark ? "theme=dark\n" : "theme=light\n");
        auto temporary = filename_ + L"." + std::to_wstring(GetCurrentProcessId()) + L"." + std::to_wstring(GetTickCount64()) + L".tmp";
        HANDLE file = CreateFileW(temporary.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
        bool ok = file != INVALID_HANDLE_VALUE;
        if (ok) {
            DWORD written = 0;
            ok = WriteFile(file, data.data(), static_cast<DWORD>(data.size()), &written, nullptr) != 0 &&
                written == data.size() && FlushFileBuffers(file);
            CloseHandle(file);
            if (ok) {
                ok = false;
                for (int attempt = 0; attempt < 5 && !ok; ++attempt) {
                    ok = MoveFileExW(temporary.c_str(), filename_.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != 0;
                    if (ok) break;
                    DWORD error = GetLastError();
                    if (error != ERROR_SHARING_VIOLATION && error != ERROR_ACCESS_DENIED) break;
                    Sleep(20 * (attempt + 1));
                }
            }
            if (!ok) DeleteFileW(temporary.c_str());
        }
        return ok;
    }
};
