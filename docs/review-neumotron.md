# Contraste de la revisión externa — 2026-11-16

Se contrastaron los dos commits informados por el usuario con el SDK fijado del
proyecto y la documentación primaria. No se fusionaron ramas como parte de esta
revisión. Al comenzar estaba activa la rama `fix/selection-timer-leak`. Los cambios
de interfaz se separaron después en `ui/reader-polish`, basada en `main`, sin
modificar ni fusionar las dos ramas externas.

## `659f467`: nombres del render progresivo

La propuesta reemplaza `FPDF_RenderPage_Continue` y `FPDF_RenderPage_Close` por
`FPDF_RenderPageBitmap_Continue` y `FPDF_RenderPageBitmap_Close`. Esos dos nombres
propuestos no están declarados en `deps/pdfium/include/fpdf_progressive.h` del SDK
que usamos. La interfaz pública sí declara `FPDF_RenderPageBitmap_Start` seguido
de `FPDF_RenderPage_Continue` y `FPDF_RenderPage_Close`. El código actual concuerda
con ella. El cambio propuesto no compilaría contra este SDK.

Referencia: [cabecera pública de PDFium](https://pdfium.googlesource.com/pdfium/+/refs/heads/main/public/fpdf_progressive.h).

## `f0d6083`: temporizador de selección

La propuesta añade `KillTimer(window, 4)` antes de `SetTimer(window, 4, 35, nullptr)`.
La documentación de Windows especifica que la misma pareja ventana/ID reemplaza
y reinicia el temporizador existente. Las llamadas anteriores no apilaban varios
temporizadores con ese ID. El cambio es redundante respecto del problema alegado;
el informe no demuestra una fuga ni aporta una reproducción de autoscroll errático.
No se eliminó el cambio que ya estaba en la rama activa.

Referencia: [SetTimer — Microsoft](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-settimer).
