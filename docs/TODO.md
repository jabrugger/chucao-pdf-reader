# Pendientes de Chucao PDF Reader

Actualizado: 2026-11-23.

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
5. [x] Publicar la entrega 0.2.1 con el manifiesto de Windows incrustado para que
   «Acerca de» y «Opciones» funcionen desde la instalación y el ZIP, sin depender
   de un archivo externo. Evidencia: [installer.md](installer.md).
6. [x] Publicar 0.2.2 con apertura de PDF recientes y capítulos del índice en un
   solo clic, y el icono de información más grande. Evidencia: [tabs.md](tabs.md)
   y [reader-interface.md](reader-interface.md).
7. [x] Publicar 0.2.3 con menú contextual de copia por clic derecho, tecla Menú o
   Mayús+F10; resolución del índice antes de la primera página y conservación del
   zoom al navegar capítulos. Evidencia: [reader-interface.md](reader-interface.md)
   y [accessibility.md](accessibility.md).
8. [x] Publicar 0.2.5 con la mini pestaña «+» al final de la fila para volver a
   recientes sin cerrar documentos, y la tabla completa de atajos en Acerca de.
   Evidencia: [tabs.md](tabs.md) y [accessibility.md](accessibility.md).
9. [x] Publicar 0.2.6 con Opciones totalmente oscuro (selector segmentado,
   etiquetas y enlaces) y sin bordes nativos en la barra tras cambiar de tema.
   Evidencia: [themes.md](themes.md).
10. [x] Publicar 0.2.7 con logo transparente, barra de título oscura, casillas
    ownerdraw, selección del índice sin gris residual y barras nativas oscuras.
    Evidencia: [themes.md](themes.md) y [performance.md](performance.md).
11. [x] Cortar 0.2.8 con el pulido final del tema oscuro y de la barra nativa,
    la división de `main.cpp` en fragmentos por responsabilidad y los archivos
    comunitarios (CONTRIBUTING, SECURITY, plantillas de issue/PR y CI). Versión y
    fecha en `app-info.h` y CMake; tag `v0.2.8` sobre `98b9056`, empujado a
    `origin`. Evidencia: [themes.md](themes.md) y [CONTRIBUTING.md](../CONTRIBUTING.md).
12. [x] Cortar 0.2.9 con el chequeo de actualizaciones opt-in (desactivado por
    defecto) y su diagnóstico offline. Evidencia: [updater.md](updater.md).
13. [x] Cortar 0.2.10 con el zoom con Ctrl+rueda anclado al puntero, el indicador
    de página «N / M» clickeable para ir a una página, la conservación del scroll
    cuando FitH/FitV/FitBH/FitBV no traen coordenada y la corrección de la línea
    clara de 1 px en los botones y el logo del tema oscuro. Evidencia:
    [reader-interface.md](reader-interface.md), [outline-navigation.md](outline-navigation.md)
    y [themes.md](themes.md).
14. [x] Cortar 0.2.11 con la corrección del arranque: en Windows 11 la ventana
    podía quedar detrás de otras y sin botón en la barra de tareas hasta activarse,
    por lo que parecía que la app se cerraba sola (típicamente al abrirla desde el
    instalador). Ahora pide el primer plano y, si Windows se niega, la vuelve a
    mostrar una vez. Evidencia: [accessibility.md](accessibility.md).
15. [x] Cortar 0.2.12 con la causa raíz del botón de la barra de tareas (normalizar
    `nCmdShow`, parpadear el botón si Windows niega el primer plano y lanzar el
    instalador con `shellexec`), un solo clic para navegar el índice con una «×» para
    limpiar el filtro, y doble clic (palabra) / triple clic (línea/párrafo) en la
    selección de texto. Evidencia: [accessibility.md](accessibility.md),
    [document-outline.md](document-outline.md) y [text-reading.md](text-reading.md).
16. [x] Cortar 0.2.13 limitando el chequeo de actualizaciones a como máximo una vez
    por día (se guarda `updates_last` en `settings.dat`). Evidencia:
    [updater.md](updater.md) y [preferences.md](preferences.md).
17. [x] Cortar 0.2.14 con barra responsiva, `+` siempre visible en la tira de
    pestañas, indicador de zoom clickeable, persistencia de modos de ajuste
    (`FitWidth`/`FitPage`/`FitHeight`), arrastrar y soltar múltiples PDF, botón
    «Buscar ahora» en Opciones, CI ampliada y limpieza de documentación.

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
- [x] Buscar actualizaciones al abrir (opt-in, desactivado por defecto): consulta
  la última release en GitHub, avisa si hay una versión nueva y abre la página de
  descargas en el navegador, sin descarga automática ni telemetría. Evidencia:
  [updater.md](updater.md).
- [x] Selección por doble clic (palabra) y triple clic (línea/párrafo), con los
  caracteres Unicode guardados junto a la geometría. Evidencia:
  [text-reading.md](text-reading.md).
