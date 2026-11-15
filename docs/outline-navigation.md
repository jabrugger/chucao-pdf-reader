# Navegación desde el índice — 2026-11-15

Enter o doble clic en una entrada del índice lleva a su destino dentro del PDF.
Los destinos XYZ respetan sus coordenadas, incluidas rotación y CropBox, y conservan
el zoom del lector. Alt+Izquierda vuelve al punto anterior con su zoom y posición
horizontal. El historial es compartido con los enlaces internos y conserva hasta
64 posiciones. Las entradas sin destino interno no producen un salto ni abren
aplicaciones externas.

## Implementación y límites

La lectura del índice guarda metadatos simples del destino, sin cargar sus páginas
ni enviar handles de PDFium a la interfaz. La entrada del árbol identifica al
marcador, de modo que distintos marcadores de la misma página pueden tener
coordenadas diferentes. Solo al activar uno se carga la página elegida en el worker.

Índice y enlaces internos comparten la conversión de coordenadas. Una coordenada
XYZ omitida usa el borde superior/izquierdo visual de la página; coordenadas fuera
de sus límites se acotan. El resultado se descarta si pertenece a otro documento
o si una interacción de navegación posterior lo dejó obsoleto. El salto no bloquea
la interfaz mientras se carga la página destino.

Los destinos Fit/FitH/FitV y otros modos de ajuste siguen llevando al inicio de la
página con el zoom actual. Tampoco se aplica todavía el zoom especificado en XYZ.
Esas mejoras siguen abiertas en [TODO.md](TODO.md).

## Verificación

- `outline-test.exe`: metadatos XYZ, resolución de la página y transformación
  de regreso a coordenadas PDF; coordenadas X/Y omitidas, rotación/recorte y
  destinos inválidos. Conserva las pruebas de límites, ciclos, cancelación,
  títulos Unicode y acciones inactivas.
- `benchmark-open.ps1 -Outline`, con `outline.pdf` y `rotated-outline.pdf`:
  controles nativos del árbol, doble clic y Enter, navegación asincrónica a XYZ
  con zoom 300%, regreso por el historial, entradas inactivas, descarte de resultados
  atrasados y reapertura con limpieza del árbol. Tres comprobaciones por ejecución.
- Las pruebas de enlaces internos/web verifican que el helper compartido conserve
  su comportamiento anterior. Todo se ejecuta con ventanas ocultas y PDF sintéticos.

Pendiente manual: clic y teclado en la ventana visible, marcadores de documentos
reales, percepción de la posición de llegada y escalado a distintos DPI.
