#pragma once
#include <windows.h>
#include <optional>
#include <sstream>
#include <string>

// Small, local preferences file. It never contains document contents, paths or
// passwords. Session paths are stored separately and only when enabled.
struct AppSettings {
    bool dark = false;
    bool restoreSession = false;
    // theme=system: the reader resolves light/dark from Windows; `dark` is then unused.
    bool followSystem = false;
    // true: the index opens docked when a PDF has bookmarks; false: only on hover/toggle.
    bool autoShowOutline = true;
    // Window placement in physical pixels; restored only if it lands on a monitor.
    bool hasWindow = false;
    RECT windowRect{};
    bool maximized = false;
    // true: consult the latest GitHub release on startup and offer to open the
    // releases page. Opt-in and off by default; no download or telemetry.
    bool checkUpdates = false;
    // Unix seconds of the last automatic check; limits it to once per day.
    long long updatesLastCheck = 0;
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
        // Tolerate a UTF-8 BOM and both LF/CRLF line endings so an edited file
        // never silently discards the whole preference.
        if (data.size() >= 3 && static_cast<unsigned char>(data[0]) == 0xEF &&
            static_cast<unsigned char>(data[1]) == 0xBB && static_cast<unsigned char>(data[2]) == 0xBF)
            data.erase(0, 3);
        std::istringstream lines(data);
        std::string line;
        auto strip = [](std::string& value) {
            while (!value.empty() && (value.back() == '\n' || value.back() == '\r')) value.pop_back();
        };
        if (!std::getline(lines, line)) return {};
        strip(line);
        if (line != "ChucaoSettings 1") return {};
        if (!std::getline(lines, line)) return {};
        strip(line);
        AppSettings settings;
        if (line == "theme=dark") settings.dark = true;
        else if (line == "theme=light") settings.dark = false;
        else if (line == "theme=system") settings.followSystem = true;
        else return {};
        while (std::getline(lines, line)) {
            strip(line);
            if (line == "restore=1") settings.restoreSession = true;
            else if (line == "restore=0") settings.restoreSession = false;
            else if (line == "outline=1") settings.autoShowOutline = true;
            else if (line == "outline=0") settings.autoShowOutline = false;
            else if (line == "updates=1") settings.checkUpdates = true;
            else if (line == "updates=0") settings.checkUpdates = false;
            else if (line.rfind("updates_last=", 0) == 0) {
                std::istringstream value(line.substr(13));
                long long seconds = 0; std::string extra;
                if (!(value >> seconds) || (value >> extra) || seconds < 0) return {};
                settings.updatesLastCheck = seconds;
            }
            else if (line.rfind("window=", 0) == 0) {
                std::istringstream window(line.substr(7));
                int left = 0, top = 0, right = 0, bottom = 0, maximized = 0;
                std::string extra;
                if (!(window >> left >> top >> right >> bottom >> maximized) || (window >> extra)) return {};
                int width = right - left, height = bottom - top;
                if (width < 200 || height < 150 || width > 20000 || height > 20000 ||
                    left < -100000 || top < -100000 || right > 100000 || bottom > 100000 ||
                    (maximized != 0 && maximized != 1)) return {};
                settings.hasWindow = true;
                settings.windowRect = {left, top, right, bottom};
                settings.maximized = maximized != 0;
            }
            else if (line.empty()) continue;
            else return {};
        }
        return settings;
    }
    bool Write(const AppSettings& settings) const {
        if (filename_.empty()) return false;
        auto slash = filename_.find_last_of(L"\\/");
        if (slash == std::wstring::npos) return false;
        if (!CreateDirectoryW(filename_.substr(0, slash).c_str(), nullptr) && GetLastError() != ERROR_ALREADY_EXISTS) return false;
        std::string data = std::string("ChucaoSettings 1\n") + (settings.followSystem ? "theme=system\n" : settings.dark ? "theme=dark\n" : "theme=light\n") + (settings.restoreSession ? "restore=1\n" : "restore=0\n") + (settings.autoShowOutline ? "outline=1\n" : "outline=0\n") + (settings.checkUpdates ? "updates=1\n" : "updates=0\n") + "updates_last=" + std::to_string(settings.updatesLastCheck) + "\n";
        if (settings.hasWindow)
            data += "window=" + std::to_string(settings.windowRect.left) + " " + std::to_string(settings.windowRect.top) + " " +
                std::to_string(settings.windowRect.right) + " " + std::to_string(settings.windowRect.bottom) + " " +
                (settings.maximized ? "1\n" : "0\n");
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
