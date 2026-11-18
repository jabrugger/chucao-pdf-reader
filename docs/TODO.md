# Pendientes de Chucao PDF Reader

Actualizado: 2026-11-18.

Lista consolidada de trabajo pendiente. Las casillas se marcan cuando el resultado
está implementado y verificado; las validaciones manuales se registran por separado.
El contexto de las etapas está en [roadmap.md](roadmap.md).

## Funciones y diseño

- [x] Incorporar el chucao aportado por el usuario como icono del EXE y las ventanas,
  imagen de bienvenida y recurso gráfico del proyecto, conservando el original.

- [ ] Diseñar y validar con el usuario la interfaz definitiva inspirada en MarkPad:
  barra compacta y paneles opcionales.
- [x] Primera disposición de lectura con iconos vectoriales, ayudas de teclado,
  búsqueda inferior ocultable e índice integrado, automático y plegable con vista
  temporal desde el borde. Un clic navega por el índice. Evidencia:
  [reader-interface.md](reader-interface.md). Pendiente de aprobación visual del usuario.
- [x] Ocultar el borde de foco de los iconos al usar el mouse y conservarlo
  para la navegación por teclado.
- [x] Usar el icono del chucao en lugar del texto de marca de la esquina superior
  derecha de la barra de lectura. En 2026-11-18 ese acceso se unificó como icono
  vectorial de información; el chucao queda en la ventana, el EXE y Acerca de.
- [x] Agregar Acerca de desde el pajarito o F1, con versión, autor/contacto,
  fecha de release y acceso al repositorio de GitHub.
- [x] Simplificar Acerca de: repositorio como enlace nativo, sin botones;
  cierre con Escape o la X del diálogo.
- [x] Evitar el foco automático del enlace al abrir Acerca de; conservar acceso
  con Tab y activación por teclado.
- [x] Mantener temporal el índice desplegado por hover al elegir un capítulo;
  devolver el foco al documento y colapsarlo al retirar el puntero.
- [x] Primera pasada de pulido visual: barra agrupada, acción Abrir con etiqueta,
  indicador de página y zoom, acento cálido, índice sin líneas y búsqueda plana.
  Compilación y regresión aprobadas; pendiente la devolución visual del usuario.
- [x] Unificar la barra de herramientas: iconos vectoriales antialiasing, sin
  marcos nativos, separador propio para Acerca de (icono de información) y sol/luna
  para el tema. Evidencia: [reader-interface.md](reader-interface.md) y
  [accessibility.md](accessibility.md).
- [x] Incorporar temas claro y oscuro, con botón, Ctrl+D y preferencia local.
  Evidencia: [themes.md](themes.md).
- [x] Incorporar pestañas para varios documentos, con franja, reutilización,
  cierre (×, clic medio, Ctrl+W), Ctrl+T y Ctrl+Tab. Evidencia: [tabs.md](tabs.md).
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
- [x] Completar FitR, FitB, FitBH y FitBV con el rectángulo indicado o la caja de
  contenido de la página. Evidencia: [outline-navigation.md](outline-navigation.md).
- [ ] Conservar la posición anterior cuando FitH, FitV, FitBH o FitBV omiten su
  coordenada (la API numérica usada no distingue null de cero).
- [x] Mejorar accesibilidad y navegación por teclado: orden de tabulación, foco
  entre documento/índice/barra, F6, Espacio, +/−, Escape y nombres MSAA de los
  controles. Evidencia: [accessibility.md](accessibility.md).
- [x] Paleta de alto contraste de Windows: se sustituye la paleta propia por los
  colores del sistema y se reevalúa ante cambios. Evidencia:
  [accessibility.md](accessibility.md).
- [ ] Anuncios para lectores de pantalla (barra de estado como región viva).
- [x] Adaptar la interfaz a distintos DPI por monitor, con fuentes, iconos,
  márgenes y controles reescalados y `WM_DPICHANGED`. Evidencia: [dpi.md](dpi.md).
- [x] Revisar y pulir los mensajes de error.
  - [x] Distinguir el primer pedido de contraseña del aviso de contraseña incorrecta.
  - [x] Distinguir formato, archivo ausente, protección no admitida, documento sin
    páginas y errores desconocidos; agregar el nombre del archivo y el número de
    página cuando corresponde. Evidencia: [error-messages.md](error-messages.md).

## Validación de funciones implementadas

- [x] Recompilar en esta PC la interfaz integrada de `8e46ece` y ejecutar la suite
  completa el 2026-11-16. Revisadas las capturas con/sin índice y búsqueda inferior.
  La validación visual del usuario continúa pendiente.

- [x] Comprobar orden y límite de recientes, nombres repetidos, Unicode,
  apertura desde el menú, archivo no disponible y borrado sin reinserción al cerrar.
- [ ] Validar manualmente el menú de recientes con mouse y teclado, su confirmación
  de borrado y la reapertura de archivos remotos/protegidos.

- [x] Generar el paquete portable y verificar automáticamente su contenido y la
  apertura/renderizado del ejecutable empaquetado. Evidencia:
  [distribution.md](distribution.md). Queda pendiente probarlo en otra PC y a
  distintos DPI antes de preparar un instalador.

