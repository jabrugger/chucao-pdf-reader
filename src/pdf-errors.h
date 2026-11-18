#pragma once
#include <fpdfview.h>
#include <string>

// User-facing text for engine failures. Keep messages short, factual and
// actionable: the status bar has a single line and never shows engine details.
inline std::wstring PdfOpenErrorMessage(unsigned long error, bool suppliedPassword) {
    switch (error) {
    case FPDF_ERR_PASSWORD:
        return suppliedPassword ? L"La contraseña es incorrecta. Volvé a intentarlo."
                                : L"Este documento está protegido. Ingresá la contraseña.";
    case FPDF_ERR_FILE:
        return L"No se pudo abrir el archivo. Verificá que exista y que tengas permiso para leerlo.";
    case FPDF_ERR_FORMAT:
        return L"El archivo no es un PDF válido o está dañado.";
    case FPDF_ERR_SECURITY:
        return L"El PDF usa una protección que este lector no admite.";
    case FPDF_ERR_PAGE:
        return L"El PDF no tiene páginas válidas.";
    default:
        return L"No se pudo abrir el PDF. Puede estar bloqueado por otra aplicación o no ser compatible.";
    }
}

enum class PageFailure { Read, Dimensions, Render, Memory };

inline std::wstring PageFailureMessage(int page, PageFailure failure) {
    auto label = L"la página " + std::to_wstring(page + 1);
    switch (failure) {
    case PageFailure::Read:
        return L"No se pudo leer " + label + L". El PDF puede estar incompleto.";
    case PageFailure::Dimensions:
        return L"La página " + std::to_wstring(page + 1) + L" tiene dimensiones no válidas.";
    case PageFailure::Render:
        return L"No se pudo dibujar " + label + L".";
    case PageFailure::Memory:
        return L"No hay memoria suficiente para dibujar " + label + L".";
    }
    return L"No se pudo mostrar " + label + L".";
}
