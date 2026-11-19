# Pendientes de Chucao PDF Reader

Actualizado: 2026-11-19.

Lista consolidada de trabajo pendiente. Las casillas se marcan cuando el resultado
está implementado y verificado; las validaciones manuales se registran por separado.
El contexto de las etapas está en [roadmap.md](roadmap.md).

## Próxima tanda, en orden

Estado revisado tras el trabajo de DS: [continuation-plan.md](continuation-plan.md).

1. [x] Estabilizar pestañas con cargas reales A→B→A y recuperación de posición;
   verificar errores y contraseña cancelada. Evidencia: [tabs.md](tabs.md).
2. [x] Liberar el PDF en el worker al cerrar la última pestaña y descartar sus
   trabajos anteriores mediante revisiones/generaciones. La prueba espera el acuse
   de cierre; una lectura de I/O bloqueante sigue siendo un límite aparte.
3. [ ] Validar uso cotidiano y rendimiento con documentos reales antes de agregar
   nuevas funciones grandes.
4. [x] Preparar la entrega portable 0.2.0 con versión/fecha y binario coherentes;
   ZIP regenerado y verificado localmente.
   [x] Preparar el instalador Inno Setup con asociación opcional de PDF.
   [ ] Probar el ZIP y el instalador en otra PC antes de publicarlos.

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
- [x] Agregar Acerca de desde su icono o F1, con versión, autor/contacto,
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
- [x] Incorporar temas claro/oscuro y «Igual que el sistema», con Opciones,
  Ctrl+D para alternar explícitamente y preferencia local.
  Evidencia: [themes.md](themes.md).
- [x] Incorporar pestañas para varios documentos, con franja, reutilización,
  cierre (×, clic medio, Ctrl+W), Ctrl+T y Ctrl+Tab. Evidencia: [tabs.md](tabs.md).
- [x] Recordar página, zoom y posición vertical/horizontal por documento, al cerrar
  o cambiar de PDF. Evidencia: [reading-state.md](reading-state.md).
- [x] Recordar tamaño, posición y estado maximizado de la ventana; ignorar
  posiciones fuera de todo monitor. Evidencia: [preferences.md](preferences.md).
- [x] Ajustar al ancho, a la página completa o al alto con Ctrl+0/1/2, indicador
  de modo y reajuste al cambiar el tamaño de la ventana. Evidencia:
  [reader-interface.md](reader-interface.md).
- [x] Ofrecer asociar los PDF en el instalador y en Opciones, con registro por
  usuario, aviso de `UserChoice` y limpieza al desinstalar. Evidencia:
  [installer.md](installer.md).
- [x] Diálogo de Opciones (tema, reabrir la última sesión opcional, olvidar
  historial y sesión). Evidencia automatizada: [preferences.md](preferences.md).
- [x] Preferencia de apertura automática del índice y guardado de la sesión al
  abrir, activar y cerrar pestañas; bienvenida con recientes y Abrir directo.
- [ ] Validar manualmente el diálogo de Opciones (mouse, teclado, DPI, tema oscuro)
  y la restauración con documentos reales, protegidos y remotos.
- [x] Filtro del índice: cuadro sobre el árbol que oculta lo que no coincide y
  conserva los padres de cada coincidencia. Ignora mayúsculas y acentos, muestra
  «N de M secciones», Enter va a la primera coincidencia, Escape limpia y flecha
  abajo pasa al árbol; el sombreado sigue funcionando entre los resultados.
  Evidencia: [document-outline.md](document-outline.md). Queda la validación
  manual del cuadro con mouse, IME y distintos DPI.
- [x] Sombrear en el índice la sección que se está leyendo (por página; un capítulo
  plegado sombrea su padre visible). Pendiente: validarlo a mano con documentos
  reales grandes y a distintos DPI.
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
- [x] Anuncios para lectores de pantalla: la barra de estado es un control
  accesible visible y emite un evento al cambiar el mensaje. Evidencia:
  [accessibility.md](accessibility.md). Queda la validación manual con Narrator.
- [x] Adaptar la interfaz a distintos DPI por monitor, con fuentes, iconos,
  márgenes y controles reescalados y `WM_DPICHANGED`. Evidencia: [dpi.md](dpi.md).
