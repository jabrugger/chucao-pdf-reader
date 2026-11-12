# Evaluación inicial de motores — 2026-11-12

## Alcance acordado

Windows, apertura rápida, navegación fluida, búsqueda, selección/copia, contraseñas
e impresión. Interfaz limpia inspirada en MarkPad. Solo lector, también a futuro.
pCloud es un caso de uso de archivos remotos, no una integración requerida.

## Comparación documental (no es un benchmark)

| Opción | Integración | Ventajas para Chucao | Costes y límites |
| --- | --- | --- | --- |
| PDFium | API C, llamada desde C++; DLL x64 | Renderizado, texto, búsqueda, contraseña y salida a impresora Windows; acceso personalizado por bloques; binarios disponibles | No trae visor ni UI; hay que construir navegación, caché y cola de trabajo. Sus llamadas deben serializarse. Actualización del motor y avisos de terceros a nuestro cargo. |
| MuPDF | API C, llamada desde C++ | Motor usado por Sumatra; lectura, renderizado, texto y carga progresiva documentados | No trae la experiencia completa de Sumatra. Revisar elección AGPL/comercial y dependencias antes de adoptarlo. Hace falta una prueba comparable de rendimiento. |
| KillerPdf.Engine | Biblioteca .NET 10 sin UI, paquete NuGet | Se puede usar separada del editor; lectura, renderizado, autenticación y extracción documentados | Integración C++ menos directa. El proyecto referencia ONNX Runtime y PKCS y contiene muchas funciones fuera de alcance. No sabemos aún el tamaño mínimo ni el coste de excluirlas. Paquete declara GPL-3.0-only. |
| Adaptar SumatraPDF | Aplicación C/C++ existente | Navegación e impresión ya resueltas, referencia funcional conocida | Cambiar su UI y mantener un fork puede acoplar el diseño a una aplicación más amplia. Revisión de GPL y componentes requerida. |

## Decisión provisional

Comenzar con C++ + PDFium, usando Win32/GDI para el primer prototipo y sin WebView,
.NET, JavaScript PDF o herramientas de edición. Win32/GDI permite compilar hoy con
el entorno instalado; no determina todavía el sistema visual final.

No afirmamos que PDFium sea más rápido que MuPDF ni que KillerPDF sea lento por
estar en .NET. La decisión minimiza el coste de integración inicial con C++ y permite
probar ya los requisitos de lectura. No se han ejecutado benchmarks entre motores.

Un solo hilo posee todos los objetos PDFium y realiza sus llamadas; la UI recibe
imágenes y resultados. No se hará renderizado simultáneo con llamadas a la misma
biblioteca desde varios hilos. Las tareas viejas se omiten y los resultados obsoletos
no sustituyen el estado visible. Se limita la memoria del bitmap de cada render.

La carga inicial usa la API de archivo de PDFium (no File.ReadAllBytes). Esto evita
imponer una copia completa previa por nuestra aplicación, pero no garantiza descarga
parcial de una unidad virtual. Medir acceso remoto antes de añadir complejidad.

## Validación necesaria para confirmar la elección

Corpus idéntico: texto, imágenes escaneadas, planos, fuentes incrustadas, transparencias,
contraseña, PDF dañado y documentos grandes. Comparar con Sumatra y KillerPDF:
arranque en frío/caliente, primera página, navegación, memoria y calidad visual.
Reportar medianas y percentil 95 con equipo, versión y caché documentados. Para
archivos remotos separar tiempo de I/O del renderizado y registrar caché local.

Conservar MuPDF como alternativa si PDFium no alcanza compatibilidad o fluidez.
Probar KillerPdf.Engine en un ejecutable .NET separado si el comportamiento de sus
documentos lo justifica; no construir un puente C++ antes de tener esa evidencia.

## Fuentes primarias revisadas

- PDFium: https://pdfium.googlesource.com/pdfium/+/refs/heads/main/public/fpdfview.h
- Texto: https://pdfium.googlesource.com/pdfium/+/refs/heads/main/public/fpdf_text.h
- Disponibilidad: https://pdfium.googlesource.com/pdfium/+/refs/heads/main/public/fpdf_dataavail.h
- Distribución de binarios: https://github.com/bblanchon/pdfium-binaries
- MuPDF: https://mupdf.readthedocs.io/en/latest/cookbook/progressive-loading.html
- Licencia MuPDF: https://mupdf.readthedocs.io/en/latest/license.html
- Sumatra: https://www.sumatrapdfreader.org/docs/Contribute-to-SumatraPDF
- KillerPDF: https://github.com/SteveTheKiller/KillerPDF/blob/main/engine/README.md
- Dependencias: https://github.com/SteveTheKiller/KillerPDF/blob/main/engine/KillerPdf.Engine/KillerPdf.Engine.csproj

Las páginas main/latest pueden cambiar; la dependencia del prototipo sí está fijada
en el script de descarga. Revisar las fuentes de nuevo antes de decidir distribución.
