# Índice del documento

El botón Índice o Ctrl+B abre una ventana opcional con los capítulos del PDF.
Las flechas recorren y expanden el árbol; Enter o doble clic lleva a la página.
Escape, Ctrl+B o cerrar la ventana ocultan el índice. Se conserva hasta abrir
otro archivo. Los documentos sin marcadores muestran «Este documento no tiene
índice». No se genera un índice a partir del texto.

La carga ocurre al pedir el índice, en el mismo worker que posee PDFium. No se
recorren páginas ni se cargan sus dimensiones al abrir el PDF. La UI recibe solo
títulos, relaciones jerárquicas y números de página; no maneja objetos PDFium.
Al cambiar de documento se limpia el árbol y se descartan los resultados anteriores.

Se admiten destinos directos, nombrados y acciones GoTo dentro del mismo documento.
Se salta al comienzo de la página conservando el zoom del lector. Todavía no se
aplican las coordenadas ni el modo de ajuste indicado por el marcador. Entradas
sin destino válido, acciones web, archivos externos y JavaScript no se ejecutan.

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

El ejecutable real se prueba con ventanas ocultas: población del árbol, navegación
por doble clic, rechazo de entradas sin destino, reapertura, descarte de resultados
anteriores y cambio a un documento sin índice. Se inspecciona una pintura local del
árbol, sin tomar control del escritorio. Sigue pendiente la validación manual del
teclado y del aspecto con distintos DPI; la ventana es parte de la interfaz provisional.
