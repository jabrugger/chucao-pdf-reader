# Opciones y restauración de sesión

El icono de la barra (antes solo alternaba el tema) abre **Opciones de Chucao**.
Ctrl+D sigue alternando el tema directamente.

- **Tema preferido**: «Igual que el sistema» (sigue el modo de aplicaciones de
  Windows y se actualiza si cambia), claro u oscuro; se aplica al instante.
  Ctrl+D fija un tema explícito.
- **Mostrar automáticamente el índice**: activado por defecto (como antes). Si se
  desmarca, el índice no se fija al abrir un PDF con marcadores y aparece al acercar
  el puntero al borde izquierdo o con Ctrl+B. Rige desde el siguiente documento.
- **Reabrir los PDF de la última sesión**: desactivado por defecto. Al activarlo,
  al iniciar sin indicar un archivo se reabren las pestañas y la activa.
  Los PDF protegidos vuelven a pedir contraseña; no se guarda ninguna.
- **Abrir los PDF con Chucao**: registra la aplicación como lector de PDF en el
  usuario actual. Windows 10/11 puede mantener otra aplicación por `UserChoice`;
  en ese caso el aviso indica usar el enlace **Abrir Configuración de aplicaciones
  predeterminadas**. Al desmarcar se quitan solo las entradas de Chucao.
- **Olvidar recientes y posiciones guardadas**: además del historial, vacía la
  sesión guardada. Si la opción de reabrir sigue activa, al cerrar se vuelve a
  guardar lo que haya abierto en ese momento.

## Tamaño y posición de la ventana

Chucao recuerda el tamaño y la posición de la ventana, incluido si estaba
maximizada, y los restaura al abrir. Se guarda el rectángulo normal (el que se
recupera al desmaximizar) y el estado maximizado por separado. Al iniciar, si el
rectángulo ya no cae en ningún monitor (por ejemplo, se desconectó una pantalla),
se ignora y se usa el tamaño inicial.

Se guarda al cerrar normalmente, al recibir el aviso de fin de sesión de Windows y
al cambiar cualquier opción; el diálogo de Opciones conserva la posición actual.
Las coordenadas se guardan en píxeles físicos: si cambia la escala del monitor
entre sesiones, el tamaño puede verse distinto en unidades lógicas, pero la ventana
sigue siendo válida y acotada por el tamaño mínimo del lector.

## Almacenamiento

`%LOCALAPPDATA%\ChucaoPDFReader\`:

- `settings.dat`: `theme=light|dark|system`, `restore=0|1`, `outline=0|1` y,
  cuando corresponde, `window=left top right bottom max`. Los archivos anteriores
  sin `restore` se leen con la restauración desactivada, sin `outline` con el índice
  automático activado y sin `window` con el tamaño inicial.
- `last-session.dat`: cabecera `ChucaoSession 1`, índice de la pestaña activa y
  una ruta UTF-8 por línea. Máximo 64 rutas y 1 MiB; escritura atómica.
  Un archivo dañado se ignora por completo. La sesión se guarda al cerrar, al
  terminar la sesión de Windows, al cambiar las opciones y al abrir, activar o
  cerrar pestañas.

## Verificación

`scripts/test-preferences.ps1` (incluida en `test-reader.ps1`): ida y vuelta,
Unicode, límites, corrupción y borrado (`tools/preferences-test.cpp`), más tres
ejecuciones del EXE: escribir la sesión, restaurarla y no restaurar con la opción
apagada. `--benchmark-window` (también en la suite general) comprueba restaurar un
rectángulo guardado, guardar el actual, ignorar una posición fuera de todo monitor
y conservar el indicador de maximizada. No cubre el diálogo con mouse/teclado ni
lectores de pantalla.

Pendiente manual: mover/redimensionar la ventana entre monitores reales, maximizar,
cerrar y reabrir, y comprobar escalas de DPI distintas.
