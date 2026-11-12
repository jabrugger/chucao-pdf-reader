# Investigación de apertura — 2026-11-12

## Método y límites

Se usó el libro de PHP señalado por el usuario y tres PDF elegidos al azar dentro
de su biblioteca local montada en P:. Los archivos no se modificaron, copiaron al
repo ni transmitieron. No se conserva aquí su ruta completa. La primera lectura
observada no garantiza caché vacía: pCloud y Windows controlan sus propias cachés.

`pdf-smoke --benchmark` mide el motor en un proceso nuevo, a 800 × 1000, incluyendo
inicialización pero excluyendo el cargador del proceso, la ventana y su pintura.
Se añadieron rutas Unicode, fases separadas y un modo que no requiere encontrar el
texto sintético `Chucao` en documentos arbitrarios.

`benchmark-open.ps1` arranca Chucao oculto, usa la misma cola y recepción de bitmap
que el lector y cierra después del primer resultado. `ready_ms` empieza en wWinMain,
excluye el cargador de DLL y NO mide píxeles visibles en pantalla. `process_ms` incluye
Start-Process, arranque del proceso, informe y cierre: no confundirlo con first paint.
La resolución del visor depende del ancho y proporciones de la página y no es la
misma que la prueba de motor. No se puede comparar sus números directamente.

## Muestra de lectura (PDFium, sin caché propia)

| Documento | Tamaño aproximado | Páginas | Primera observación, hasta bitmap | Repetición, hasta bitmap |
| --- | --- | --- | --- | --- |
| Windows Internals, Part 1 | 61,1 MB | 801 | 2967 ms | 34 ms |
| Principles of Computer Architecture | 2,9 MB | 654 | 1831 ms | 94 ms |
| Python and CPP for Beginners | 46,6 MB | 78 | 1917 ms | 126 ms |

La mayor parte de la primera observación transcurrió en abrir el documento y cargar
los recursos de la página. Renderizarla tomó aproximadamente 7–43 ms. Los datos
muestran una fuerte dependencia del estado de caché/I/O, pero no separan por sí solos
el coste del proveedor de archivos del análisis de PDFium. No atribuir toda la demora
a la red sin traza de I/O. El libro de PHP, que el usuario confirmó que había abierto,
dio aproximadamente 72–95 ms de motor en distintas ejecuciones.

## Experimento de caché

Se probó lectura personalizada con bloques de 64 KiB y 4 KiB. En PHP el modo de
64 KiB dio primero 918 ms y luego 22 ms; el modo nativo había dado unos 73 ms. Con
4 KiB se observaron resultados peores que el nativo en los tres libros muestreados.
Los ensayos cambian la caché, por lo que no son comparaciones en frío controladas.
Decisión: mantener la lectura nativa en Chucao; no activar read-ahead por intuición.
El código experimental está aislado en tools, con memoria acotada.

## Arranque del visor

Las primeras cinco ejecuciones instrumentadas del libro de PHP dieron 337–880 ms
desde wWinMain hasta recepción del primer bitmap. No son cifras del commit original:
ya incluían la cancelación de render redundante y la inicialización bajo demanda.

Se comparó carga normal de DLL (`Chucao-before.exe`, artefacto local excluido) con
carga diferida, alternando cinco procesos de cada uno con el mismo archivo. Medianas
de `ready_ms`: 214 ms y 285 ms respectivamente. Hubo alta variación (165–422 ms y
194–349 ms). No demuestra una mejora de latencia; NO anunciar aceleración porcentual.
La carga diferida tiene por ahora el objetivo estructural de no cargar/inicializar
PDFium cuando solo se necesita la ventana vacía. Hace falta una medición controlada
del arranque vacío y de la primera pintura para decidir si conservarla a largo plazo.

## Motor de KillerPDF instalado

Se creó un diagnóstico .NET contra `KillerPdf.Engine.dll` instalado en esta PC. No
se enlaza ni distribuye con Chucao. En el libro de PHP dio 2321 ms en la primera
ejecución y 313 ms en la siguiente, a 800 × 1000. Incluye apertura del stream y
procesamiento/renderizado, pero excluye arranque de .NET anterior al programa y toda
la aplicación KillerPDF. No mide su visor, caché de aplicación ni primera pintura.
Su primera prueba puede haber leído datos que PDFium no necesitaba. No inferir que
KillerPDF es más lento ni que el lenguaje explica el resultado.

## Cambios en el lector y próximos pasos

- Inicialización del worker/PDFium al abrir un documento, no al crear ventana vacía.
- Carga diferida de la DLL para que sus importaciones no precedan a la ventana vacía.
- Cancelación del timer de resize cuando ya hay un render inmediato: evita un segundo
  render de la primera página por el evento inicial de tamaño.
- La lectura de archivos no incorpora la caché experimental que dio resultados mixtos.
- Pendiente: primera pintura visible, perfil del arranque con trazas del sistema,
  comparación de aplicaciones completas y carga progresiva/cancelación del motor.

Esta investigación todavía no confirma que Chucao sea más rápido que KillerPDF.
