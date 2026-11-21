# Contribuir a Chucao

Chucao es un lector de PDF para Windows, de solo lectura, en C++/Win32 con PDFium.
Las ideas y correcciones son bienvenidas; antes de una mejora grande, abrí un issue.

## Compilar y probar

Requiere Visual Studio con las herramientas de C++ x64.

```powershell
powershell -ExecutionPolicy Bypass -File scripts/setup-pdfium.ps1
scripts\build.cmd
powershell -ExecutionPolicy Bypass -File scripts/test-reader.ps1
```

Revisá el código de salida de `test-reader.ps1` (0 = todo bien). Algunas pruebas
con ventana pueden fallar por tiempo si el equipo está muy cargado; repetí antes
de sospechar del cambio. Cierra `build\Chucao.exe` antes de compilar.

## Organización del código

`src/main.cpp` solo contiene el punto de entrada y modos de prueba. El resto son
fragmentos que comparten una unidad de compilación y se incluyen en este orden:

| Archivo | Contenido |
|---|---|
| `win-helpers.h` | Constantes, DPI, tema de ventana y utilidades |
| `about-dialog.h` | Diálogo Acerca de y atajos |
| `pdf-worker.h` | Hilo de PDFium y mensajes de resultado |
| `password-dialog.h` | Pedido de contraseña |
| `reader.h` | Clase `Reader`: pestañas, dibujo, entrada, índice |
| `settings-dialog.h` | Diálogo Opciones |
| `chrome-controls.h` | Controles de la barra, índice y lista de recientes |
| `window-proc.h` | Procedimiento de la ventana principal |

La lógica sin dependencias de Win32 (diseño de páginas, ajustes, sesión, estado
de lectura, destinos) vive en cabeceras propias con pruebas en `tools/`.

## Criterios

- Sin telemetría ni conexiones de red en el visor.
- Cambios de interfaz: probar tema claro, oscuro y alto contraste, y varios DPI.
- Textos de la interfaz en español.
- Código propio bajo licencia MIT. No copiar código de otros lectores de PDF.
