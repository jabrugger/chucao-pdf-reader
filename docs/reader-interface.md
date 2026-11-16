# Interfaz de lectura — 2026-11-15

Primera propuesta funcional según las preferencias del usuario. Conserva Win32/GDI
y PDFium; no incorpora un framework visual ni dependencias gráficas nuevas.

## Controles

- Barra superior de iconos vectoriales para abrir/recientes, página anterior y
  siguiente, zoom, ajuste al ancho, índice, búsqueda e impresión. Cada botón tiene
  nombre accesible, ayuda al pasar el mouse y foco visible al recorrerlo con Tab.
- Búsqueda inferior a demanda con Ctrl+F o la lupa. Consulta incremental con espera
  de 220 ms, anterior/siguiente, indicador de resultado y cierre con Escape o ×.
  Cerrar cancela la búsqueda pendiente, limpia el resaltado y conserva la consulta.
- Índice como panel hijo de la ventana principal. Se abre al detectar marcadores;
  permanece cerrado al abrir un PDF sin ellos. Botón, Ctrl+B y plegado en la cabecera.
- Vista temporal al acercarse al borde izquierdo, sin cambiar el ancho del documento
  ni generar otro render. Se retira al alejarse el puntero y se puede fijar con la
  chincheta o al elegir un capítulo. Clic único y Enter navegan; expandir una rama
  no activa su destino.

El panel fijo cambia el ancho disponible, preservando página, posición vertical
y centro horizontal al ampliar. La búsqueda reduce solamente la altura disponible.
Pintura, selección, hit testing, enlaces y destinos comparten ese espacio de lectura.
Las llamadas a PDFium continúan exclusivamente en el worker; las respuestas de
documentos anteriores y los resultados recibidos con la búsqueda cerrada se descartan.

## Verificación

`benchmark-open.ps1 -Interface`, integrado en `test-reader.ps1`, comprueba con
ventanas ocultas: panel hijo y apertura automática, búsqueda inicialmente oculta,
plegado y recuperación de ancho, vista temporal sin render, centro horizontal con
zoom, búsqueda visible y hit testing del resultado, cierre con conservación de
consulta, descarte de un resultado tardío y cambio a un documento sin índice.
Genera pinturas locales ignoradas por Git para inspeccionar el conjunto de controles.

Una instancia visible con el fixture sintético verificó clic único, Ctrl+F desde
el árbol, escritura y búsqueda incremental, Escape, Ctrl+B y aparición/retiro del
panel al mover el puntero por el borde izquierdo. No se alteró el historial real.
Se ejecuta además la regresión completa de lectura, texto, navegación, enlaces,
destinos, contraseñas/permisos, recientes y persistencia.

Pendientes: validación visual del usuario, DPI por monitor, temas, pestañas y
revisión más amplia de accesibilidad. Esta entrega no declara definitivo el diseño.
