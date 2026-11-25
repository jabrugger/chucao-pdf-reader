#pragma once
#include <windows.h>
#include <shlobj.h>
#include <shellapi.h>
#include <string>

// Per-user file association for a PDF reader. The target is parameterized so the
// unit test can use a disposable extension and ProgID without touching .pdf.
struct FileAssociationTarget {
    std::wstring extension = L".pdf";
    std::wstring progId = L"Chucao.PDF";
    std::wstring description = L"Documento PDF";
    std::wstring applicationName = L"Chucao PDF Reader";
};

enum class FileAssociationState { NotRegistered, OpenWithOnly, Default };

inline bool RegReadString(HKEY root, const std::wstring& subkey, const wchar_t* value, std::wstring& result) {
    HKEY key = nullptr;
    if (RegOpenKeyExW(root, subkey.c_str(), 0, KEY_READ, &key) != ERROR_SUCCESS) return false;
    DWORD type = 0, size = 0;
    bool ok = RegQueryValueExW(key, value, nullptr, &type, nullptr, &size) == ERROR_SUCCESS &&
        (type == REG_SZ || type == REG_EXPAND_SZ) && size >= sizeof(wchar_t);
    if (ok) {
        result.assign(size / sizeof(wchar_t), 0);
        DWORD read = size;
        ok = RegQueryValueExW(key, value, nullptr, &type, reinterpret_cast<BYTE*>(&result[0]), &read) == ERROR_SUCCESS;
        while (!result.empty() && result.back() == L'\0') result.pop_back();
    }
    RegCloseKey(key);
    return ok && !result.empty();
}

inline bool RegWriteString(HKEY root, const std::wstring& subkey, const wchar_t* value, const std::wstring& data) {
    HKEY key = nullptr;
    if (RegCreateKeyExW(root, subkey.c_str(), 0, nullptr, 0, KEY_WRITE, nullptr, &key, nullptr) != ERROR_SUCCESS) return false;
    bool ok = RegSetValueExW(key, value, 0, REG_SZ, reinterpret_cast<const BYTE*>(data.c_str()),
        static_cast<DWORD>((data.size() + 1) * sizeof(wchar_t))) == ERROR_SUCCESS;
    RegCloseKey(key);
    return ok;
}

inline bool RegWriteEmpty(HKEY root, const std::wstring& subkey, const wchar_t* value) {
    HKEY key = nullptr;
    if (RegCreateKeyExW(root, subkey.c_str(), 0, nullptr, 0, KEY_WRITE, nullptr, &key, nullptr) != ERROR_SUCCESS) return false;
    bool ok = RegSetValueExW(key, value, 0, REG_NONE, nullptr, 0) == ERROR_SUCCESS;
    RegCloseKey(key);
    return ok;
}

inline bool RegHasValue(HKEY root, const std::wstring& subkey, const wchar_t* value) {
    HKEY key = nullptr;
    if (RegOpenKeyExW(root, subkey.c_str(), 0, KEY_READ, &key) != ERROR_SUCCESS) return false;
    bool ok = RegQueryValueExW(key, value, nullptr, nullptr, nullptr, nullptr) == ERROR_SUCCESS;
    RegCloseKey(key);
    return ok;
}

inline bool RegDeleteValueSafe(HKEY root, const std::wstring& subkey, const wchar_t* value) {
    HKEY key = nullptr;
    LSTATUS opened = RegOpenKeyExW(root, subkey.c_str(), 0, KEY_SET_VALUE, &key);
    if (opened == ERROR_FILE_NOT_FOUND || opened == ERROR_PATH_NOT_FOUND) return true;
    if (opened != ERROR_SUCCESS) return false;
    LSTATUS deleted = RegDeleteValueW(key, value);
    RegCloseKey(key);
    return deleted == ERROR_SUCCESS || deleted == ERROR_FILE_NOT_FOUND;
}

inline bool RegDeleteTreeSafe(HKEY root, const std::wstring& subkey) {
    LSTATUS status = RegDeleteTreeW(root, subkey.c_str());
    return status == ERROR_SUCCESS || status == ERROR_FILE_NOT_FOUND || status == ERROR_PATH_NOT_FOUND;
}

inline std::wstring ClassesPath(const std::wstring& extension) { return L"Software\\Classes\\" + extension; }
inline std::wstring ProgIdPath(const FileAssociationTarget& target) { return L"Software\\Classes\\" + target.progId; }
inline std::wstring UserChoicePath(const std::wstring& extension) {
    return L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\FileExts\\" + extension + L"\\UserChoice";
}

// Windows honors UserChoice before the plain .ext default, so report the value the
// shell would actually use when it is present.
inline std::wstring EffectiveDefaultProgId(const FileAssociationTarget& target) {
    std::wstring value;
    if (RegReadString(HKEY_CURRENT_USER, UserChoicePath(target.extension), L"ProgId", value)) return value;
    if (RegReadString(HKEY_CURRENT_USER, ClassesPath(target.extension), L"", value)) return value;
    return {};
}

