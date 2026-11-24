# Temas claro y oscuro — 2026-11-18

Chucao tiene dos paletas completas para la interfaz: clara (predeterminada) y
oscura. El botón de tema de la barra superior abre **Opciones**, donde se elige
claro, oscuro o **igual que el sistema**; **Ctrl+D** alterna directamente entre
claro y oscuro. La elección se guarda para el usuario actual y se restaura al
abrir el lector.

Las páginas del PDF se dibujan siempre sobre blanco: el documento es el
contenido y no cambia con el tema. Sí cambian la barra, el índice, la búsqueda
inferior, la bienvenida, la barra de estado, los botones y los diálogos propios.
El diálogo de contraseña conserva el aspecto nativo de Windows.

## Persistencia

La preferencia se guarda en:

`%LOCALAPPDATA%\ChucaoPDFReader\settings.dat`

El archivo contiene `theme=light`, `theme=dark` o `theme=system`, junto con las
preferencias `restore`, `outline` y la posición de la ventana descritas en
[preferences.md](preferences.md).
Se escribe con un temporal y un reemplazo atómico, y nunca incluye rutas, contenido
del PDF ni contraseñas. Si falta, está corrupto o no se puede leer, se usa el tema
claro sin interrumpir la apertura. Para volver al valor predeterminado alcanza con
borrarlo o alternar con Ctrl+D.

## Implementación

- `src/ui-icons.h` centraliza la paleta (`LightPalette`, `DarkPalette`) y los
  glifos vectoriales, incluidos luna y sol. Los controles propios consultan la
  paleta activa en cada dibujo.
- Los controles nativos reciben los colores del tema: el árbol del índice usa
  `TreeView_SetBkColor`/`SetTextColor` y `DarkMode_Explorer` cuando está
  disponible; las barras de desplazamiento y el borde de la ventana usan el
  tema oscuro del sistema.
- La barra de título intenta `DWMWA_USE_IMMERSIVE_DARK_MODE` (atributos 19 y 20).
  Si Windows no lo admite, se conserva la barra nativa sin fallar.
- La imagen de bienvenida tiene fondo blanco; en tema oscuro se dibuja sobre una
  tarjeta redondeada clara para que no parezca un recorte accidental. Los iconos
  de la barra son vectoriales y se adaptan a la paleta activa.

## Opciones en tema oscuro

El selector de tema es un control segmentado propio (**Sistema / Claro / Oscuro**)
dibujado con la paleta activa, no un `COMBOBOX` nativo. Las casillas son botones
ownerdraw: caja `#475063` con borde `#4C566A` sin marcar, relleno de acento con
tilde oscura al marcar y anillo de foco alrededor de la caja. Los enlaces son
botones ownerdraw con el color de acento. El estado de cada casilla se guarda en
el control (`GWLP_USERDATA`) porque `BS_OWNERDRAW` no conserva `BM_SETCHECK`.

La barra de título usa los colores DWM de Windows 11 (`DWMWA_CAPTION_COLOR`,
`DWMWA_TEXT_COLOR`, `DWMWA_BORDER_COLOR`) con `bar`, `ink` y `line`; en tema claro
o alto contraste se restablece el valor nativo. El logo de bienvenida usa el PNG
con transparencia recortado a la tarjeta redondeada, sin el cuadrado blanco del
BMP anterior. Las barras nativas se marcan con `DarkMode_Explorer`.

Al cambiar de tema, los controles ownerdraw de la ventana principal reciben de
nuevo el tema vacío (`SetWindowTheme(..., "", "")`) para que Windows no dibuje un
borde nativo al pasar el mouse; al cerrar Opciones el foco vuelve al documento
para no dejar el rectángulo de foco en el engranaje. Los checkboxes no reaplican
la paleta ni repintan la ventana principal: solo guardan flags y settings.dat.

## Verificación

- `--benchmark-theme <perfil> <informe>` (usado por `test-reader.ps1`) escribe un
  perfil corrupto, comprueba que el lector cae en tema claro, alterna a oscuro,
  verifica el archivo guardado, guarda una captura oscura y vuelve a claro.
- `--theme-dark` junto con `--preview-home`, `--preview-about`,
  `--benchmark-interface`, `--benchmark-outline` u otros diagnósticos genera
  capturas con el tema oscuro. Los diagnósticos no leen ni escriben el perfil
  real del usuario.
- Se revisaron las capturas de bienvenida, Acerca de, índice y lectura con
  búsqueda e índice fijado en ambos temas.

- Línea clara de 1 px en el borde superior e izquierdo de cada botón de la barra y del
  panel Contenido (tema oscuro): el fondo de `DrawUiIcon` se rellenaba con un rectángulo
  GDI+ suavizado, que cubre solo parte de la primera fila y columna. Ahora se rellena con
  GDI. Las capturas de diagnóstico no la mostraban porque no pasan por la composición real;
  se verificó capturando la ventana real con `PrintWindow`. También se acortó el recorte del
  logo de bienvenida (216 px) para excluir el borde claro del PNG en su lado derecho e inferior.

Pendiente de validación manual: contraste con monitores a 150–200 %, modo de alto
contraste de Windows, cambio de tema con varias ventanas abiertas, apariencia de
las barras de desplazamiento en versiones antiguas de Windows 10 y seguimiento
real de los cambios del tema de Windows. «Igual que el sistema» ya está disponible
en Opciones; Ctrl+D selecciona un tema explícito. Ver [preferences.md](preferences.md).
