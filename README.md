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
- Página anterior/siguiente: botones y flechas izquierda/derecha.
- Lectura continua: rueda, barra de desplazamiento, Page Up/Page Down y flechas arriba/abajo.
- Home/End: principio/final del documento.
- Zoom: botones +/−, Ctrl+rueda. Ajustar al ancho: Ctrl+0.
- Buscar: Ctrl+F, escribir y pulsar Enter; F3 repite la búsqueda.
- Imprimir: Ctrl+P (diálogo de Windows, documento completo o rango de páginas).
- Los documentos cifrados solicitan contraseña; no se guarda en disco ni en logs.
- El zoom conserva la página y la posición de lectura; la imagen anterior sirve de
  vista previa mientras llega el render a la nueva escala.

El motor se ejecuta en un hilo dedicado. Los renderizados obsoletos se cancelan en
los puntos de pausa que ofrece PDFium; las páginas visibles tienen prioridad sobre
la precarga de vecinas. La caché de imágenes está limitada a 96 MiB y cada render a
12 millones de píxeles; estos límites no incluyen la memoria interna del motor.
Las dimensiones de las páginas lejanas se estiman hasta visitarlas, para evitar
recorrer el PDF completo al abrir. Todavía faltan selección/copia de texto, índice,
pestañas, persistencia de posición y pulido visual/accesibilidad. Buscar en escaneos requiere
una capa de texto ya existente. No se implementará OCR.

PDFium se carga e inicializa al abrir el primer documento; abrir la ventana vacía
no inicializa el motor. Un render inmediato cancela el render de tamaño pendiente.

## Diagnóstico de velocidad

```powershell
# Solo procesa el documento localmente: no lo modifica ni lo sube a ningún servicio.
build\pdf-smoke.exe "P:\ruta\documento.pdf" --benchmark
powershell -ExecutionPolicy Bypass -File scripts/benchmark-open.ps1 -Pdf "P:\ruta\documento.pdf"
# Prueba oculta de navegación, caché, zoom y reapertura con el ejecutable real:
powershell -ExecutionPolicy Bypass -File scripts/benchmark-open.ps1 -Pdf "P:\ruta\documento.pdf" -Runs 1 -Navigation
build\layout-test.exe
```

El primer comando mide inicialización, apertura, carga y dibujo de la primera página
a 800 × 1000. El segundo recorre el ejecutable real con una ventana oculta y termina
cuando la UI recibe el primer bitmap; no mide la presentación física en pantalla.
Sus informes quedan en `test-results`, excluido de Git. No se vacían cachés del
sistema o pCloud. La primera ejecución observada no equivale a una caché vacía.

Los modos `--buffered` y `--buffered-64k` del diagnóstico permiten comparar una
caché experimental; no están activados en el visor. El experimento de KillerPDF
en `tools/killer-benchmark` usa la biblioteca de una instalación existente, sin
incorporarla a Chucao. Ver [mediciones](docs/performance.md).

## Verificación

`-Navigation` mide toda la secuencia de prueba, no solo la apertura.

```powershell
powershell -ExecutionPolicy Bypass -File scripts/smoke-test.ps1
# Geometría y navegación con páginas de tamaños distintos:
powershell -ExecutionPolicy Bypass -File scripts/test-reader.ps1
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
