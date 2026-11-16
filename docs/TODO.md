# Pendientes de Chucao PDF Reader

Actualizado: 2026-11-15.

Lista consolidada de trabajo pendiente. Las casillas se marcan cuando el resultado
está implementado y verificado; las validaciones manuales se registran por separado.
El contexto de las etapas está en [roadmap.md](roadmap.md).

## Funciones y diseño

- [x] Incorporar el chucao aportado por el usuario como icono del EXE y las ventanas,
  imagen de bienvenida y recurso gráfico del proyecto, conservando el original.

- [ ] Diseñar y validar con el usuario la interfaz definitiva inspirada en MarkPad:
  barra compacta y paneles opcionales.
- [ ] Incorporar temas claro y oscuro.
- [ ] Incorporar pestañas para varios documentos.
- [x] Recordar página, zoom y posición vertical/horizontal por documento, al cerrar
  o cambiar de PDF. Evidencia: [reading-state.md](reading-state.md).
- [x] Incorporar archivos recientes, con apertura bajo demanda y opción para
  olvidar historial y posiciones. Evidencia: [recent-files.md](recent-files.md).
- [x] Permitir abrir enlaces HTTP/HTTPS con clic explícito en el navegador
  predeterminado. Evidencia: [internal-links.md](internal-links.md).
- [x] Respetar coordenadas XYZ y regreso desde los marcadores del índice.
  Evidencia: [outline-navigation.md](outline-navigation.md).
- [x] Aplicar Fit, FitH, FitV y zoom XYZ en marcadores y enlaces internos, con
  límites de zoom y regreso a la vista anterior.
- [ ] Completar FitR, FitB, FitBH y FitBV; conservar la posición anterior cuando
  FitH/FitV omiten su coordenada (la API numérica usada no distingue null de cero).
- [ ] Mejorar accesibilidad y navegación por teclado, incluidos foco y controles.
- [ ] Adaptar la interfaz a distintos DPI por monitor.
- [ ] Revisar y pulir los mensajes de error.
  - [x] Distinguir el primer pedido de contraseña del aviso de contraseña incorrecta.

## Validación de funciones implementadas

- [x] Comprobar orden y límite de recientes, nombres repetidos, Unicode,
  apertura desde el menú, archivo no disponible y borrado sin reinserción al cerrar.
- [ ] Validar manualmente el menú de recientes con mouse y teclado, su confirmación
  de borrado y la reapertura de archivos remotos/protegidos.

- [x] Comprobar la recuperación de posición entre procesos, rutas Unicode, dos
  documentos distintos y PDF protegido, sin guardar la contraseña.
- [x] Comprobar límites, perfiles dañados, fallo de escritura y escritores
  simultáneos para el estado de lectura.
- [ ] Validar manualmente cerrar/reabrir documentos reales en distintas ventanas
  y tamaños de pantalla, incluidos archivos remotos y protegidos.

- [ ] Revisar el icono en Explorer/barra de tareas y la bienvenida a distintos DPI.

- [x] Validar automáticamente contraseñas correctas, vacías e incorrectas,
  reintentos, Unicode, cancelación y recuperación. Evidencia: [passwords.md](passwords.md).
- [x] Validar automáticamente cifrado AES-256, AES-128 y RC4, búsqueda/renderizado
  tras autenticación y permisos de lector/propietario.
- [ ] Validar manualmente el ingreso de contraseña, foco, teclado/IME y cancelación
  en la ventana visible, incluidos caracteres especiales.
- [ ] Impresión física y virtual: rangos, orientación, tamaño, copias, cancelación
  y errores de impresora/spooler.
- [ ] Seleccionar, copiar y pegar en otras aplicaciones, incluidos acentos y Unicode.
- [x] Comprobar los controles de copia e impresión del lector con un PDF sintético
  restringido, sin escribir al portapapeles ni iniciar trabajos de impresión.
- [ ] Comprobar documentos reales con restricciones de copia e impresión y el
  comportamiento al pegar/imprimir en otras aplicaciones o dispositivos.
- [ ] Probar el arrastre físico de selección y el autoscroll entre páginas.
- [ ] Probar desplazamiento horizontal con mouse, rueda y touchpad, incluido zoom alto.
- [x] Corregir y verificar el arrastre de la barra vertical con mouse en ambas
  direcciones. Evidencia y regresión: [scrollbar-input.md](scrollbar-input.md).
- [ ] Validar el índice con teclado y mouse, y revisar su aspecto a distintos DPI.
- [x] Validar automáticamente saltos del índice a coordenadas XYZ, coordenadas
  omitidas, rotación/recorte, regreso y descarte de resultados atrasados.
- [x] Validar ajuste de página/ancho/alto, zoom XYZ explícito o cero y límites,
  con páginas normales y rotadas/recortadas.
- [ ] Validar manualmente ajustes y zoom de enlaces/marcadores de PDF reales.
- [ ] Probar enlaces internos de documentos reales, cursor, clic y regreso con
  Alt+Izquierda.
- [x] Validar destinos web, UTF-8, protocolos no admitidos y descarte de resultados
  atrasados, sin abrir un navegador durante las pruebas.
- [ ] Validar manualmente clic en enlaces web reales, navegador predeterminado
  y mensaje de error si Windows no puede abrirlo.
- [ ] Abrir documentos consecutivamente con rapidez y comprobar que los resultados
  anteriores no sustituyan al documento actual.
- [ ] Cambiar zoom y tamaño de ventana durante el renderizado.
- [ ] Cerrar durante lectura, búsqueda o renderizado, sin bloqueos ni fugas.
- [ ] Medir CPU y memoria tras muchas aperturas y recorridos.
- [ ] Probar documentos dañados y de dimensiones extremas.
- [ ] Validar documentos grandes y remotos con diferentes estados de caché.

## Rendimiento

- [ ] Mejorar la tolerancia del guardado de historial a la contención con E/S lenta:
  en la regresión del 2026-11-13 una prueba de escritores simultáneos agotó la
  espera de 500 ms; la repetición aislada pasó. No se modificó ese límite.
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

- [x] Corregir el README y distinguir el registro inicial de validación de las
  comprobaciones actuales de contraseñas y enlaces internos.

Mantener esta lista al cerrar cada hito y dejar evidencia de las pruebas en la
documentación correspondiente. Una prueba automatizada no cierra por sí sola una
validación manual.

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
