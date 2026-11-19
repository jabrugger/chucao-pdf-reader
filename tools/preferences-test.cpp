#include "../src/settings.h"
#include "../src/session-state.h"
#include <cstdio>
#include <stdexcept>
void Check(bool ok, const char* message) { if (!ok) throw std::runtime_error(message); }
void Raw(const std::wstring& name, const std::string& data) {
    HANDLE file = CreateFileW(name.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    Check(file != INVALID_HANDLE_VALUE, "create fixture"); DWORD written = 0;
    bool ok = WriteFile(file, data.data(), static_cast<DWORD>(data.size()), &written, nullptr) != 0;
    CloseHandle(file); Check(ok && written == data.size(), "write fixture");
}
int wmain(int argc, wchar_t** argv) {
    if (argc != 2) return 2;
    try {
        std::wstring folder = argv[1]; CreateDirectoryW(folder.c_str(), nullptr);
        auto settingsFile = folder + L"\\unit-settings.dat", sessionFile = folder + L"\\unit-session.dat";
        SettingsStore settings(settingsFile); SessionStore sessions(sessionFile);
        Raw(settingsFile, "\xef\xbb\xbf" "ChucaoSettings 1\r\ntheme=dark\r\n");
        auto read = settings.Read(); Check(read && read->dark && !read->restoreSession, "legacy theme and safe default");
        Check(settings.Write({false, true}), "save restore preference"); read = settings.Read();
        Check(read && !read->dark && read->restoreSession, "preferences round trip");
        Check(settings.Write({true, false, true}), "save system theme"); read = settings.Read();
        Check(read && read->followSystem && !read->restoreSession, "system theme round trip");
        Raw(settingsFile, "ChucaoSettings 1\ntheme=system\nrestore=1\n"); read = settings.Read();
        Check(read && read->followSystem && read->restoreSession, "system theme with restore");
        Raw(settingsFile, "ChucaoSettings 1\ntheme=dark\n"); read = settings.Read();
        Check(read && read->dark && !read->followSystem, "explicit theme does not follow system");
        Check(read && read->autoShowOutline, "index opens automatically by default");
        Check(settings.Write({false, false, false, false}), "save manual index"); read = settings.Read();
        Check(read && !read->autoShowOutline, "manual index round trip");
        Raw(settingsFile, "ChucaoSettings 1\ntheme=light\noutline=0\nrestore=1\n"); read = settings.Read();
        Check(read && !read->autoShowOutline && read->restoreSession, "outline preference alongside restore");
        Raw(settingsFile, "ChucaoSettings 1\ntheme=dark\noutline=maybe\n"); Check(!settings.Read(), "invalid outline preference ignored safely");
        Raw(settingsFile, "ChucaoSettings 1\ntheme=dark\nrestore=oops\n"); Check(!settings.Read(), "invalid restore ignored safely");
        Check(settings.Write({false, false, false, true, true, {10, 20, 1010, 720}, false}), "save window placement");
        read = settings.Read();
        Check(read && read->hasWindow && read->windowRect.left == 10 && read->windowRect.bottom == 720 && !read->maximized, "window placement round trip");
        Raw(settingsFile, "ChucaoSettings 1\ntheme=dark\nwindow=10 20 1010 720 1\n"); read = settings.Read();
        Check(read && read->hasWindow && read->maximized, "maximized window flag");
        Raw(settingsFile, "ChucaoSettings 1\ntheme=dark\nwindow=oops\n"); Check(!settings.Read(), "invalid window ignored safely");
        Raw(settingsFile, "ChucaoSettings 1\ntheme=dark\nwindow=10 20 20 20 0\n"); Check(!settings.Read(), "degenerate window ignored safely");
        SavedSession session{{folder + L"\\lectura-café.pdf", folder + L"\\漢字.pdf"}, 1};
        Check(sessions.Write(session), "save Unicode session"); auto saved = sessions.Read();
        Check(saved.paths.size() == 2 && saved.active == 1 && saved.paths[1] == ReadingStateStore::NormalizePath(session.paths[1]), "paths, order and active tab round trip");
        session.active = 2; Check(!sessions.Write(session), "invalid active tab rejected");
        session.active = 0; session.paths.resize(65, L"C:\\x.pdf"); Check(!sessions.Write(session), "bounded session");
        Raw(sessionFile, "ChucaoSession 1\n5\n\"C:\\\\x.pdf\"\n"); Check(sessions.Read().paths.empty(), "invalid active record ignored");
        Raw(sessionFile, "ChucaoSession 1\n0\n\"\xff\"\n"); Check(sessions.Read().paths.empty(), "invalid UTF-8 ignored");
        Raw(sessionFile, std::string(1024 * 1024 + 1, 'x')); Check(sessions.Read().paths.empty(), "oversized session ignored");
        Check(sessions.Write({}) && sessions.Read().paths.empty(), "clear and recover corrupt session");
        Check(!SessionStore(folder).Write({}), "write failure preserves directory");
        std::puts("preferences: legacy, opt-in, window placement, Unicode, active tab, bounds, corruption and clear OK"); return 0;
    } catch (const std::exception& error) { std::fprintf(stderr, "%s\n", error.what()); return 1; }
}
