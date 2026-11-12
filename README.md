# Chucao PDF Reader

Lector de PDF para Windows, rápido y con una interfaz limpia. Solo lectura: no se
prevén edición, anotaciones, OCR ni herramientas para modificar documentos.

## Estado

Prototipo inicial en C++ y Win32, con PDFium. La selección del motor es provisional:
ver [evaluación técnica](docs/engine-evaluation.md). La interfaz actual es una base
funcional; todavía no representa el diseño final inspirado en MarkPad.

## Compilar (Windows x64)

Requiere Visual Studio con las herramientas de C++ y Windows SDK; se admite el
compilador de Visual Studio 2017 instalado en esta PC y versiones posteriores.

```powershell
powershell -ExecutionPolicy Bypass -File scripts/setup-pdfium.ps1
cmd /c scripts\build.cmd
build\Chucao.exe
```

La descarga fija PDFium `chromium/8152` para x64, sin V8, y comprueba su SHA-256.
Los binarios proceden de la distribución comunitaria `bblanchon/pdfium-binaries`.
El motor y sus avisos de terceros se conservan en `deps/pdfium`; no se suben al repo.

## Uso del prototipo

- Abrir: Ctrl+O, arrastrar un PDF o pasar su ruta como argumento.
- Página anterior/siguiente: botones, flechas izquierda/derecha, Page Up/Page Down.
- Zoom: botones +/−, Ctrl+rueda. Ajustar al ancho: Ctrl+0.
- Buscar: Ctrl+F, escribir y pulsar Enter; F3 repite la búsqueda.
- Imprimir: Ctrl+P (diálogo de Windows, documento completo o rango de páginas).
- Los documentos cifrados solicitan contraseña; no se guarda en disco ni en logs.
- Rueda y barra de desplazamiento recorren la página actual.

El motor se ejecuta en un hilo dedicado. Las solicitudes de renderizado antiguas
se descartan; las páginas se cargan bajo demanda. Todavía faltan desplazamiento
continuo entre páginas, selección/copia de texto, índice, pestañas, caché de páginas,
persistencia de posición y pulido visual/accesibilidad. Buscar en escaneos requiere
una capa de texto ya existente. No se implementará OCR.

## Verificación

```powershell
powershell -ExecutionPolicy Bypass -File scripts/smoke-test.ps1
```

Genera un PDF sintético (sin datos personales) y verifica apertura,
renderizado, búsqueda y error de archivo con el mismo motor del visor.
La contraseña está implementada, pero su prueba con PDF cifrado está pendiente.
La impresión física y el rendimiento sobre documentos reales requieren pruebas
manuales; ver [plan de trabajo](docs/roadmap.md).

## Distribución y licencia

Repositorio privado durante el desarrollo. No se ha elegido todavía una licencia
para el código propio. No se incorpora código de SumatraPDF, KillerPDF o MarkPad.
Antes de distribuir: revisar los avisos de PDFium y de sus dependencias y establecer
el proceso de actualización del motor. No hay telemetría ni conexión de red en el visor.
