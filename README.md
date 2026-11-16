# Chucao PDF Reader

<img src="assets/branding/chucao-icon-256.png" width="128" alt="Chucao con anteojos y un documento">

Lector de PDF para Windows, rápido y con una interfaz limpia. Solo lectura: no se
prevén edición, anotaciones, OCR ni herramientas para modificar documentos.

## Estado

Prototipo inicial en C++ y Win32, con PDFium. La selección del motor es provisional:
ver [evaluación técnica](docs/engine-evaluation.md). La interfaz actual es una base
funcional; todavía no representa el diseño final inspirado en MarkPad.

## Compilar (Windows x64)

Requiere Visual Studio con las herramientas de C++ x64 y Windows SDK (carga
«Desarrollo para el escritorio con C++»). El script detecta la instalación con
`vswhere`, admite un Developer Command Prompt x64 y conserva el respaldo para
Visual Studio 2017 de la PC original.

```powershell
powershell -ExecutionPolicy Bypass -File scripts/setup-pdfium.ps1
cmd /c scripts\build.cmd
build\Chucao.exe
```

La descarga fija PDFium `chromium/8152` para x64, sin V8, y comprueba su SHA-256.
Los binarios proceden de la distribución comunitaria `bblanchon/pdfium-binaries`.
El motor y sus avisos de terceros se conservan en `deps/pdfium`; no se suben al repo.

## Uso del prototipo

El lector recuerda la página, el zoom y la posición al cerrar o cambiar de PDF.
El estado es local a cada PC y se guarda en
`%LOCALAPPDATA%\ChucaoPDFReader\reading-state.dat`, sin contraseñas ni texto del
documento. Ver [alcance y pruebas](docs/reading-state.md).

- Enlaces internos: clic para ir al destino; Alt+Izquierda vuelve al punto anterior.
  El cursor cambia a una mano cuando se reconoce un enlace interno.
- Enlaces web HTTP/HTTPS: clic para abrirlos en el navegador predeterminado.
  Pasar el mouse, seleccionar texto o abrir el PDF no abre el navegador.

- Abrir: Ctrl+O, arrastrar un PDF o pasar su ruta como argumento.
- Recientes: menú **Abrir ▾** o Ctrl+Shift+O; muestra hasta diez documentos.
  **Olvidar historial y posiciones…** elimina ambos del perfil local, previa
  confirmación. No modifica los PDF. Ver [alcance y pruebas](docs/recent-files.md).
- Página anterior/siguiente: botones y flechas izquierda/derecha.
- Lectura continua: rueda, barra de desplazamiento, Page Up/Page Down y flechas arriba/abajo.
- Home/End: principio/final del documento.
- Zoom: botones +/−, Ctrl+rueda. Ajustar al ancho: Ctrl+0.
- Desplazamiento horizontal: barra inferior, Shift+rueda o rueda horizontal.
  La búsqueda lleva también a las coincidencias fuera del ancho visible.
- Índice integrado a la izquierda: se muestra al abrir si el PDF tiene marcadores.
  El botón del índice o Ctrl+B lo pliega/despliega. Acercar el puntero al borde
  izquierdo permite verlo temporalmente; la chincheta permite fijarlo abierto.
  Un clic o Enter lleva al destino; las flechas expanden los capítulos.
  Alt+Izquierda vuelve al punto anterior.
- Buscar: Ctrl+F o el icono de lupa muestra la barra inferior. Busca al escribir;
  Enter/F3 avanza y Mayús+Enter/Mayús+F3 retrocede. Escape o × oculta la barra,
  conserva la consulta y devuelve el espacio al documento.
  La coincidencia aparece resaltada y la vista se desplaza hasta ella.
- Seleccionar: arrastrar sobre el texto, incluso entre páginas. Ctrl+C copia;
  Escape limpia la selección. Al arrastrar cerca de un borde la vista se desplaza.
- Imprimir: Ctrl+P (diálogo de Windows, documento completo o rango de páginas).
- Los documentos cifrados solicitan contraseña y avisan si es incorrecta; no se
  guarda en disco ni en logs. Los permisos de copia e impresión se comprueban al abrir.
- El zoom conserva la página y la posición de lectura; la imagen anterior sirve de
  vista previa mientras llega el render a la nueva escala.

El motor se ejecuta en un hilo dedicado. Los renderizados obsoletos se cancelan en
los puntos de pausa que ofrece PDFium; las páginas visibles tienen prioridad sobre
la precarga de vecinas. La caché de imágenes está limitada a 96 MiB y cada render a
12 millones de píxeles; estos límites no incluyen la memoria interna del motor.
Las dimensiones de las páginas lejanas se estiman hasta visitarlas, para evitar
recorrer el PDF completo al abrir. La geometría del texto se obtiene bajo demanda
al seleccionar, con caché acotada. La copia respeta los permisos del PDF y usa
Unicode en el portapapeles. El índice del PDF se carga al pedirlo y permite saltar
a capítulos y subcapítulos. Todavía faltan pestañas
y pulido visual/accesibilidad. Buscar o seleccionar en escaneos requiere
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

Las pruebas incluyen enlaces directos y nombrados, posiciones XYZ, rotación y
recorte, y regreso desde un enlace con zoom alto y desplazamiento horizontal.
Los enlaces HTTP/HTTPS solo se envían al navegador por un clic explícito.
Las acciones de scripts/archivos y otros protocolos permanecen inactivos.

`-Navigation` y `-Text` miden toda la secuencia de prueba, no solo la apertura.
Las pruebas de texto no modifican el portapapeles: verifican internamente el
resultado y generan imágenes locales del visor para inspección visual.

```powershell
powershell -ExecutionPolicy Bypass -File scripts/smoke-test.ps1
# Geometría y navegación con páginas de tamaños distintos:
powershell -ExecutionPolicy Bypass -File scripts/test-reader.ps1
```

Genera un PDF sintético (sin datos personales) y verifica apertura,
renderizado, búsqueda y error de archivo con el mismo motor del visor.
Las pruebas de contraseñas incluyen archivos cifrados AES-256, AES-128 y RC4,
Unicode, reintentos, cancelación y permisos; ver [passwords.md](docs/passwords.md).
El diálogo visible todavía requiere comprobación manual de foco y teclado.
La impresión física y el rendimiento sobre documentos reales requieren pruebas
manuales; ver [plan de trabajo](docs/roadmap.md).

## Distribución y licencia

Repositorio privado durante el desarrollo. No se ha elegido todavía una licencia
para el código propio. No se incorpora código de SumatraPDF, KillerPDF o MarkPad.
Antes de distribuir: revisar los avisos de PDFium y de sus dependencias y establecer
el proceso de actualización del motor. No hay telemetría ni conexión de red en el visor;
el navegador puede conectarse al sitio elegido al hacer clic en un enlace web.

La imagen elegida por el usuario está en [assets/branding](assets/branding/README.md).
El icono y la bienvenida se incluyen en el EXE; no requieren imágenes externas al ejecutarlo.