- [x] Indicador central de carga animado por pestaña, con nombre de archivo y
  estados de apertura/primera página, que se detiene al llegar el bitmap o fallar.
  Evidencia: [reader-interface.md](reader-interface.md).
- [x] Diálogo de Opciones (tema, reabrir la última sesión opcional, olvidar
  historial y sesión). Evidencia automatizada: [preferences.md](preferences.md).
- [x] Preferencia de apertura automática del índice y guardado de la sesión al
  abrir, activar y cerrar pestañas; bienvenida con recientes y Abrir directo.
- [ ] Validar manualmente el diálogo de Opciones (mouse, teclado, DPI, tema oscuro)
  y la restauración con documentos reales, protegidos y remotos.
- [ ] Validar manualmente el aviso de actualización con una release publicada, y el
  comportamiento sin red y con proxy/firewall corporativo.
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
- [x] Conservar la posición anterior cuando FitH, FitV, FitBH o FitBV no traen
  coordenada en el arreglo (`[página /FitH]`). Evidencia:
  [outline-navigation.md](outline-navigation.md).
- [ ] Conservar la posición anterior cuando la coordenada es `null`
  (`[página /FitH null]`, el caso habitual en la práctica): la API pública de PDFium
  la informa como 0 y cae en el borde. Requiere otra vía de lectura del arreglo.
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
- [x] Zoom con Ctrl+rueda anclado al punto del PDF bajo el puntero (ese punto
  queda bajo el mismo píxel). Los botones +/− y las teclas conservan su anclado.
  Evidencia: [reader-interface.md](reader-interface.md) y la prueba `-Navigation`.
- [x] Indicador de página «N / M» clickeable: clic abre un campo numérico con el
  valor preseleccionado; Enter va a la página, Escape o perder el foco cancela.
  Validación de entrada y salto cubiertas en `-Navigation`. Evidencia:
  [reader-interface.md](reader-interface.md) y [accessibility.md](accessibility.md).

## Validación de funciones implementadas

- [x] Recompilar v0.2.13 y ejecutar la suite completa el 2026-11-24: código de
  salida 0. ZIP verificado con `test-package.ps1` e instalador generado. SHA-256:
  ZIP `df0e1b8df0769f9d769aaf18d876f10cdcd74cd6f74643cc2c6ff718c4704413`,
  instalador `38447316a3f85269e69bef60697dc49bf7d0852020adbed4f57d9a7f062776c7`.
  Salida local en `test-results/regression-0.2.13-2026-11-24.log`, no versionada.
  El chequeo de actualizaciones una vez por día se cubre en `preferences-test.exe`
  (parseo, valor inválido y round-trip de `updates_last`).

- [x] Recompilar v0.2.12 y ejecutar la suite completa el 2026-11-24: código de
  salida 0. ZIP verificado con `test-package.ps1` e instalador generado. SHA-256:
  ZIP `e9bb0cab07cb3fd981b679732d9e15985c1e6d9defca83a20b93e9ceecdc962b`,
  instalador `971160d23f002fe2373da89b1ff3101e08b1335fee3edccccf66f94675b52bdd`.
  Salida local en `test-results/regression-0.2.12-2026-11-24.log`, no versionada.
  El aviso de actualización, el clic simple del índice y el doble/triple clic se
  probaron con el instalador local; la validación del flujo real del instalador y
  en otra PC sigue pendiente.

- [x] Recompilar v0.2.11 y ejecutar la suite completa el 2026-11-24: código de
  salida 0. El botón de la barra de tareas se comprobó leyendo la barra con UI
  Automation: antes del cambio no aparecía en ningún lanzamiento (desde un proceso
  en segundo plano ni desde el Explorador) y con la compilación y con el EXE del
  ZIP final aparece en todos (3 de 3 y 2 de 2). ZIP verificado con
  `test-package.ps1` sobre el ZIP final (`package-installer.ps1` lo regenera).
  SHA-256: ZIP `f1849ef0a38e2bca580fef2349e65ada85d595691bc4f25459ed94f96d463f9a`,
  instalador `eb325250e78708d60d64ccb5d58c05d531562149f0e5847d31757d4b2146bf80`.
  Salida local en `test-results/regression-0.2.11-2026-11-24.log`, no versionada.
  Pendiente: repetir el flujo completo del instalador (opción «Abrir Chucao» al
  terminar) con esta versión y probarla en otra PC. No se conoce la causa de fondo
  por la que Windows no asigna el botón en el arranque.

- [x] Recompilar v0.2.10 y ejecutar la suite completa el 2026-11-23: código de
  salida 0. ZIP verificado con `test-package.ps1` (sobre el ZIP final, ya que
  `package-installer.ps1` lo regenera) e instalador generado con
  `package-installer.ps1`. SHA-256: ZIP `f2c606882073731f10c1fdc7d9c94b5b6ccfde9fe7cb1e784a5e85b1661a3f9b`,
  instalador `a031d3a927f1fafb89a5e4c72af714abfcaa93ad4eb87c3c87420dda996dfd1c`.
  Salida local en `test-results/regression-0.2.10-2026-11-23.log`, no versionada.
  Siguen pendientes las validaciones manuales (zoom con mouse real, indicador de
  página, tema oscuro en otros DPI) y probar el instalador en otra PC.

