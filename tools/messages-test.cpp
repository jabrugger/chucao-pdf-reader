#include "../src/pdf-errors.h"
#include <cstdio>
#include <stdexcept>

void Check(bool ok, const char* message) { if (!ok) throw std::runtime_error(message); }
bool Has(const std::wstring& text, const wchar_t* part) { return text.find(part) != std::wstring::npos; }

int wmain() {
    try {
        auto prompt = PdfOpenErrorMessage(FPDF_ERR_PASSWORD, false);
        auto retry = PdfOpenErrorMessage(FPDF_ERR_PASSWORD, true);
        Check(Has(prompt, L"contraseña") && Has(prompt, L"protegido"), "first password prompt");
        Check(Has(retry, L"incorrecta") && retry != prompt, "wrong password differs from first prompt");
        Check(Has(PdfOpenErrorMessage(FPDF_ERR_FILE, false), L"permiso"), "missing file mentions permissions");
        Check(Has(PdfOpenErrorMessage(FPDF_ERR_FORMAT, false), L"PDF válido"), "damaged file message");
        Check(Has(PdfOpenErrorMessage(FPDF_ERR_SECURITY, false), L"protección"), "unsupported protection message");
        Check(Has(PdfOpenErrorMessage(FPDF_ERR_PAGE, false), L"páginas válidas"), "empty document message");
        auto unknown = PdfOpenErrorMessage(FPDF_ERR_UNKNOWN, false);
        Check(Has(unknown, L"bloqueado") && Has(unknown, L"compatible"), "unknown error stays actionable");
        Check(unknown != PdfOpenErrorMessage(FPDF_ERR_FORMAT, false) &&
            unknown != PdfOpenErrorMessage(FPDF_ERR_FILE, false), "error codes stay distinguishable");
        for (auto failure : {PageFailure::Read, PageFailure::Dimensions, PageFailure::Render, PageFailure::Memory}) {
            auto text = PageFailureMessage(4, failure);
            Check(Has(text, L"página 5") || Has(text, L"dibujar la página 5"), "page message uses one-based number");
            Check(text.size() > 12 && text.back() == L'.', "page message is a complete sentence");
        }
        Check(PageFailureMessage(0, PageFailure::Memory) != PageFailureMessage(0, PageFailure::Render), "page failures differ");
        Check(Has(PageFailureMessage(0, PageFailure::Memory), L"memoria"), "memory failure mentions memory");
        std::puts("messages: open/error codes, password retry, unknown errors and page context OK");
        return 0;
    } catch (const std::exception& error) {
        std::fprintf(stderr, "%s\n", error.what()); return 1;
    }
}
