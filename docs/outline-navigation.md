# Navegación desde el índice — 2026-11-15

Un clic o Enter en una entrada del índice lleva a su destino dentro del PDF.
Los destinos XYZ respetan sus coordenadas, incluidas rotación y CropBox, y aplican
su zoom cuando es positivo; cero o null conserva el zoom del lector.
Alt+Izquierda vuelve al punto anterior con su zoom y posición
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

## Modos de ajuste

La misma aplicación de destinos se usa para marcadores y enlaces internos:

| Destino | Resultado |
| --- | --- |
| XYZ | Posición indicada y zoom positivo; cero/null conserva el zoom |
| Fit | Página completa, con margen de 24 píxeles por lado |
| FitH | Ancho de la página y coordenada superior indicada; sin coordenada conserva la posición vertical |
| FitV | Alto de la página y coordenada izquierda indicada; sin coordenada conserva la posición horizontal |
| FitR | Rectángulo indicado, con margen de 24 píxeles por lado |
| FitB | Caja de contenido de la página completa |
| FitBH | Ancho de la caja de contenido y coordenada superior indicada; sin coordenada conserva la posición vertical |
| FitBV | Alto de la caja de contenido y coordenada izquierda indicada; sin coordenada conserva la posición horizontal |

Los ajustes se calculan para el área de lectura al activar el destino. El zoom se
limita al intervalo del lector, 25–400%; las páginas extremas pueden no caber
completas cuando se alcanza un límite. El presupuesto de renderizado sigue vigente.
Fit, FitB y FitBH centran horizontalmente la página; FitR, FitV y FitBV anclan su
borde izquierdo, y todos se sitúan al inicio vertical de la vista continua.
Alt+Izquierda recupera el zoom y la posición anteriores al salto.

La caja de contenido se obtiene con `FPDF_GetPageBoundingBox` al activar el destino;
si la página no informa una caja válida se usa la página completa. El rectángulo de
FitR se acota a los límites de la página. Cuando FitH/FitBH omiten su coordenada
superior, o FitV/FitBV su coordenada izquierda, el lector conserva el desplazamiento
actual en ese eje en lugar de saltar al borde. El estado «omitida» se lee de
`FPDFDest_GetView`, que informa el parámetro solo si está presente en el arreglo: una
coordenada ausente queda sin resolver y conserva la posición. Un valor null no se
distingue de un cero explícito (la API numérica lo informa como 0), así que ambos caen
en el borde; solo la coordenada ausente conserva la posición. No se aplica el mismo
tratamiento a XYZ, donde `FPDFDest_GetLocationInPage` sí informa qué campos se
omitieron y la coordenada omitida usa el borde superior/izquierdo visual.

## Verificación

- `outline-test.exe`: metadatos XYZ, resolución de la página y transformación
  de regreso a coordenadas PDF; coordenadas X/Y omitidas, rotación/recorte y
  destinos inválidos. Verifica FitR contra su rectángulo, FitB/FitBH/FitBV contra
  la caja de contenido y el zoom resultante de los cuatro modos. Verifica además
  que FitH/FitV/FitBH/FitBV sin coordenada marquen el eje a conservar y que un
  cero explícito (y un null, indistinguible) no se traten como omitidos. Conserva
  las pruebas de límites, ciclos, cancelación, títulos Unicode y acciones inactivas.
- `benchmark-open.ps1 -Outline`, con `outline.pdf` y `rotated-outline.pdf`:
  controles nativos del árbol, activación de una entrada y Enter, navegación asincrónica a XYZ
  con zoom 300%, regreso por el historial, entradas inactivas, descarte de resultados
  atrasados y reapertura con limpieza del árbol. Tres comprobaciones por ejecución.
- Las pruebas de enlaces internos/web verifican que el helper compartido conserve
  su comportamiento anterior. Todo se ejecuta con ventanas ocultas y PDF sintéticos.
- `benchmark-open.ps1 -Destinations`: variantes de Fit, FitH, FitV, XYZ (zoom
  200% y cero), FitR, FitB, FitBH y FitBV, incluidas las de coordenada cero
  explícita (FitH/FitV/FitBH/FitBV 0), repetidas con rotación y CropBox. Comprueba
  las dimensiones resultantes, el zoom, regreso, entrada inactiva y reapertura.
  `outline-test.exe` verifica además los cálculos con un área conocida, los límites
  y los ejes a conservar de las coordenadas omitidas.

El clic único y los accesos del panel integrado se verificaron con un fixture en
la ventana visible. Pendiente manual: marcadores de documentos reales, percepción
de la posición de llegada y escalado a distintos DPI.