inline FileAssociationState AssociationState(const FileAssociationTarget& target) {
    if (!target.progId.empty() && EffectiveDefaultProgId(target) == target.progId) return FileAssociationState::Default;
    if (RegHasValue(HKEY_CURRENT_USER, ClassesPath(target.extension) + L"\\OpenWithProgids", target.progId.c_str()))
        return FileAssociationState::OpenWithOnly;
    HKEY key = nullptr;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, ProgIdPath(target).c_str(), 0, KEY_READ, &key) == ERROR_SUCCESS) {
        RegCloseKey(key);
        return FileAssociationState::OpenWithOnly;
    }
    return FileAssociationState::NotRegistered;
}

inline bool RegisterFileAssociation(const std::wstring& exePath, const FileAssociationTarget& target, std::wstring& error) {
    error.clear();
    if (exePath.empty() || GetFileAttributesW(exePath.c_str()) == INVALID_FILE_ATTRIBUTES) {
        error = L"La ruta del ejecutable no es válida.";
        return false;
    }
    auto progId = ProgIdPath(target);
    auto command = L"\"" + exePath + L"\" \"%1\"";
    auto icon = L"\"" + exePath + L"\",0";
    bool ok = RegWriteString(HKEY_CURRENT_USER, progId, L"", target.description) &&
        RegWriteString(HKEY_CURRENT_USER, progId + L"\\DefaultIcon", L"", icon) &&
        RegWriteString(HKEY_CURRENT_USER, progId + L"\\Application", L"ApplicationName", target.applicationName) &&
        RegWriteString(HKEY_CURRENT_USER, progId + L"\\Application", L"ApplicationIcon", icon) &&
        RegWriteString(HKEY_CURRENT_USER, progId + L"\\shell\\open\\command", L"", command) &&
        RegWriteEmpty(HKEY_CURRENT_USER, ClassesPath(target.extension) + L"\\OpenWithProgids", target.progId.c_str()) &&
        RegWriteString(HKEY_CURRENT_USER, ClassesPath(target.extension), L"", target.progId);
    if (ok && target.extension == L".pdf") {
        std::wstring caps = L"Software\\ChucaoPDFReader\\Capabilities";
        RegWriteString(HKEY_CURRENT_USER, caps, L"ApplicationName", target.applicationName);
        RegWriteString(HKEY_CURRENT_USER, caps, L"ApplicationDescription", L"Lector de PDF rápido y con una interfaz limpia.");
        RegWriteString(HKEY_CURRENT_USER, caps + L"\\FileAssociations", target.extension.c_str(), target.progId);
        RegWriteString(HKEY_CURRENT_USER, L"Software\\RegisteredApplications", target.applicationName.c_str(), caps);
        RegWriteString(HKEY_CURRENT_USER, L"Software\\Classes\\Applications\\Chucao.exe\\shell\\open\\command", L"", command);
        RegWriteEmpty(HKEY_CURRENT_USER, L"Software\\Classes\\Applications\\Chucao.exe\\SupportedTypes", target.extension.c_str());
    }
    if (!ok) error = L"No se pudieron escribir las claves de asociación.";
    SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, nullptr, nullptr);
    return ok;
}

inline bool UnregisterFileAssociation(const FileAssociationTarget& target, std::wstring& error) {
    error.clear();
    bool ok = true;
    // If Windows chose Chucao (UserChoice), clear that record so the shell does not
    // keep pointing at a ProgID that is about to disappear. Another app's default
    // is never touched.
    if (EffectiveDefaultProgId(target) == target.progId) {
        if (!RegDeleteTreeSafe(HKEY_CURRENT_USER, UserChoicePath(target.extension))) {
            error = L"No se pudo quitar la asociación de PDF.";
            return false;
        }
    }
    std::wstring current;
    if (RegReadString(HKEY_CURRENT_USER, ClassesPath(target.extension), L"", current) && current == target.progId)
        ok = RegDeleteValueSafe(HKEY_CURRENT_USER, ClassesPath(target.extension), L"") && ok;
    ok = RegDeleteValueSafe(HKEY_CURRENT_USER, ClassesPath(target.extension) + L"\\OpenWithProgids", target.progId.c_str()) && ok;
    ok = RegDeleteTreeSafe(HKEY_CURRENT_USER, ProgIdPath(target)) && ok;
    if (target.extension == L".pdf") {
        ok = RegDeleteValueSafe(HKEY_CURRENT_USER, L"Software\\RegisteredApplications", target.applicationName.c_str()) && ok;
        ok = RegDeleteTreeSafe(HKEY_CURRENT_USER, L"Software\\ChucaoPDFReader") && ok;
        ok = RegDeleteTreeSafe(HKEY_CURRENT_USER, L"Software\\Classes\\Applications\\Chucao.exe") && ok;
    }
    if (!ok) error = L"No se pudo quitar la asociación de PDF.";
    SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, nullptr, nullptr);
    return ok;
}
