# DPI por monitor — 2026-11-18

Chucao se declara *per-monitor DPI aware v2* cuando Windows lo admite. Al mover
la ventana a un monitor con otra escala, la barra, las fuentes, los iconos, el
índice, la búsqueda y los márgenes se recalculan; no se estira una captura
bitmap de la interfaz.

## Implementación

- `EnablePerMonitorDpi` intenta `SetProcessDpiAwarenessContext` con
  `PER_MONITOR_AWARE_V2` (resuelto dinámicamente para no exigir un SDK nuevo),
  luego `SetProcessDpiAwareness` de `shcore.dll` y por último
  `SetProcessDPIAware` como respaldo.
- `WindowDpi` usa `GetDpiForWindow` y cae a `GetDeviceCaps(LOGPIXELSX)` si la API
  no existe. `SystemDpi` hace lo equivalente para el tamaño inicial.
- Todas las medidas propias pasan por `D(valor)`, que escala desde 96 DPI. Esto
  incluye `BarHeight`, márgenes entre páginas, botones, barra de búsqueda, pie,
  bienvenida, panel del índice, alto de fila y sangría del árbol.
- Las fuentes se recrean con `CreateFonts` al cambiar de monitor; los controles
  se actualizan con `ApplyFonts`. La imagen de bienvenida y el icono de Acerca de
  se escalan al tamaño correspondiente.
- `WM_DPICHANGED` aplica el rectángulo sugerido por Windows, vuelve a maquetar y
  dispara un render nuevo con el ancho de vista actualizado.
- Los glifos vectoriales escalan su espacio de 20 × 20 y el grosor del trazo;
  no se cargan imágenes para los iconos.
- El zoom del documento sigue siendo independiente del DPI: la escala de la
  página no cambia, solo el tamaño de la interfaz que la rodea.

## Qué queda a cargo de Windows

Las barras de desplazamiento, el borde, la barra de título, los menús y el
diálogo de contraseña usan métricas del sistema. El diálogo de contraseña
escala sus controles con `WindowDpi`; el de Acerca de usa unidades de diálogo y
se adapta por el gestor nativo. El tema oscuro no cambia con el DPI.

## Verificación

`--benchmark-dpi <informe>` (integrado en `test-reader.ps1`) simula
`WM_DPICHANGED` a 144 y 192 DPI sobre una ventana aumentada, comprueba:

- que `dpi`, `BarPixels()`, `ViewWidth()` y `ViewHeight()` se actualicen;
- que la fuente normal tenga la altura esperada;
- que el botón Acerca de y el alto de fila del árbol escalen;
- que se generen capturas `.initial.bmp`, `.144.bmp` y `.192.bmp`.

Se revisaron las capturas a 96, 144 y 192: la barra deja de superponer botones,
el texto «Abrir» entra completo, el campo de página y el zoom mantienen su lugar
y la bienvenida se centra con el nuevo tamaño. La prueba se ejecuta con la
ventana oculta y no altera el perfil real.

## Validación manual pendiente

- Monitores físicos con escalas 125 %, 150 % y 200 %, y arrastre de la ventana
  entre ellos.
- Reinicio con la ventana guardada en un monitor secundario.
- Legibilidad de los textos y del resaltado a escalas fraccionarias.
- Interacción con el modo de alto contraste y con la configuración de tamaño de
  texto de Windows.
- Comparación de las barras de desplazamiento nativas en versiones antiguas de
  Windows 10.
