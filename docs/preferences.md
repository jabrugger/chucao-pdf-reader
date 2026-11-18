# Opciones y restauración de sesión

El icono de la barra (antes solo alternaba el tema) abre **Opciones de Chucao**.
Ctrl+D sigue alternando el tema directamente.

- **Tema preferido**: claro u oscuro; se aplica al instante.
- **Reabrir los PDF de la última sesión**: desactivado por defecto. Al activarlo,
  al iniciar sin indicar un archivo se reabren las pestañas y la activa.
  Los PDF protegidos vuelven a pedir contraseña; no se guarda ninguna.
- **Olvidar recientes y posiciones guardadas**: además del historial, vacía la
  sesión guardada. Si la opción de reabrir sigue activa, al cerrar se vuelve a
  guardar lo que haya abierto en ese momento.

## Almacenamiento

`%LOCALAPPDATA%\ChucaoPDFReader\`:

- `settings.dat`: `theme=` y `restore=0|1`. Los archivos anteriores sin `restore`
  se leen con la restauración desactivada.
- `last-session.dat`: cabecera `ChucaoSession 1`, índice de la pestaña activa y
  una ruta UTF-8 por línea. Máximo 64 rutas y 1 MiB; escritura atómica.
  Un archivo dañado se ignora por completo. La sesión se guarda al cerrar, al
  terminar la sesión de Windows, al cambiar las opciones y al abrir, activar o
  cerrar pestañas.

## Verificación

`scripts/test-preferences.ps1` (incluida en `test-reader.ps1`): ida y vuelta,
Unicode, límites, corrupción y borrado (`tools/preferences-test.cpp`), más tres
ejecuciones del EXE: escribir la sesión, restaurarla y no restaurar con la opción
apagada. No cubre el diálogo con mouse/teclado ni lectores de pantalla.
