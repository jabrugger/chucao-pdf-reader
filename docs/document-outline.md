# Índice del documento

El índice es un panel integrado a la izquierda. Se muestra automáticamente cuando
el PDF tiene marcadores y se mantiene cerrado si no los tiene. El botón del índice,
Ctrl+B o el botón de plegado permiten ocultarlo y volver a abrirlo. Acercar el mouse
al borde izquierdo muestra una vista temporal sobre el documento; al alejarlo se
retira. La chincheta permite alternar entre fijo y temporal. Elegir un capítulo
desde la vista temporal fija el panel para seguir navegando.

Un clic o Enter lleva al destino. Las flechas recorren y expanden el árbol.
Escape lo pliega cuando el foco está en el índice; si la búsqueda está abierta,
Escape cierra primero esa barra. El panel se limpia al cambiar de documento.
No se genera un índice a partir del texto. Si se pide explícitamente el índice de
un PDF sin marcadores, se informa que el documento no tiene índice.

Los marcadores se solicitan después de encolar el primer render, en el mismo worker
que posee PDFium. No se recorren páginas ni se cargan sus dimensiones al leer el
índice. La UI recibe metadatos simples; no maneja objetos PDFium. Se descartan los
resultados de documentos anteriores. El panel fijo reserva ancho y conserva el
punto de lectura; la vista temporal no cambia la escala ni solicita otro render.

Se admiten destinos directos, nombrados y acciones GoTo dentro del mismo documento.
Las coordenadas, ajustes y el historial de regreso se detallan en
[outline-navigation.md](outline-navigation.md). Entradas sin destino válido,
acciones web, archivos externos y JavaScript no se ejecutan desde el índice.

El recorrido comprueba cancelación entre entradas y referencias ya visitadas.
Se limita a 4096 entradas, 64 niveles y títulos de hasta 1023 unidades UTF-16.
Si alcanza un límite, muestra una indicación y conserva las entradas recuperadas.
Esto acota la memoria propia y el recorrido de Chucao, aunque una llamada individual
a PDFium puede tardar al leer un archivo remoto o dañado.

## Verificación

`scripts/test-reader.ps1` incluye fixtures sintéticos para jerarquías, acentos,
destinos nombrados y GoTo, destinos inválidos, acciones externas/JavaScript,
ausencia de índice, ciclos y límites de profundidad, cantidad y longitud de título.
`outline-test.exe` usa el mismo lector de índice que el worker.

El ejecutable real se prueba con ventanas ocultas: población del árbol, activación
de una entrada y Enter, rechazo de entradas sin destino, reapertura, descarte de resultados
anteriores y cambio a un documento sin índice. Se inspecciona una pintura local del
árbol, sin tomar control del escritorio. Sigue pendiente la validación manual
del aspecto con distintos DPI. El clic único, Ctrl+F, Escape y la vista temporal se
comprobaron también en una instancia visible con fixtures. Ver
[reader-interface.md](reader-interface.md) para la nueva disposición de controles.
