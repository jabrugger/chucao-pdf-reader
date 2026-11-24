# Chucao PDF Reader

<img src="assets/branding/chucao-icon-256.png" width="128" alt="Chucao con anteojos y un documento">

Lector de PDF para Windows, rápido y con una interfaz limpia. Solo lectura: no se
prevén edición, anotaciones, OCR ni herramientas para modificar documentos.

## Estado

Versión 0.2.x en desarrollo, en C++ y Win32, con PDFium. La selección del motor es provisional:
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

Como alternativa, `CMakeLists.txt` compila los mismos binarios y pruebas con el
CMake incluido en Visual Studio:

```powershell
cmake -S . -B build\cmake-check -G "Visual Studio 18 2026" -A x64
cmake --build build\cmake-check --config Release
ctest --test-dir build\cmake-check -C Release
```

Elegir el generador que corresponda a la versión instalada (`Visual Studio 18
2026`, `Visual Studio 17 2022`, etc.). La carpeta `build\cmake-check` queda dentro
de `build/`, excluida de Git.

La descarga fija PDFium `chromium/8152` para x64, sin V8, y comprueba su SHA-256.
Los binarios proceden de la distribución comunitaria `bblanchon/pdfium-binaries`.
El motor y sus avisos de terceros se conservan en `deps/pdfium`; no se suben al repo.

## Paquete portable

```powershell
powershell -ExecutionPolicy Bypass -File scripts/package.ps1
powershell -ExecutionPolicy Bypass -File scripts/test-package.ps1
```

El primero compila y genera `dist/Chucao-0.2.9-win-x64.zip` con el ejecutable,
`pdfium.dll`, el LEEME, la licencia MIT y los avisos de PDFium y sus
dependencias. El segundo descomprime el paquete, comprueba los archivos
obligatorios y ejecuta el lector empaquetado contra un PDF sintético.

Para generar el instalador (Inno Setup 7, instalación por usuario y asociación de
PDF opcional):

```powershell
powershell -ExecutionPolicy Bypass -File scripts/package-installer.ps1
```

Produce `dist/Chucao-0.2.9-setup.exe`. El instalador y el EXE **no están firmados**:
Windows SmartScreen puede mostrar "Windows protegió su PC"; elegí "Más información" y
"Ejecutar de todas formas" si descargaste Chucao desde este repositorio. Ver [distribución](docs/distribution.md) e
[instalador](docs/installer.md).

## Uso del prototipo

El icono de información de la esquina superior derecha o F1 abre **Acerca de**, con versión,
autor, contacto, fecha de release y un enlace al repositorio de GitHub.

El lector recuerda la página, el zoom y la posición al cerrar o cambiar de PDF.
El estado es local a cada PC y se guarda en
`%LOCALAPPDATA%\ChucaoPDFReader\reading-state.dat`, sin contraseñas ni texto del
documento. Ver [alcance y pruebas](docs/reading-state.md). También recuerda el
tamaño, la posición y si la ventana estaba maximizada; ver
[opciones](docs/preferences.md).

- Enlaces internos: clic para ir al destino; Alt+Izquierda vuelve al punto anterior.
  El cursor cambia a una mano cuando se reconoce un enlace interno.
- Enlaces web HTTP/HTTPS: clic para abrirlos en el navegador predeterminado.
  Pasar el mouse, seleccionar texto o abrir el PDF no abre el navegador.

- Abrir: el icono de carpeta, Ctrl+O, arrastrar un PDF o pasar su ruta como argumento.
  El botón abre directamente el selector, sin menú intermedio.
- Recientes: en la pantalla de bienvenida aparece una lista con hasta diez PDF
  abiertos antes; clic o Enter abre el elegido. Se leen bajo demanda, sin recorrer
  carpetas. **Olvidar recientes y posiciones** está en Opciones y no modifica los
  PDF. Ver [alcance y pruebas](docs/recent-files.md).
- Página anterior/siguiente: botones y flechas izquierda/derecha.
- Lectura continua: rueda, barra de desplazamiento, Page Up/Page Down y flechas arriba/abajo.
- Home/End: principio/final del documento.
- Zoom: botones +/−, Ctrl+rueda. Ajustar al ancho con Ctrl+0, a la página con
  Ctrl+1 y al alto con Ctrl+2. El indicador muestra Ancho, Página o Alto.
- Desplazamiento horizontal: barra inferior, Shift+rueda o rueda horizontal.
  La búsqueda lleva también a las coincidencias fuera del ancho visible.
- Índice integrado a la izquierda: se muestra al abrir si el PDF tiene marcadores
  (configurable en Opciones). El botón del índice o Ctrl+B lo pliega/despliega. Acercar el puntero al borde
  izquierdo permite verlo temporalmente; la chincheta permite fijarlo abierto.
  El cuadro superior filtra por título sin distinguir mayúsculas ni acentos; Enter
  va a la primera coincidencia y Escape limpia el filtro. Un clic o Enter lleva al
  destino; las flechas expanden los capítulos. Alt+Izquierda vuelve al punto anterior.
