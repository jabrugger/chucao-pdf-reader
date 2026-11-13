# Enlaces internos — 2026-11-13

Clic en un enlace interno lleva a la página y, para destinos XYZ, a la posición
indicada. Se conserva el zoom actual. Alt+Izquierda vuelve al punto de lectura
anterior, incluyendo zoom y desplazamiento horizontal. El historial guarda hasta
64 posiciones y se limpia al abrir otro documento.

La detección de enlaces se solicita al pasar el mouse por una página, con hasta
dos solicitudes pendientes. No se recorren los enlaces de todo el PDF al abrir.
Las áreas reconocidas muestran un cursor de mano y se conservan para hasta 16
páginas. Cada enumeración está limitada a 1024 anotaciones. Un clic vuelve a
resolver el destino con PDFium, respetando el orden de superposición de anotaciones.

Todas las llamadas siguen dentro del worker de PDFium. La UI recibe geometría
normalizada y destinos simples. Las conversiones respetan rotación y CropBox. No
se ejecutan acciones: se resuelven solo destinos directos, nombrados y GoTo dentro
del mismo documento. URI, archivos externos, Launch y JavaScript quedan inactivos.
Al hacer clic en una acción no compatible se indica en la barra de estado.

Arrastrar sigue seleccionando texto, incluso sobre un enlace. Los saltos pendientes
se descartan al cambiar de documento o tras otra interacción de navegación. No se
agregan botones ni paneles a la interfaz para esta función.

## Verificación

- `links-test.exe` usa los mismos helpers que el worker: geometría, hit testing,
  destinos directos y nombrados, posición XYZ, cancelación y acciones inactivas.
- Repite los casos con páginas rotadas y recortadas. Compara la posición de destino
  transformada nuevamente a coordenadas PDF, no solo el número de página.
- `benchmark-open.ps1 -Links` recorre el ejecutable oculto: cursor lógico/hit testing
  a zoom 300% con desplazamiento horizontal, salto, regreso a la posición anterior,
  rechazo de resultados de documentos/interacciones anteriores, enlace externo
  inactivo y reapertura con limpieza de caché e historial.
- `test-reader.ps1` integra estas pruebas con las de índice, navegación y texto.
  No se abre un navegador, no se imprimen documentos ni se modifica el portapapeles.

Pendiente de comprobación manual: sensación del cursor y del clic físico, regreso
con teclado y enlaces de documentos reales. Los modos Fit/FitH/FitV y otros distintos
de XYZ llevan por ahora al inicio de la página conservando el zoom; no se aplica el
factor de zoom del destino. Los enlaces web aún no están habilitados.
