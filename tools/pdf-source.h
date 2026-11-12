#pragma once
#include <windows.h>
#include <fpdfview.h>
#include <algorithm>
#include <array>
#include <limits>
#include <vector>

// Experimental benchmark source, not used by the reader: read-ahead can hurt cold I/O.
class PdfSource {
    struct Block { unsigned long offset = 0; size_t valid = 0; unsigned long long used = 0; std::vector<unsigned char> bytes; };
    unsigned long blockSize_;
    HANDLE file_ = INVALID_HANDLE_VALUE;
    std::array<Block, 128> blocks_{};
    unsigned long long clock_ = 0;
    static int Read(void* context, unsigned long position, unsigned char* output, unsigned long size) {
        return static_cast<PdfSource*>(context)->ReadAt(position, output, size);
    }
    int ReadAt(unsigned long position, unsigned char* output, unsigned long size) {
        ++requests;
        if (position > access.m_FileLen || size > access.m_FileLen - position) return 0;
        while (size) {
            unsigned long offset = position - position % blockSize_;
            Block* block = nullptr;
            for (auto& candidate : blocks_) if (candidate.valid && candidate.offset == offset) { block = &candidate; break; }
            if (!block) {
                block = &*std::min_element(blocks_.begin(), blocks_.end(), [](const Block& a, const Block& b) { return a.used < b.used; });
                block->valid = 0;
                block->bytes.resize(blockSize_);
                LARGE_INTEGER at{}; at.QuadPart = offset; DWORD read = 0;
                DWORD wanted = std::min<unsigned long>(blockSize_, access.m_FileLen - offset);
                if (!SetFilePointerEx(file_, at, nullptr, FILE_BEGIN) || !ReadFile(file_, block->bytes.data(), wanted, &read, nullptr) || read != wanted) return 0;
                ++reads; bytesRead += read; block->offset = offset; block->valid = read;
            }
            block->used = ++clock_;
            size_t within = position - offset;
            if (within >= block->valid) return 0;
            auto amount = static_cast<unsigned long>(std::min<size_t>(size, block->valid - within));
            memcpy(output, block->bytes.data() + within, amount);
            output += amount; position += amount; size -= amount;
        }
        return 1;
    }
public:
    FPDF_FILEACCESS access{};
    unsigned long long requests = 0, reads = 0, bytesRead = 0;
    explicit PdfSource(unsigned long blockSize = 4096) : blockSize_(blockSize) {}
    PdfSource(const PdfSource&) = delete;
    PdfSource& operator=(const PdfSource&) = delete;
    ~PdfSource() { if (file_ != INVALID_HANDLE_VALUE) CloseHandle(file_); }
    bool Open(const wchar_t* path) {
        file_ = CreateFileW(path, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (file_ == INVALID_HANDLE_VALUE) return false;
        LARGE_INTEGER length{};
        if (!GetFileSizeEx(file_, &length) || length.QuadPart <= 0 || length.QuadPart > (std::numeric_limits<unsigned long>::max)()) return false;
        access.m_FileLen = static_cast<unsigned long>(length.QuadPart);
        access.m_Param = this; access.m_GetBlock = Read;
        return true;
    }
};
