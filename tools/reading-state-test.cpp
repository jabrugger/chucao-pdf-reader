#include "../src/reading-state.h"
#include <cstdio>
#include <stdexcept>
#include <limits>
#include <thread>
#include <atomic>

void Check(bool ok, const char* message) { if (!ok) throw std::runtime_error(message); }
void Write(const std::wstring& filename, const std::string& data) {
    HANDLE file = CreateFileW(filename.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    Check(file != INVALID_HANDLE_VALUE, "fixture create"); DWORD written = 0;
    bool ok = WriteFile(file, data.data(), static_cast<DWORD>(data.size()), &written, nullptr) != 0; CloseHandle(file); Check(ok && written == data.size(), "fixture write");
}
int wmain(int argc, wchar_t** argv) {
    if (argc != 2) return 2;
    try {
        std::wstring folder = argv[1]; CreateDirectoryW(folder.c_str(), nullptr);
        auto filename = folder + L"\\unit-" + std::to_wstring(GetCurrentProcessId()) + L".dat";
        ReadingStateStore first(filename), second(filename);
        Check(first.Read().empty(), "missing profile is empty");
        ReadingState a{L"C:\\Libros\\lectura-café.pdf", 410, 37, 2.5, .375, .65, 0};
        ReadingState b{L"C:\\Libros\\otra lectura.pdf", 12, 9, 0, 0, .5, -24};
        Check(first.Update(a) && second.Update(b), "two writer instances");
        auto read = first.Find(L"c:/LIBROS/lectura-café.pdf", 410);
        Check(read && read->page == 37 && read->zoom == 2.5 && read->vertical == .375 && read->horizontal == .65, "Unicode, case, separators and round trip");
        a.page = 39; Check(first.Update(a) && second.Find(b.path, b.pages).has_value(), "merge does not lose other document");
        Check(!second.Find(a.path, 411), "changed page count skips stale position");
        auto invalid = a; invalid.zoom = (std::numeric_limits<double>::quiet_NaN)();
        Check(!first.Update(invalid), "reject non-finite zoom"); invalid = a; invalid.page = 999;
        Check(!first.Update(invalid), "reject out of range page");
        std::atomic<bool> merged{true};
        auto parallel = [&](int group) {
            for (int i = 0; i < 5; ++i) {
                auto entry = a; entry.path = L"C:\\Libros\\parallel-" + std::to_wstring(group) + L"-" + std::to_wstring(i) + L".pdf";
                if (!ReadingStateStore(filename).Update(entry)) merged = false;
            }
        };
        std::thread writerA(parallel, 0), writerB(parallel, 1); writerA.join(); writerB.join();
        Check(merged, "parallel writers complete within bounded wait");
        Check(first.Read().size() == 12, "parallel writers preserve all records");
        // A lock held by slow I/O longer than the old half-second budget must still be tolerated.
        std::atomic<bool> locked{false};
        std::thread holder([&] {
            HANDLE handle = CreateFileW((filename + L".lock").c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
            locked = handle != INVALID_HANDLE_VALUE;
            if (locked) { Sleep(700); CloseHandle(handle); }
        });
        while (!locked) Sleep(1);
        auto slow = a; slow.path = L"C:\\Libros\\slow-lock.pdf";
        Check(ReadingStateStore(filename).Update(slow), "slow lock holder tolerated");
        holder.join();
        Check(locked && first.Find(slow.path, slow.pages).has_value(), "slow save persisted");
        for (int i = 0; i < 130; ++i) { a.path = L"C:\\Libros\\" + std::to_wstring(i) + L".pdf"; Check(first.Update(a), "bounded history write"); }
        Check(first.Read().size() == 128 && !first.Find(L"C:\\Libros\\0.pdf", 410), "history evicts oldest at 128");
        Write(filename, "unknown version\n"); Check(first.Read().empty(), "unknown format ignored");
        Write(filename, "ChucaoReadingState 1\nzz 10 2 1 .2 .5 0\n00 10 2 1 .2 .5 0\n");
        Check(first.Read().empty(), "malformed path and NUL ignored");
        Write(filename, std::string(4u * 1024u * 1024u + 1, 'x')); Check(first.Read().empty(), "oversized profile ignored");
        Check(first.Update(b) && first.Find(b.path, b.pages), "recover from corrupt profile");
        Check(!ReadingStateStore(folder).Update(a), "unwritable destination does not overwrite directory");
        std::puts("reading state: Unicode, normalization, round trip, merge, limits, corruption, recovery and write failure OK"); return 0;
    } catch (const std::exception& error) { std::fprintf(stderr, "%s\n", error.what()); return 1; }
}
