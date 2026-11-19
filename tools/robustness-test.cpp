#include <windows.h>
#include <fpdfview.h>
#include "../src/render-budget.h"
#include "../src/pdf-errors.h"
#include <cmath>
#include <cstdio>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

void Check(bool ok, const char* message) { if (!ok) throw std::runtime_error(message); }

std::string Utf8(const std::wstring& path) {
    int size = WideCharToMultiByte(CP_UTF8, 0, path.data(), static_cast<int>(path.size()), nullptr, 0, nullptr, nullptr);
    std::string result(size, 0);
    WideCharToMultiByte(CP_UTF8, 0, path.data(), static_cast<int>(path.size()), &result[0], size, nullptr, nullptr);
    return result;
}

void Write(const std::wstring& path, const std::string& data) {
    HANDLE file = CreateFileW(path.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    Check(file != INVALID_HANDLE_VALUE, "create fixture");
    DWORD written = 0;
    bool ok = WriteFile(file, data.data(), static_cast<DWORD>(data.size()), &written, nullptr) != 0;
    CloseHandle(file);
    Check(ok && written == data.size(), "write fixture");
}

std::string BuildPdf(const std::string& mediaBox, const std::string& content, const std::string& kids = "[3 0 R]") {
    std::vector<std::string> objects = {
        "<< /Type /Catalog /Pages 2 0 R >>",
        "<< /Type /Pages /Kids " + kids + " /Count 1 >>",
        "<< /Type /Page /Parent 2 0 R /MediaBox " + mediaBox +
            " /Resources << /Font << /F1 4 0 R >> >> /Contents 5 0 R >>",
        "<< /Type /Font /Subtype /Type1 /BaseFont /Helvetica >>",
        "<< /Length " + std::to_string(content.size()) + " >>\nstream\n" + content + "\nendstream"
    };
    std::string pdf = "%PDF-1.4\n";
    std::vector<size_t> offsets;
    for (size_t i = 0; i < objects.size(); ++i) {
        offsets.push_back(pdf.size());
        pdf += std::to_string(i + 1) + " 0 obj\n" + objects[i] + "\nendobj\n";
    }
    size_t xref = pdf.size();
    pdf += "xref\n0 " + std::to_string(objects.size() + 1) + "\n0000000000 65535 f \n";
    for (size_t offset : offsets) {
        std::ostringstream entry; entry << std::setw(10) << std::setfill('0') << offset << " 00000 n \n";
        pdf += entry.str();
    }
    pdf += "trailer\n<< /Size " + std::to_string(objects.size() + 1) + " /Root 1 0 R >>\nstartxref\n" +
        std::to_string(xref) + "\n%%EOF\n";
    return pdf;
}

// A damaged document must never crash the process; a null handle is expected for
// clearly invalid input. Every engine error must have a user-facing message.
void CheckDamaged(const std::wstring& path, bool mustFail) {
    auto doc = FPDF_LoadDocument(Utf8(path).c_str(), nullptr);
    if (!doc) {
        auto error = FPDF_GetLastError();
        Check(!PdfOpenErrorMessage(error, false).empty(), "damaged document has a message");
        return;
    }
    int pages = FPDF_GetPageCount(doc);
    Check(pages >= 0, "damaged document page count stays sane");
    if (mustFail) Check(pages == 0, "truncated/garbage document exposes no pages");
    FPDF_CloseDocument(doc);
}

void CheckRenderable(const std::wstring& path, bool requireBudget) {
    auto doc = FPDF_LoadDocument(Utf8(path).c_str(), nullptr);
    Check(doc != nullptr, "extreme fixture opens");
    Check(FPDF_GetPageCount(doc) == 1, "extreme fixture has one page");
    auto page = FPDF_LoadPage(doc, 0);
    Check(page != nullptr, "extreme page loads");
    double width = FPDF_GetPageWidth(page), height = FPDF_GetPageHeight(page);
    Check(std::isfinite(width) && std::isfinite(height), "extreme page dimensions are finite");
    auto target = LimitRenderSize(width, height, 1.0);
    if (requireBudget) {
        Check(target.width > 0 && target.height > 0, "extreme page has a render target");
        Check(target.width <= 16000 && target.height <= 16000, "render dimensions stay bounded");
        Check(static_cast<long long>(target.width) * target.height <= 12000000 + 16000, "render pixels stay bounded");
    }
    if (target.width > 0 && target.height > 0) {
        auto bitmap = FPDFBitmap_Create(target.width, target.height, 0);
        Check(bitmap != nullptr, "extreme bitmap allocates");
        FPDFBitmap_FillRect(bitmap, 0, 0, target.width, target.height, 0xffffffff);
        FPDF_RenderPageBitmap(bitmap, page, 0, 0, target.width, target.height, 0, 0);
        FPDFBitmap_Destroy(bitmap);
    }
    FPDF_ClosePage(page);
    FPDF_CloseDocument(doc);
}

int wmain(int argc, wchar_t** argv) {
    if (argc != 2) return 2;
    try {
        std::wstring folder = argv[1];
        CreateDirectoryW(folder.c_str(), nullptr);
        auto file = [&](const wchar_t* name) { return folder + L"\\" + name; };
        const std::string content = "BT /F1 24 Tf 72 700 Td (Chucao robustness) Tj ET";
        auto valid = BuildPdf("[0 0 612 792]", content);
        Write(file(L"valid.pdf"), valid);
        Write(file(L"truncated.pdf"), valid.substr(0, valid.size() / 2));
        Write(file(L"garbage.pdf"), std::string("this is not a PDF\n") + std::string(4096, 'x'));
        Write(file(L"empty.pdf"), {});
        Write(file(L"missing-page.pdf"), BuildPdf("[0 0 612 792]", content, "[9 0 R]"));
        Write(file(L"huge.pdf"), BuildPdf("[0 0 5000 5000]", "BT /F1 100 Tf 100 4500 Td (Huge) Tj ET"));
        Write(file(L"tiny.pdf"), BuildPdf("[0 0 1 1]", "BT /F1 1 Tf 0 0 Td (x) Tj ET"));
        Write(file(L"zero-box.pdf"), BuildPdf("[0 0 0 0]", content));

        FPDF_LIBRARY_CONFIG config{}; config.version = 2;
        FPDF_InitLibraryWithConfig(&config);
        try {
            CheckDamaged(file(L"truncated.pdf"), true);
            CheckDamaged(file(L"garbage.pdf"), true);
            CheckDamaged(file(L"empty.pdf"), true);
            CheckDamaged(file(L"missing-page.pdf"), false);
            CheckRenderable(file(L"valid.pdf"), true);
            CheckRenderable(file(L"huge.pdf"), true);
            CheckRenderable(file(L"tiny.pdf"), true);
            // A zero MediaBox may be rejected by PDFium; if it loads, no target is produced.
            auto zero = FPDF_LoadDocument(Utf8(file(L"zero-box.pdf")).c_str(), nullptr);
            if (zero) {
                auto page = FPDF_LoadPage(zero, 0);
                if (page) {
                    double width = FPDF_GetPageWidth(page), height = FPDF_GetPageHeight(page);
                    auto target = LimitRenderSize(width, height, 1);
                    Check((width <= 0 || height <= 0) ? target.width == 0 :
                        (target.width > 0 && target.width <= 16000 && target.height <= 16000),
                        "zero page stays bounded or rejects the target");
                    FPDF_ClosePage(page);
                }
                FPDF_CloseDocument(zero);
            } else {
                Check(!PdfOpenErrorMessage(FPDF_GetLastError(), false).empty(), "zero page error has a message");
            }

            auto bounded = LimitRenderSize(20000, 20000, 1);
            Check(bounded.width > 0 && bounded.height > 0 && bounded.width <= 16000 && bounded.height <= 16000,
                "synthetic huge page keeps dimension bounds");
            Check(static_cast<long long>(bounded.width) * bounded.height <= 12000000 + 16000,
                "synthetic huge page keeps pixel bounds");
            auto longPage = LimitRenderSize(100000, 100, 1);
            Check(longPage.width <= 16000 && longPage.height >= 1 &&
                static_cast<long long>(longPage.width) * longPage.height <= 12000000 + 16000, "long page is bounded");
            auto zoomed = LimitRenderSize(612, 792, 100);
            Check(zoomed.width > 0 && zoomed.width <= 16000 && zoomed.height <= 16000, "high zoom is bounded");
            auto tiny = LimitRenderSize(1, 1, 0.001);
            Check(tiny.width >= 1 && tiny.height >= 1, "tiny page stays at least one pixel");
            Check(LimitRenderSize(0, 100, 1).width == 0, "zero width rejected");
            Check(LimitRenderSize(100, -1, 1).width == 0, "negative height rejected");
            Check(LimitRenderSize(std::nan(""), 100, 1).width == 0, "NaN rejected");
            Check(LimitRenderSize(100, 100, 0).width == 0, "zero scale rejected");
            Check(LimitRenderSize(1e308, 1e308, 1e308).width == 0, "overflow rejected");
            FPDF_DestroyLibrary();
            std::puts("robustness: damaged PDFs, extreme dimensions, render budget and error messages OK");
            return 0;
        } catch (...) {
            FPDF_DestroyLibrary();
            throw;
        }
    } catch (const std::exception& error) {
        std::fprintf(stderr, "%s\n", error.what());
        return 1;
    }
}
