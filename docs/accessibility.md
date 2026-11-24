# Accesibilidad y teclado — 2026-11-18

La interfaz se puede recorrer sin mouse. Los controles propios conservan el foco
visible cuando se llega con Tab y lo ocultan al hacer clic, de modo que el anillo
de foco no aparece por accidente con el puntero.

## Orden de tabulación

La ventana principal usa `WS_EX_CONTROLPARENT` y el bucle de mensajes llama a
`IsDialogMessage`, así que Tab y Mayús+Tab recorren los controles visibles en el
orden de creación:

1. Abrir
2. Índice
3. Buscar
4. Página anterior / siguiente
5. Alejar / acercar
6. Ajustar al ancho
7. Imprimir
8. Tema
9. Acerca de

El panel del índice se agrega como contenedor y el árbol recibe el foco con sus
flechas, Enter para navegar y Escape/Ctrl+B para plegarlo. Su cuadro **Filtrar el
índice** se recorre con Tab; Enter va a la primera coincidencia, Escape limpia el
filtro y la flecha abajo pasa al árbol. Los controles de la búsqueda inferior
aparecen en el recorrido solo cuando la barra está visible.

## Atajos

| Tecla | Acción |
| --- | --- |
| Tab / Mayús+Tab | Recorrer los controles de la barra |
| F6 | Alternar entre el documento, el índice y la barra |
| Espacio / Mayús+Espacio | Avanzar/retroceder una pantalla en el documento |
| `+`, `=` / `-` | Acercar/alejar cuando el foco está en el documento o el índice |
| Ctrl+rueda | Zoom |
| Home / End, Page Up/Down, flechas | Navegación continua |
| Alt+Izquierda | Volver al punto anterior |
| Escape | Cerrar la búsqueda; en la barra, devolver el foco al documento |
| Tecla Menú / Mayús+F10 | Abrir el menú contextual de copia sobre el documento |
| Alt+Home | Volver a la pestaña inicial de recientes sin cerrar documentos |

Los atajos existentes (Ctrl+O, Ctrl+B, Ctrl+F, Ctrl+P, Ctrl+C, Ctrl+0, Ctrl+1,
Ctrl+2, F1, F3, Mayús+F3) se mantienen. Ctrl+0 ajusta al ancho, Ctrl+1 a la página
completa y Ctrl+2 al alto. En la bienvenida, las flechas recorren la lista de
recientes y Enter abre la entrada elegida. El menú contextual expone «Copiar» con
su atajo Ctrl+C; si no hay selección, el ítem aparece deshabilitado.

La **tabla completa de atajos** está en **Acerca de** (F1), debajo de los datos de
la versión. Incluye Ctrl+O/Ctrl+T para abrir, Ctrl+W y Ctrl+Tab para pestañas,
Ctrl+0/1/2 para ajuste, el menú contextual y la navegación con teclado.

## Nombres accesibles

Los botones propios son `BS_OWNERDRAW` con texto de ventana descriptivo, por lo
que exponen un nombre a MSAA. Además se fijan nombres explícitos con
`IAccPropServices`, sin los atajos entre paréntesis, para el botón Abrir, el
índice, la búsqueda, la paginación, el zoom, la impresión, el tema y Acerca de.
El campo de búsqueda recibe el nombre «Buscar en el documento», porque el texto
de sugerencia no es un rótulo accesible. El campo del indicador de página recibe
el nombre «Ir a la página».

Si `IAccPropServices` no está disponible, el lector sigue funcionando: el nombre
del campo de búsqueda simplemente queda vacío.

### Indicador de página clickeable

El indicador «N / M» de la barra se activa con un clic: aparece un campo EDIT
numérico con el valor actual preseleccionado. Enter va a la página; Escape o
perder el foco cancela y deja el indicador como estaba. El campo es un EDIT hijo
(`WS_TABSTOP` + `ES_NUMBER`) oculto hasta el clic, así que no altera el orden de
tabulación de los once botones mientras no está activo; cuando se muestra, Tab
sigue el recorrido normal y su pérdida de foco lo cierra. Tiene nombre accesible
«Ir a la página» y sobre el indicador el cursor cambia a mano para indicar que es
clickeable.

## Estado para lectores de pantalla