- [x] Recompilar v0.2.9 y ejecutar la suite completa el 2026-11-23: código de
  salida 0. ZIP verificado con `test-package.ps1`, instalador generado con
  `package-installer.ps1` y Release publicada en GitHub con ambos assets y sus
  SHA-256. Salida local en `test-results/regression-0.2.9-2026-11-23.log`, no
  versionada.

- [x] Recompilar v0.2.8 (`98b9056`) con `scripts/build.cmd` y ejecutar la suite
  completa el 2026-11-23: código de salida 0. Pasaron apertura/renderizado,
  estado de lectura, recientes, preferencias, ventana, robustez, asociación,
  contraseñas/permisos, mensajes, temas, DPI, teclado, alto contraste, apertura
  rápida, pestañas (18 etapas, 27 comprobaciones, 2 cierres y 3 pedidos de
  contraseña), geometría, arranque, carga, Acerca de, índice, filtro, enlaces,
  destinos, ajustes, navegación y texto. La salida local quedó en
  `test-results/regression-2026-11-23.log`, que no se versiona. Esta corrida no
  convierte en realizadas las validaciones manuales pendientes.

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
- [x] Incrustar el manifiesto de Windows (Common Controls v6) en el EXE, retirar
  la dependencia de `Chucao.exe.manifest` y liberar 0.2.1 con ZIP e instalador
  regenerados. Evidencia: [installer.md](installer.md).

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
- [x] Validar automáticamente que la vista previa del indicador de carga genere
  su captura sin esperar al worker. Evidencia:
  [reader-interface.md](reader-interface.md).
- [x] Validar automáticamente que el ejecutable compilado y el extraído del ZIP
  creen «Acerca de» sin manifiesto externo (`--preview-about`). Evidencia:
  [installer.md](installer.md).
- [x] Verificar localmente la instalación 0.2.2: «Acerca de» y «Opciones» abren
  desde el acceso directo del menú Inicio y el EXE usa Common Controls v6.
  Evidencia: [installer.md](installer.md).
- [x] Abrir con un clic los PDF recientes (incluido un ítem ya seleccionado) y
  navegar con un clic en toda la fila del índice, sin activar destinos al
  expandir una rama. Evidencia: [tabs.md](tabs.md) y
  [reader-interface.md](reader-interface.md).
- [x] Verificar el menú contextual de copia con clic derecho y con la tecla Menú,
  y que «Copiar» vuelque la selección al portapapeles. Evidencia:
  [reader-interface.md](reader-interface.md).
- [x] Verificar que un PDF con índice muestre el panel antes de la primera página
  y que navegar un capítulo conserve el zoom/modo actual; los enlaces internos
  mantienen la vista del destino. Evidencia:
  [reader-interface.md](reader-interface.md).
- [x] Verificar la mini pestaña «+» con dos documentos abiertos: muestra recientes,
  conserva las pestañas y Ctrl+W regresa al último documento. Evidencia:
  [tabs.md](tabs.md).
- [x] Verificar el «+» al final de la fila y la tabla de atajos en Acerca de (F1),
  con desplazamiento y sin scroll horizontal. Evidencia:
  [accessibility.md](accessibility.md).
- [x] Verificar Opciones en oscuro (selector segmentado, etiquetas y enlaces) y que
  la barra no muestre bordes nativos al pasar el mouse tras cambiar de tema.
  Evidencia: [themes.md](themes.md).
- [x] Verificar el logo transparente, la barra de título con colores DWM, las
  casillas ownerdraw con estado persistente y el índice sin selección gris.
  Evidencia: [themes.md](themes.md).
- [ ] Validar manualmente con PDF reales/remotos que tildar los checkboxes no
  produce destello en el fondo, y revisar el parpadeo al ocultar/mostrar el índice
  y redimensionar. Diagnóstico opcional en [performance.md](performance.md).
- [ ] Validar manualmente la animación y el contraste de la carga con PDF grandes,
  unidades cloud, DPI altos y alto contraste.
- [ ] Validar manualmente la instalación, la asociación real de `.pdf`, la
  actualización y la desinstalación en otra PC, incluidos «Acerca de» y «Opciones».
- [ ] Validar manualmente los ajustes de zoom con PDF reales, ventanas extremas y
  DPI altos.
- [ ] Validar manualmente el zoom con Ctrl+rueda anclado al puntero y el clic en el
  indicador de página (Enter, Escape, pérdida de foco) con mouse/touchpad reales,
  a distintos DPI.
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
- [ ] Validar manualmente doble/triple clic de selección con PDF reales (acentos,
  CJK, RTL, columnas) y pegar el resultado en otra aplicación.
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