- [x] Comprobar la recuperación de posición entre procesos, rutas Unicode, dos
  documentos distintos y PDF protegido, sin guardar la contraseña.
- [x] Comprobar límites, perfiles dañados, fallo de escritura y escritores
  simultáneos para el estado de lectura.
- [ ] Validar manualmente cerrar/reabrir documentos reales en distintas ventanas
  y tamaños de pantalla, incluidos archivos remotos y protegidos.

- [x] Validar automáticamente el cambio de tema, la persistencia, la caída a claro
  con un perfil corrupto y las capturas oscuras. Evidencia: [themes.md](themes.md).
- [ ] Validar manualmente el contraste de ambos temas a distintos DPI, el alto
  contraste de Windows y el cambio de tema con varias ventanas abiertas.

- [x] Validar automáticamente el orden de tabulación, el ciclo de foco, los atajos
  de paneo y los nombres accesibles de los controles. Evidencia:
  [accessibility.md](accessibility.md).
- [x] Validar automáticamente que la paleta de alto contraste use los colores del
  sistema y guarde captura. Evidencia: [accessibility.md](accessibility.md).
- [ ] Validar manualmente con Narrator/NVDA, alto contraste, IME y navegación
  completa solo con teclado en documentos reales.

- [x] Validar automáticamente el reescalado a 144 y 192 DPI de fuentes, botones,
  campos, alto de fila del árbol y capturas. Evidencia: [dpi.md](dpi.md).
- [ ] Validar manualmente monitores reales a 125–200 %, arrastre entre monitores,
  reinicio en un monitor secundario, escalas fraccionarias y alto contraste.

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
- [x] Validar ajuste de página/ancho/alto/rectángulo/caja de contenido, zoom XYZ
  explícito o cero y límites, con páginas normales y rotadas/recortadas.
- [ ] Validar manualmente ajustes y zoom de enlaces/marcadores de PDF reales.
- [ ] Probar enlaces internos de documentos reales, cursor, clic y regreso con
  Alt+Izquierda.
- [x] Validar destinos web, UTF-8, protocolos no admitidos y descarte de resultados
  atrasados, sin abrir un navegador durante las pruebas.
- [ ] Validar manualmente clic en enlaces web reales, navegador predeterminado
  y mensaje de error si Windows no puede abrirlo.
- [x] Validar automáticamente que abrir dos documentos consecutivos mantenga el
  segundo y descarte un resultado atrasado del primero (`--benchmark-rapid`).
- [ ] Validar manualmente abrir muchos documentos seguidos con mouse/teclado y con
  archivos reales remotos o protegidos.
- [x] Validar automáticamente la gestión de pestañas: abrir, reutilizar, activar,
  cerrar conservando la vecina y volver a la bienvenida. Evidencia:
  [tabs.md](tabs.md).
- [ ] Validar manualmente alternar pestañas con muchos documentos, archivos
  protegidos y remotos, y con teclado y mouse.
- [ ] Cambiar zoom y tamaño de ventana durante el renderizado.
- [ ] Cerrar durante lectura, búsqueda o renderizado, sin bloqueos ni fugas.
- [ ] Medir CPU y memoria tras muchas aperturas y recorridos.
- [ ] Probar documentos dañados y de dimensiones extremas.
- [ ] Validar documentos grandes y remotos con diferentes estados de caché.

## Rendimiento

- [x] Mejorar la tolerancia del guardado de historial a la contención con E/S lenta:
  espera creciente de hasta dos segundos para el bloqueo y reintentos del reemplazo
  final. La prueba cubre un bloqueo mantenido 700 ms. Evidencia:
  [reading-state.md](reading-state.md).
- [x] Medir el arranque y el tiempo hasta el primer cuadro compuesto,
  distinguiendo apertura, render, recepción del bitmap y composición. Evidencia:
  [performance.md](performance.md). La presentación física (DWM/scan-out) sigue
  pendiente y se registra abajo.
- [ ] Medir la presentación física en pantalla con trazas del sistema o captura
  externa; `first_compose_ms` no incluye DWM ni el barrido del monitor.
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

- [x] Preparar un paquete portable con dependencias y avisos necesarios.
  Evidencia: [distribution.md](distribution.md). `package.ps1` genera el ZIP y
  `test-package.ps1` lo descomprime y lo ejecuta.
- [ ] Preparar un instalador después de validar la versión portable.
- [x] Definir la licencia del código propio: MIT en [LICENSE](../LICENSE).
- [x] Revisar e incluir los avisos y licencias de PDFium y sus dependencias.
  Evidencia: [THIRD-PARTY-NOTICES.md](../THIRD-PARTY-NOTICES.md) y `licenses/`
  del paquete.
- [x] Definir y documentar el proceso de actualización de PDFium.
  Evidencia: [pdfium-update.md](pdfium-update.md).

## Documentación

- [x] Corregir el README y distinguir el registro inicial de validación de las
  comprobaciones actuales de contraseñas y enlaces internos.
- [x] Documentar el paquete portable, la licencia MIT y el procedimiento de
  actualización de PDFium.

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
