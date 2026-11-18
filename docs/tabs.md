# Pestañas — 2026-11-18

Chucao puede tener varios documentos abiertos a la vez, cada uno en una pestaña.
La franja de pestañas aparece debajo de la barra de herramientas cuando hay al
menos un documento. Cada pestaña muestra el nombre del archivo y un aspa para
cerrarla; la pestaña activa se distingue por su fondo, un subrayado de acento y
el texto en color principal.

## Abrir, alternar y cerrar

- Abrir un archivo (Ctrl+O, recientes, arrastrar o pasar la ruta como argumento)
  agrega una pestaña. Si ya está abierto, se activa la existente en lugar de
  duplicarla (comparando la ruta normalizada).
- Clic en una pestaña la activa. **Ctrl+Tab** y **Ctrl+Mayús+Tab** recorren las
  pestañas; **Ctrl+T** abre un documento nuevo y **Ctrl+W** cierra la activa.
- El aspa, **clic del medio** o Ctrl+W cierran una pestaña. Al cerrar la activa
  se activa la vecina; al cerrar la última se vuelve a la pantalla de bienvenida.
- Los atajos se muestran en [accesibilidad](accessibility.md).

## Modelo de memoria y posición

Cada pestaña recuerda su posición gracias al perfil de lectura existente: al
cambiar de pestaña se guarda la posición del documento saliente y se restaura la
del entrante (página, zoom, posición vertical y horizontal), igual que al abrir
otro PDF. No se conservan entre cambios la selección ni la búsqueda pendiente:
al volver a una pestaña se reabre el documento desde disco.

Solo hay **un documento cargado** en memoria a la vez. Cambiar de pestaña vuelve
a abrir el archivo; para documentos remotos o grandes esto depende del I/O. Esta
es una implementación intencionalmente liviana: no multiplica la memoria de
PDFium por pestaña. Si más adelante se quiere conmutación instantánea, habría que
mantener un documento PDFium por pestaña en el worker, con su costo de memoria.

## Verificación

`--benchmark-tabs <a> <b> <informe>` (integrado en `test-reader.ps1`) comprueba:
abrir dos documentos, reutilizar una pestaña ya abierta, activar, cerrar una
pestaña conservando la vecina y cerrar la última volviendo a la bienvenida. Las
capturas reales de la ventana verifican la franja en tema claro y oscuro.

Pendiente manual: alternar con muchos documentos, archivos protegidos (que
vuelven a pedir contraseña al reactivar), documentos remotos y la sensación al
cambiar rápido con teclado y mouse.
