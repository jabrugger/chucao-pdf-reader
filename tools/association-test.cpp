#include <windows.h>
#include "../src/file-association.h"
#include <cstdio>
#include <stdexcept>

void Check(bool ok, const char* message) { if (!ok) throw std::runtime_error(message); }

int wmain() {
    FileAssociationTarget target{L".chucaotestpdf", L"Chucao.AssociationTest", L"Documento de prueba", L"Chucao Test"};
    try {
        std::wstring error;
        UnregisterFileAssociation(target, error);
        RegDeleteTreeW(HKEY_CURRENT_USER, ClassesPath(target.extension).c_str());
        RegDeleteTreeW(HKEY_CURRENT_USER, ProgIdPath(target).c_str());
        Check(AssociationState(target) == FileAssociationState::NotRegistered, "clean state");

        wchar_t path[32768]{};
        DWORD length = GetModuleFileNameW(nullptr, path, 32768);
        Check(length > 0 && length < 32768, "test executable path");
        Check(RegisterFileAssociation(path, target, error), "register association");
        Check(AssociationState(target) == FileAssociationState::Default, "registered as default");
        Check(EffectiveDefaultProgId(target) == target.progId, "extension points to the ProgID");

        std::wstring command, icon, description;
        Check(RegReadString(HKEY_CURRENT_USER, ProgIdPath(target) + L"\\shell\\open\\command", L"", command) &&
            command.find(path) != std::wstring::npos && command.find(L"%1") != std::wstring::npos, "open command");
        Check(RegReadString(HKEY_CURRENT_USER, ProgIdPath(target) + L"\\DefaultIcon", L"", icon) &&
            icon.find(path) != std::wstring::npos, "default icon");
        Check(RegReadString(HKEY_CURRENT_USER, ProgIdPath(target), L"", description) &&
            description == target.description, "ProgID description");

        Check(UnregisterFileAssociation(target, error), "unregister association");
        Check(AssociationState(target) == FileAssociationState::NotRegistered, "state after unregister");
        HKEY key = nullptr;
        Check(RegOpenKeyExW(HKEY_CURRENT_USER, ProgIdPath(target).c_str(), 0, KEY_READ, &key) != ERROR_SUCCESS,
            "ProgID tree removed");
        Check(!RegReadString(HKEY_CURRENT_USER, ClassesPath(target.extension), L"", description),
            "extension default removed");

        Check(RegisterFileAssociation(path, target, error), "register again");
        Check(RegWriteString(HKEY_CURRENT_USER, ClassesPath(target.extension), L"", L"Other.Application"),
            "simulate another default");
        Check(UnregisterFileAssociation(target, error), "unregister without stealing another default");
        Check(RegReadString(HKEY_CURRENT_USER, ClassesPath(target.extension), L"", description) &&
            description == L"Other.Application", "other default preserved");
        Check(!RegHasValue(HKEY_CURRENT_USER, ClassesPath(target.extension) + L"\\OpenWithProgids",
            target.progId.c_str()), "OpenWith entry removed");

        RegDeleteTreeW(HKEY_CURRENT_USER, ClassesPath(target.extension).c_str());
        RegDeleteTreeW(HKEY_CURRENT_USER, ProgIdPath(target).c_str());
        std::puts("association: register, default, command/icon, unregister, preserve foreign default OK");
        return 0;
    } catch (const std::exception& error) {
        RegDeleteTreeW(HKEY_CURRENT_USER, ClassesPath(target.extension).c_str());
        RegDeleteTreeW(HKEY_CURRENT_USER, ProgIdPath(target).c_str());
        std::fprintf(stderr, "%s\n", error.what());
        return 1;
    }
}