- Buscar: Ctrl+F o el icono de lupa muestra la barra inferior. Busca al escribir;
  Enter/F3 avanza y Mayús+Enter/Mayús+F3 retrocede. Escape o × oculta la barra,
  conserva la consulta y devuelve el espacio al documento.
  La coincidencia aparece resaltada y la vista se desplaza hasta ella.
- Seleccionar: arrastrar sobre el texto, incluso entre páginas. Ctrl+C copia;
  Escape limpia la selección. Al arrastrar cerca de un borde la vista se desplaza.
- Imprimir: Ctrl+P (diálogo de Windows, documento completo o rango de páginas).
- Tema: el botón de tema abre **Opciones** (claro, oscuro o igual que el sistema);
  Ctrl+D alterna claro/oscuro directamente. La elección se recuerda en
  `%LOCALAPPDATA%\ChucaoPDFReader\settings.dat`; ver [temas](docs/themes.md).
  Las páginas del PDF se mantienen blancas.
- La interfaz se adapta al DPI de cada monitor, con fuentes e iconos vectoriales
  suavizados; ver [DPI por monitor](docs/dpi.md).
- Teclado: Tab recorre la barra, F6 alterna documento/índice/barra, Espacio y
  Mayús+Espacio desplazan una pantalla, `+`/`-` ajustan el zoom y Escape devuelve
  el foco al documento. La barra de estado es un control accesible que anuncia sus
  cambios a lectores de pantalla. Ver [accesibilidad](docs/accessibility.md).
- Pestañas: varios documentos abiertos a la vez. Ctrl+T abre, Ctrl+W cierra,
  Ctrl+Tab y Ctrl+Mayús+Tab alternan; el clic medio cierra una pestaña. Cada una
  recuerda su posición. Mientras un PDF grande o remoto se abre, la pestaña muestra
  un indicador central animado con el nombre del archivo. Ver
  [pestañas](docs/tabs.md).
- Opciones: el icono de tema abre un diálogo con el tema (claro, oscuro o igual que
  el sistema), mostrar automáticamente el índice, reabrir los PDF de la última
  sesión (desactivado por defecto), **Buscar actualizaciones al abrir**
  (desactivado por defecto), **Abrir los PDF con Chucao** y un enlace a la
  configuración de aplicaciones predeterminadas de Windows. Ver
  [opciones](docs/preferences.md) y [actualizaciones](docs/updater.md).
- El índice sombrea suavemente la sección que se está leyendo (por página; un
  capítulo plegado sombrea su padre visible) y la muestra sin tocar la selección.
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
a capítulos y subcapítulos. Todavía falta pulido visual y los anuncios de estado
para lectores de pantalla. Buscar o seleccionar en escaneos requiere
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
`-Startup` separa ventana, apertura, render, recepción del bitmap y composición
del primer cuadro; no incluye la presentación física en pantalla.

```powershell
powershell -ExecutionPolicy Bypass -File scripts/smoke-test.ps1
# Geometría y navegación con páginas de tamaños distintos:
powershell -ExecutionPolicy Bypass -File scripts/test-reader.ps1
```

Genera un PDF sintético (sin datos personales) y verifica apertura,
renderizado, búsqueda y error de archivo con el mismo motor del visor.
La prueba de robustez genera PDF truncados, basura, vacíos, con página faltante y
páginas de dimensiones extremas, y comprueba el presupuesto de renderizado; ver
[robustez](docs/robustness.md).
Las pruebas de contraseñas incluyen archivos cifrados AES-256, AES-128 y RC4,
Unicode, reintentos, cancelación y permisos; ver [passwords.md](docs/passwords.md).
El diálogo visible todavía requiere comprobación manual de foco y teclado.
La impresión física y el rendimiento sobre documentos reales requieren pruebas
manuales; ver [plan de trabajo](docs/roadmap.md).

## Distribución y licencia

El código propio se distribuye con la licencia [MIT](LICENSE). No se incorpora
código de SumatraPDF, KillerPDF o MarkPad. PDFium y sus dependencias conservan sus
licencias: ver [avisos de terceros](THIRD-PARTY-NOTICES.md). El proceso de
actualización del motor está en [pdfium-update.md](docs/pdfium-update.md). El
instalador preparado no está firmado y debe validarse en otra PC; ver
[instalador](docs/installer.md). No hay telemetría ni conexión de red en el visor;
con la opción **Buscar actualizaciones al abrir** activada, se consulta una sola vez
la última release en GitHub para avisar si hay una versión nueva, sin descargar ni
instalar nada; el navegador puede conectarse al sitio elegido al hacer clic en un
enlace web.

La imagen elegida por el usuario está en [assets/branding](assets/branding/README.md).
El icono y la bienvenida se incluyen en el EXE; no requieren imágenes externas al ejecutarlo.

## Contribuir

Ver [CONTRIBUTING.md](CONTRIBUTING.md) para compilar, probar y orientarte en el código,
y [SECURITY.md](SECURITY.md) para reportar vulnerabilidades.
