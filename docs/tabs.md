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
- Un solo clic en un PDF de la lista de recientes lo abre, aunque ya estuviera
  seleccionado.
- Una **mini pestaña «+»** al final de la franja, después de la última pestaña,
  muestra la pantalla de recientes sin cerrar los documentos. Al elegir un
  reciente se abre o activa su pestaña; **Alt+Home** hace lo mismo y **Ctrl+W**
  sobre la vista de recientes vuelve al último documento.
- Clic en una pestaña la activa. **Ctrl+Tab** y **Ctrl+Mayús+Tab** recorren las
  pestañas; **Ctrl+T** abre un documento nuevo y **Ctrl+W** cierra la activa.
- El aspa, **clic del medio** o Ctrl+W cierran una pestaña. Al cerrar la activa
  se activa la vecina; al cerrar la última se vuelve a la pantalla de bienvenida.
- Mientras el documento se abre, su pestaña muestra un indicador central animado
  con el nombre del archivo; desaparece al recibir el primer bitmap o al fallar.
- Los atajos se muestran en [accesibilidad](accessibility.md).

## Modelo de memoria y posición

Cada pestaña recuerda página, zoom y posición vertical/horizontal en memoria
durante la sesión, incluso si el perfil de lectura no se puede escribir. Al volver
se utiliza primero ese estado y, si no existe, el perfil local habitual. El estado
se captura antes de cambiar de documento y sigue sujeto a los límites de la vista
y a la estimación de tamaños de páginas no visitadas. Olvidar historial y posiciones
también limpia los estados de las pestañas en esta ventana.
No se conservan entre cambios la selección ni la búsqueda pendiente:
al volver a una pestaña se reabre el documento desde disco.

Solo hay **un documento cargado** en memoria a la vez. Cambiar de pestaña vuelve
a abrir el archivo; para documentos remotos o grandes esto depende del I/O. Esta
es una implementación intencionalmente liviana: no multiplica la memoria de
PDFium por pestaña. Si más adelante se quiere conmutación instantánea, habría que
mantener un documento PDFium por pestaña en el worker, con su costo de memoria.

Al cerrar la última pestaña se invalida el trabajo anterior y se encola el cierre
del documento en el worker. Solo ese hilo llama a `FPDF_CloseDocument` y confirma
que finalizó. La biblioteca y el worker siguen disponibles para la siguiente
apertura. Una lectura de disco/red bloqueante puede demorar el cierre hasta que
regrese el I/O; no se destruye el hilo ni se llama a PDFium desde la interfaz.

## Errores y contraseña cancelada

Si un archivo no está disponible o se cancela la contraseña, la pestaña permanece
abierta con el aviso en la barra de estado. Volver a hacer clic en esa pestaña o
elegir el mismo archivo reintenta la apertura sin duplicarla. Mientras está cargando
o ya está listo, elegir de nuevo el mismo documento solo activa la pestaña.
También se puede cerrar la fallida y volver a otra. Las contraseñas no se guardan
en el estado de pestañas y se solicitan de nuevo al reactivar un protegido.

## Verificación

`scripts/test-tabs.ps1 -Executable 'cmake\Release\Chucao.exe'`, integrado en la
suite general, copia PDF sintéticos a una carpeta única con rutas Unicode. Ejecuta
`--benchmark-tabs <a> <b> <protegido> <informe>` con ventana oculta y sin leer ni
escribir el perfil real. Realiza 27 comprobaciones:

- Espera los bitmaps de A/B y sus páginas destino; restaura sus posiciones a zoom
  300/400%, incluida la coordenada horizontal alcanzable en la vista.
- Reutiliza una pestaña y cierra la inactiva sin recargar la activa.
- Descarta resultados antiguos de apertura, bitmap, error, contraseña y cierre.
- Cierra la última y espera el acuse del worker antes de volver a abrir un PDF.
- Reintenta una ruta inexistente sin duplicar pestañas y regresa al documento válido.
- Cancela un pedido de contraseña, reintenta con la correcta y vuelve a solicitarla
  al cambiar de pestaña y regresar. Tres pedidos se resuelven con el diálogo real
  automatizado y una contraseña sintética pública.
- Cierra de nuevo la última pestaña protegida y espera la segunda confirmación.

El informe incluye `tabs_checks`, `tabs_step`, `tabs_password_prompts` y
`tabs_closed`. Se guarda una captura de la franja; no se mueve el puntero ni se
activan ventanas del escritorio. La prueba anterior solo comprobaba el modelo
sin esperar las cargas y fue reemplazada por este ciclo asincrónico.

Pendiente manual: alternar con muchos documentos, archivos protegidos (que
vuelven a pedir contraseña al reactivar), documentos remotos y la sensación al
cambiar rápido con teclado y mouse.
