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
flechas, Enter para navegar y Escape/Ctrl+B para plegarlo. Los controles de la
búsqueda inferior aparecen en el recorrido solo cuando la barra está visible.

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

Los atajos existentes (Ctrl+O, Ctrl+Shift+O, Ctrl+B, Ctrl+F, Ctrl+P, Ctrl+C,
Ctrl+0, F1, F3, Mayús+F3) se mantienen.

## Nombres accesibles

Los botones propios son `BS_OWNERDRAW` con texto de ventana descriptivo, por lo
que exponen un nombre a MSAA. Además se fijan nombres explícitos con
`IAccPropServices`, sin los atajos entre paréntesis, para el botón Abrir, el
índice, la búsqueda, la paginación, el zoom, la impresión, el tema y Acerca de.
El campo de búsqueda recibe el nombre «Buscar en el documento», porque el texto
de sugerencia no es un rótulo accesible.

Si `IAccPropServices` no está disponible, el lector sigue funcionando: el nombre
del campo de búsqueda simplemente queda vacío.

## Verificación

`--benchmark-keys <informe>` (integrado en `test-reader.ps1`) comprueba:

- que el recorrido de `GetNextDlgTabItem` pase por los once controles visibles y
  vuelva al primero, y que todos tengan `WS_TABSTOP`;
- que los nombres MSAA de la búsqueda, el tema y la página anterior sean los
  esperados;
- que F6 alterne el foco entre el documento y el primer botón;
- que `IsDialogMessage` mueva el foco con Tab del botón Abrir al botón Índice;
- que se genere una captura de la interfaz con foco.

Se revisaron además las capturas de ambos temas y de 96/144/192 DPI. Los iconos
vectoriales se dibujan con GDI+ y `SmoothingModeAntiAlias`, con extremos y uniones
redondeados, para evitar los trazos escalonados del dibujo GDI. Los botones
propios interceptan `WM_PAINT`, `WM_NCCALCSIZE` y `WM_NCPAINT` para que el control
`BUTTON` no agregue su marco 3D nativo, que se veía como un borde claro en el
tema oscuro. El estado presionado, el foco y el hover se calculan en el subclass.

## Pendiente manual

- Narrator/NVDA: anuncio del nombre de los botones, del campo de búsqueda y del
  cambio de documento.
- Modo de alto contraste de Windows: la paleta propia todavía no se reemplaza por
  los colores del sistema.
- Anuncios de la barra de estado (resultado de búsqueda, página, errores) como
  región viva.
- IME, teclados alternativos, touch y navegación completa solo con teclado en
  documentos reales.
