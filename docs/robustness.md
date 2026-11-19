# Robustez con PDF dañados y dimensiones extremas

`robustness-test.exe` genera sus propios archivos en una carpeta temporal y los
abre con el mismo PDFium del visor. No usa documentos del usuario ni escribe en el
perfil real.

## Archivos generados

| Archivo | Contenido | Comportamiento esperado |
| --- | --- | --- |
| `valid.pdf` | Página normal con texto | Abre y renderiza |
| `truncated.pdf` | PDF válido cortado a la mitad | Se rechaza con mensaje de formato/archivo |
| `garbage.pdf` | Texto que no es PDF | Se rechaza con mensaje |
| `empty.pdf` | Cero bytes | Se rechaza con mensaje |
| `missing-page.pdf` | Árbol de páginas que apunta a un objeto inexistente | Se rechaza o queda sin páginas, sin bloquear |
| `zero-box.pdf` | `/MediaBox [0 0 0 0]` | PDFium lo normaliza o lo rechaza; nunca genera un bitmap inválido |
| `huge.pdf` | Página de 5000 × 5000 puntos | Abre, se acota el bitmap y renderiza |
| `tiny.pdf` | Página de 1 × 1 punto | Abre y renderiza al menos 1 × 1 píxel |

Para los archivos dañados se comprueba que el proceso no se cae, que la cantidad de
páginas es coherente y que todo código de `FPDF_GetLastError` tiene un mensaje de
usuario no vacío mediante `PdfOpenErrorMessage`.

## Presupuesto de renderizado

El límite vive en `src/render-budget.h` y lo comparten el worker y la prueba:

- Lado máximo: **16 000 px**.
- Total máximo: **12 000 000 de píxeles** por bitmap.
- Entradas no positivas, no finitas o con desbordamiento devuelven un objetivo
  vacío, que el worker convierte en «La página N tiene dimensiones no válidas».

Además de las páginas reales, se prueban dimensiones sintéticas: 20 000 × 20 000,
100 000 × 100, zoom 100×, 1 × 1 con escala mínima, cero, negativo, NaN y valores
que desbordan el producto. En todos los casos el resultado queda dentro del
presupuesto o se rechaza.

## Verificación

```powershell
build\robustness-test.exe test-results\robustness
```

Está integrado en `scripts/test-reader.ps1` con una carpeta exclusiva por ejecución
y un límite de 30 segundos. No abre ventanas, no imprime y no toca archivos del
usuario.

Pendiente manual: PDF dañados reales, páginas escaneadas enormes, archivos remotos
con I/O lento, mensajes en la ventana visible, copia/impresión restringida y
comportamiento al cerrar durante una lectura bloqueante.