La barra de estado es un control `STATIC` real, no solo texto dibujado. Cuando el
mensaje cambia (página, búsqueda, error, copia, impresión) se actualiza su texto y
se emite `NotifyWinEvent(EVENT_OBJECT_NAMECHANGE, …)` una sola vez por cambio, de
modo que un lector de pantalla puede anunciarlo. El nombre accesible del control es
su propio texto; es visible, no tiene `WS_TABSTOP` y usa la misma fuente, color y
fondo que el resto de la barra en ambos temas y en alto contraste.

El texto se sincroniza desde `Redraw()` y `PageStatus()`, que cubren los cambios
normales de la interfaz. Los mensajes idénticos repetidos no generan eventos.

## Alto contraste

Cuando Windows tiene el **modo de alto contraste** activo, Chucao reemplaza su
paleta clara/oscura por los colores del sistema (`COLOR_WINDOW`, `COLOR_WINDOWTEXT`,
`COLOR_HIGHLIGHT`, `COLOR_3DSHADOW`). Los fondos de hover/activo se aplanan para
que el acento siga siendo el único indicador y nunca pierda contraste, y se
desactivan la barra de título oscura y el tema `DarkMode_Explorer`, que no aplican
en ese modo.

- La detección usa `SPI_GETHIGHCONTRAST` y se reevalúa al recibir
  `WM_SETTINGCHANGE`, `WM_SYSCOLORCHANGE` o `WM_THEMECHANGED`, sin cambiar la
  preferencia guardada del usuario.
- El indicador de alto contraste se protege contra reentrada (`applyingPalette`)
  porque cambiar el tema de una ventana puede devolver `WM_THEMECHANGED`.

`--benchmark-highcontrast <informe>` (integrado en `test-reader.ps1`) fuerza la
paleta de alto contraste, comprueba que barra, texto, acento, lienzo y hoja usen
los colores del sistema y guarda una captura. `--force-highcontrast` permite
generar capturas para revisión visual sin alterar el perfil.

## Verificación

`--benchmark-keys <informe>` (integrado en `test-reader.ps1`) comprueba:

- que el recorrido de `GetNextDlgTabItem` pase por los once controles visibles y
  vuelva al primero, y que todos tengan `WS_TABSTOP`;
- que los nombres MSAA de la búsqueda, el tema y la página anterior sean los
  esperados;
- que la barra de estado sea visible y que su nombre accesible cambie al cambiar
  el mensaje;
- que F6 alterne el foco entre el documento y el primer botón;
- que `IsDialogMessage` mueva el foco con Tab del botón Abrir al botón Índice;
- que se genere una captura de la interfaz con foco.

Se revisaron además las capturas de ambos temas y de 96/144/192 DPI. Los iconos
vectoriales se dibujan con GDI+ y `SmoothingModeAntiAlias`, con extremos y uniones
redondeados, para evitar los trazos escalonados del dibujo GDI. Los botones
propios interceptan `WM_PAINT`, `WM_NCCALCSIZE` y `WM_NCPAINT` para que el control
`BUTTON` no agregue su marco 3D nativo, que se veía como un borde claro en el
tema oscuro. El estado presionado, el foco y el hover se calculan en el subclass.

## Botón en la barra de tareas al arrancar

En Windows 11 la ventana principal podía quedar detrás de otras y sin botón en la barra
de tareas hasta que se activaba (o se ocultaba y volvía a mostrar), por lo que parecía
que la aplicación se cerraba sola, típicamente al abrirla desde el instalador. Tras
mostrar la ventana, Chucao pide el primer plano y, si Windows lo rechaza, la oculta y la
muestra una vez más para que el shell registre el botón. Se verificó leyendo los botones
de la barra con UI Automation: no aparecía en ningún lanzamiento antes del cambio y
aparece en todos después, con la compilación y con el EXE del ZIP. Se desconoce la causa
de fondo; el cambio es un rodeo. Pendiente: repetir el flujo del instalador y probar en
otra PC.

## Pendiente manual

- Narrator/NVDA: anuncio del nombre de los botones, del campo de búsqueda, del
  cambio de documento y de la barra de estado; ajustar la verbosidad si resulta
  ruidosa al recorrer páginas.
- Modo de alto contraste real del sistema: legibilidad con cada tema de alto
  contraste de Windows y con el modo de alto contraste activo desde el arranque.
- Anuncios de la barra de estado (resultado de búsqueda, página, errores) como
  región viva.
- Indicador de página clickeable: validación manual del clic, Enter, Escape y
  pérdida de foco con mouse y teclado reales, a distintos DPI y con Narrator/NVDA.
- IME, teclados alternativos, touch y navegación completa solo con teclado en
  documentos reales.