- [x] Revisar y pulir los mensajes de error.
  - [x] Distinguir el primer pedido de contraseña del aviso de contraseña incorrecta.
  - [x] Distinguir formato, archivo ausente, protección no admitida, documento sin
    páginas y errores desconocidos; agregar el nombre del archivo y el número de
    página cuando corresponde. Evidencia: [error-messages.md](error-messages.md).

## Validación de funciones implementadas

- [x] Revisar `07de759`, recompilar y ejecutar la suite completa el 2026-11-19.
  Estado y límites: [review-2026-11-19.md](review-2026-11-19.md).

- [x] Corregir la suite para que todos los modos nuevos respeten `-Executable`,
  sin mezclar pruebas de otra compilación de Chucao.
- [x] Recompilar `3f55ece` y completar la regresión del 2026-11-18 con el ejecutable
  elegido. Alcance y próximo trabajo: [continuation-plan.md](continuation-plan.md).

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
- [x] Cortar la entrega 0.2.0: versión y fecha actualizadas en `app-info.h`, CMake
  y documentación; ZIP `Chucao-0.2.0-win-x64` regenerado y verificado localmente.

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
- [x] Validar automáticamente restaurar/guardar la posición de ventana, el estado
  maximizado y el descarte de posiciones fuera de pantalla. Evidencia:
  [preferences.md](preferences.md).
- [x] Validar automáticamente ajuste a página, alto y ancho con páginas de tamaños
  distintos. Evidencia: [reader-interface.md](reader-interface.md).
- [x] Validar automáticamente que la barra de estado sea visible y su nombre
  accesible coincida con el mensaje. Evidencia: [accessibility.md](accessibility.md).
- [x] Validar automáticamente la asociación con una extensión desechable:
  registro, comando, icono, desregistro y conservación del default ajeno.
  Evidencia: [installer.md](installer.md).
- [ ] Validar manualmente la instalación, la asociación real de `.pdf`, la
  actualización y la desinstalación en otra PC.
- [ ] Validar manualmente los ajustes de zoom con PDF reales, ventanas extremas y
  DPI altos.
- [ ] Validar manualmente tamaño/posición entre monitores reales, maximizar,
  cerrar y reabrir, y escalas de DPI distintas.
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
- [x] Esperar bitmaps reales al alternar A→B→A, recuperar página/zoom/posición sin
  historial en disco, reintentar error y contraseña cancelada, volver a autenticar
  y confirmar el cierre del PDF en el worker (27 comprobaciones).
- [ ] Validar manualmente alternar pestañas con muchos documentos, archivos
  protegidos y remotos, y con teclado y mouse.
- [ ] Cambiar zoom y tamaño de ventana durante el renderizado.
- [ ] Cerrar durante lectura, búsqueda o renderizado, sin bloqueos ni fugas.
- [ ] Medir CPU y memoria tras muchas aperturas y recorridos.
- [x] Validar automáticamente PDF truncados, basura, vacíos, con página faltante,
  caja cero y páginas de 5000×5000/1×1, además del presupuesto de 16 000 px y
  12 MP. Evidencia: [robustness.md](robustness.md).
- [ ] Validar manualmente documentos dañados y de dimensiones extremas reales,
  incluida la ventana visible y la impresión.
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
- [x] Preparar el instalador con Inno Setup 7, instalación por usuario, tarea
  opcional de asociación de PDF y opción equivalente en Opciones.
  Evidencia: [installer.md](installer.md).
- [ ] Validar instalación, asociación, actualización y desinstalación en otra PC
  antes de publicar el instalador.
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
bajo demanda; enlaces internos y regreso al punto anterior; temas, DPI por monitor,
teclado/accesibilidad básica, alto contraste y primera versión de pestañas.
Algunas de estas
funciones todavía requieren las validaciones manuales indicadas arriba.

## Fuera de alcance permanente

Edición de PDF, anotaciones, OCR, combinar/dividir, redactar, firmar y crear
documentos. Chucao es únicamente un lector. pCloud es un escenario de archivos
remotos, no una integración específica requerida.
