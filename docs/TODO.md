# Pendientes de Chucao PDF Reader

Actualizado: 2026-11-13.

Lista consolidada de trabajo pendiente. Las casillas se marcan cuando el resultado
está implementado y verificado; las validaciones manuales se registran por separado.
El contexto de las etapas está en [roadmap.md](roadmap.md).

## Funciones y diseño

- [ ] Diseñar y validar con el usuario la interfaz definitiva inspirada en MarkPad:
  barra compacta y paneles opcionales.
- [ ] Incorporar temas claro y oscuro.
- [ ] Incorporar pestañas para varios documentos.
- [ ] Recordar página, zoom y posición de lectura por documento.
- [ ] Incorporar archivos recientes.
- [ ] Permitir abrir enlaces web mediante una acción explícita del usuario.
- [ ] Respetar posición y modo de ajuste de los marcadores del índice; actualmente
  saltan al inicio de la página conservando el zoom.
- [ ] Completar los modos de destino de enlaces internos, incluidos Fit, FitH,
  FitV y el zoom indicado por el PDF. Actualmente se aplica la posición XYZ y se
  conserva el zoom actual; los otros modos van al inicio de la página.
- [ ] Mejorar accesibilidad y navegación por teclado, incluidos foco y controles.
- [ ] Adaptar la interfaz a distintos DPI por monitor.
- [ ] Revisar y pulir los mensajes de error.

## Validación de funciones implementadas

- [ ] Contraseñas: correcta, incorrecta, cancelación y caracteres especiales.
- [ ] Impresión física y virtual: rangos, orientación, tamaño, copias, cancelación
  y errores de impresora/spooler.
- [ ] Seleccionar, copiar y pegar en otras aplicaciones, incluidos acentos y Unicode.
- [ ] Comprobar documentos con restricciones de copia e impresión.
- [ ] Probar el arrastre físico de selección y el autoscroll entre páginas.
- [ ] Probar desplazamiento horizontal con mouse, rueda y touchpad, incluido zoom alto.
- [ ] Validar el índice con teclado y mouse, y revisar su aspecto a distintos DPI.
- [ ] Probar enlaces internos de documentos reales, cursor, clic y regreso con
  Alt+Izquierda.
- [ ] Abrir documentos consecutivamente con rapidez y comprobar que los resultados
  anteriores no sustituyan al documento actual.
- [ ] Cambiar zoom y tamaño de ventana durante el renderizado.
- [ ] Cerrar durante lectura, búsqueda o renderizado, sin bloqueos ni fugas.
- [ ] Medir CPU y memoria tras muchas aperturas y recorridos.
- [ ] Probar documentos dañados y de dimensiones extremas.
- [ ] Validar documentos grandes y remotos con diferentes estados de caché.

## Rendimiento

- [ ] Medir arranque y tiempo hasta la primera página visible en pantalla,
  distinguiéndolos de la generación y recepción del bitmap.
- [ ] Comparar Chucao, SumatraPDF y KillerPDF como aplicaciones completas con un
  corpus reproducible, versiones, equipo y estado de caché documentados.
- [ ] Investigar la cancelación o aislamiento de lecturas lentas: actualmente una
  lectura de página/recurso puede bloquear el worker hasta que termine el I/O.
- [ ] Confirmar con mediciones controladas el efecto de la carga diferida de PDFium
  sobre la ventana vacía y la primera pintura.

MuPDF y KillerPdf.Engine siguen como alternativas condicionadas a problemas o
ventajas medibles. No está decidido cambiar de motor. La caché experimental de
lectura por bloques permanece fuera del visor hasta que haya evidencia favorable.

## Distribución

- [ ] Preparar un paquete portable con dependencias y avisos necesarios.
- [ ] Preparar un instalador después de validar la versión portable.
- [ ] Definir la licencia del código propio.
- [ ] Revisar e incluir los avisos y licencias de PDFium y sus dependencias.
- [ ] Definir y documentar el proceso de actualización de PDFium.

## Documentación

- [ ] Revisar notas históricas y README para corregir referencias a funciones que
  ya se implementaron y distinguir limitaciones actuales de observaciones anteriores.
- [ ] Mantener esta lista al cerrar cada hito y dejar evidencia de las pruebas en
  la documentación correspondiente.

## Ya implementado; no duplicar trabajo

Apertura de archivos, contraseña e impresión; lectura continua; zoom con ancla;
caché acotada y precarga de páginas vecinas; selección/copia entre páginas;
búsqueda resaltada con navegación; desplazamiento horizontal; índice jerárquico
bajo demanda; enlaces internos y regreso al punto anterior. Algunas de estas
funciones todavía requieren las validaciones manuales indicadas arriba.

## Fuera de alcance permanente

Edición de PDF, anotaciones, OCR, combinar/dividir, redactar, firmar y crear
documentos. Chucao es únicamente un lector. pCloud es un escenario de archivos
remotos, no una integración específica requerida.
