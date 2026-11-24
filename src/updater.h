#pragma once
// Opt-in "check for updates" against the GitHub releases API.
// Internal fragment of main.cpp: include it only from there, after the Windows
// headers and app-info.h. Only a single GET of the latest release metadata is
// performed; nothing is downloaded or installed, and no identifier is sent.
#include <winhttp.h>
#pragma comment(lib, "winhttp.lib")

namespace {
struct UpdateInfo {
    bool checked = false;   // a release payload was read and its tag parsed
    bool newer = false;     // the published tag is above CHUCAO_VERSION_STRING
    std::wstring latest;    // e.g. L"v0.2.9"
    std::wstring url;       // releases page to open in the browser
};

// Parse "v1.2.3" or "1.2.3" into three numeric components; missing pieces are 0.
bool ParseVersionTriple(const std::wstring& tag, int out[3]) {
    out[0] = out[1] = out[2] = 0;
    size_t i = 0;
    if (i < tag.size() && (tag[i] == L'v' || tag[i] == L'V')) ++i;
    for (int part = 0; part < 3; ++part) {
        int value = 0; bool digit = false;
        while (i < tag.size() && tag[i] >= L'0' && tag[i] <= L'9') { value = value * 10 + (tag[i] - L'0'); digit = true; ++i; }
        if (!digit) break;
        out[part] = value;
        if (i < tag.size() && tag[i] == L'.') ++i; else break;
    }
    return true;
}

bool VersionNewer(const std::wstring& latest, const std::wstring& current) {
    int a[3]{}, b[3]{};
    ParseVersionTriple(latest, a); ParseVersionTriple(current, b);
    for (int i = 0; i < 3; ++i) if (a[i] != b[i]) return a[i] > b[i];
    return false;
}

// Extract the "tag_name" string value from a GitHub releases/latest JSON body.
// Returns an empty string when the payload does not look like a release object.
std::wstring LatestTagFromJson(const std::string& json) {
    const std::string needle = "\"tag_name\":\"";
    size_t pos = json.find(needle);
    if (pos == std::string::npos) return {};
    pos += needle.size();
    size_t end = json.find('"', pos);
    if (end == std::string::npos) return {};
    std::string value = json.substr(pos, end - pos);
    int count = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, value.data(), static_cast<int>(value.size()), nullptr, 0);
    if (count <= 0) return {};
    std::wstring result(count, 0);
    MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, value.data(), static_cast<int>(value.size()), &result[0], count);
    return result;
}

// Derive /<owner>/<repo>/releases/latest from CHUCAO_REPOSITORY.
std::wstring ReleasesWebPath() {
    std::wstring repo = CHUCAO_WIDE(CHUCAO_REPOSITORY);
    size_t pos = repo.find(L"github.com/");
    if (pos == std::wstring::npos) return {};
    std::wstring slug = repo.substr(pos + 11);
    while (!slug.empty() && slug.back() == L'/') slug.pop_back();
    if (slug.empty() || slug.find(L'/') == std::wstring::npos) return {};
    return L"/" + slug + L"/releases/latest";
}

// Extract the tag segment from a GitHub redirect URL (.../releases/tag/<tag>).
std::wstring LatestTagFromLocation(const std::wstring& location) {
    const std::wstring marker = L"/releases/tag/";
    size_t pos = location.find(marker);
    if (pos == std::wstring::npos) return {};
    pos += marker.size();
    size_t end = location.find_first_of(L"/?#", pos);
    return end == std::wstring::npos ? location.substr(pos) : location.substr(pos, end - pos);
}

UpdateInfo FromReleaseTag(const std::wstring& tag) {
    UpdateInfo info;
    info.url = std::wstring(CHUCAO_WIDE(CHUCAO_REPOSITORY)) + L"/releases";
    if (tag.empty()) return info;
    info.checked = true;
    info.latest = tag;
    info.newer = VersionNewer(tag, CHUCAO_WIDE(CHUCAO_VERSION_STRING));
    return info;
}

// Evaluate a release payload against the current version.
UpdateInfo FromReleaseJson(const std::string& body) {
    return FromReleaseTag(LatestTagFromJson(body));
}

// Time-boxed HTTPS HEAD against github.com/<owner>/<repo>/releases/latest,
// reading the 302 Location header without hitting api.github.com's 60 req/h limit.
std::wstring UpdateRedirectLocation(const wchar_t* host, const std::wstring& path) {
    std::wstring location;
    HINTERNET session = WinHttpOpen(L"Chucao/" CHUCAO_WIDE(CHUCAO_VERSION_STRING),
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (!session) return location;
    HINTERNET connect = WinHttpConnect(session, host, INTERNET_DEFAULT_HTTPS_PORT, 0);
    if (connect) {
        HINTERNET request = WinHttpOpenRequest(connect, L"HEAD", path.c_str(), nullptr,
            WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE);
        if (request) {
            WinHttpSetTimeouts(request, 3000, 3000, 3000, 3000);
            DWORD disable = WINHTTP_DISABLE_REDIRECTS;
            WinHttpSetOption(request, WINHTTP_OPTION_DISABLE_FEATURE, &disable, sizeof(disable));
            if (WinHttpSendRequest(request, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0) &&
                WinHttpReceiveResponse(request, nullptr)) {
                DWORD size = 0;
                WinHttpQueryHeaders(request, WINHTTP_QUERY_LOCATION, WINHTTP_HEADER_NAME_BY_INDEX,
                    nullptr, &size, WINHTTP_NO_HEADER_INDEX);
                if (GetLastError() == ERROR_INSUFFICIENT_BUFFER && size >= sizeof(wchar_t) && size <= 4096) {
                    location.resize(size / sizeof(wchar_t));
                    if (WinHttpQueryHeaders(request, WINHTTP_QUERY_LOCATION, WINHTTP_HEADER_NAME_BY_INDEX,
                        &location[0], &size, WINHTTP_NO_HEADER_INDEX)) {
                        while (!location.empty() && location.back() == L'\0') location.pop_back();
                    } else {
                        location.clear();
                    }
                }
            }
            WinHttpCloseHandle(request);
        }
        WinHttpCloseHandle(connect);
    }
    WinHttpCloseHandle(session);
    return location;
}

UpdateInfo CheckForUpdate() {
    std::wstring path = ReleasesWebPath();
    if (path.empty()) return {};
    return FromReleaseTag(LatestTagFromLocation(UpdateRedirectLocation(L"github.com", path)));
}
} // namespace
