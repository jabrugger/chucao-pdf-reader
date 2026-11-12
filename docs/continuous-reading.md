# Lectura continua — 2026-11-12

## Implementación

- Las páginas forman un recorrido vertical continuo, con separación de 24 píxeles.
  Rueda, Page Up/Down, flechas arriba/abajo y barra recorren el documento; izquierda
  y derecha o los botones saltan a la página anterior/siguiente. Home/End van a los
  extremos. El indicador de página sigue la posición de lectura.
- No se consultan las dimensiones de todas las páginas al abrir. Se usa una
  estimación inicial, ajustada con la primera página y luego con cada página visitada.
  Los documentos con tamaños distintos pueden ajustar la longitud de la barra al
  descubrir nuevas dimensiones; se conserva el ancla de lectura durante el ajuste.
- Se dibujan primero las páginas visibles y luego las vecinas. Las imágenes se
  conservan en una caché de 96 MiB, con máximo de 12 millones de píxeles por bitmap.
  El límite no incluye documentos, fuentes o buffers internos de PDFium ni el buffer
  de pintura de la ventana. El worker puede tener un bitmap adicional en preparación.
- Al hacer zoom o cambiar el ancho se conserva página y fracción vertical. Se muestra
  la imagen anterior escalada hasta recibir la nueva, evitando una pantalla vacía.
- El worker sigue siendo el único hilo que llama a PDFium. Usa render progresivo y
  comprueba si cambió el trabajo solicitado en los puntos de pausa del motor. Cargar
  una página o un recurso en un proveedor lento todavía puede bloquear ese worker
  hasta que termine la lectura: no se promete cancelación instantánea de I/O.
- Un bitmap completado a la escala actual puede reutilizarse aunque el usuario haya
  desplazado la vista desde que se solicitó; resultados de otros documentos o escalas
  no reemplazan el contenido actual. La pintura de la ventana usa doble buffer.

## Verificación

`scripts/test-reader.ps1` ejecuta la prueba del motor, la geometría sin UI y una
secuencia con el ejecutable real en una ventana oculta. Los fixtures son sintéticos
y quedan excluidos de Git: uno de dos páginas y otro con páginas verticales,
horizontales, cortas y altas. Opcionalmente acepta `-Pdf` para un archivo del usuario.

La secuencia comprueba cambio de página al desplazar, zoom con ancla, ir a última
página, reutilización sin nueva petición de render, regreso al principio y reapertura
del documento, rechazando resultados anteriores. Comprueba también el límite de la
caché. No envía impresiones ni modifica archivos de origen.

La prueba de geometría cubre límites de documento, páginas de proporciones distintas,
estimaciones, ancla de zoom y extensiones superiores a 32 bits. El recorrido integrado
se probó además con el libro de PHP de 410 páginas indicado por el usuario.

Estas pruebas no equivalen a inspección visual ni evalúan sensación de fluidez en
pantalla. Pendientes: inspección visual, selección/copia, resaltado de búsqueda,
índice, desplazamiento horizontal en zoom alto y validación de impresión/contraseñas.
