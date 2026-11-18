# Enlaces internos y web — 2026-11-13

Clic en un enlace interno lleva a la página y, para destinos XYZ, a la posición
indicada. Un zoom XYZ positivo se aplica dentro de los límites del lector;
cero/null conserva el zoom. Fit, FitH, FitV, FitR y los modos de caja de contenido
FitB/FitBH/FitBV ajustan la vista según el destino.
Alt+Izquierda vuelve al punto de lectura
anterior, incluyendo zoom y desplazamiento horizontal. El historial guarda hasta
64 posiciones y se limpia al abrir otro documento.

La detección de enlaces se solicita al pasar el mouse por una página, con hasta
dos solicitudes pendientes. No se recorren los enlaces de todo el PDF al abrir.
Las áreas reconocidas muestran un cursor de mano y se conservan para hasta 16
páginas. Cada enumeración está limitada a 1024 anotaciones. Un clic vuelve a
resolver el destino con PDFium, respetando el orden de superposición de anotaciones.

Todas las llamadas a PDFium siguen dentro del worker. La UI recibe geometría
normalizada y destinos simples. Las conversiones respetan rotación y CropBox.
Los destinos directos, nombrados y GoTo se resuelven dentro del mismo documento.
Las acciones URI se admiten solo para HTTP/HTTPS; archivos externos, Launch,
JavaScript y otros protocolos quedan inactivos. Al hacer clic en una acción
no compatible se indica en la barra de estado.

## Enlaces web

Un clic sin arrastre sobre un enlace HTTP/HTTPS lo envía al navegador predeterminado
con `ShellExecuteW`, sin argumentos de línea de comandos. No se abre nada durante
la carga del PDF, al pasar el mouse, al buscar ni al seleccionar texto. Estos enlaces
también muestran el cursor de mano y no cambian la página ni el historial interno.
Si Windows no puede abrir el navegador, se informa en la barra de estado.

La URI se lee bajo demanda con un máximo de 8192 bytes, se valida como UTF-8 y se
rechazan NUL internos, controles, espacios sin codificar, comillas, barras inversas,
host vacío y credenciales dentro de la autoridad. Solo se admiten direcciones
absolutas con `http://` o `https://`, sin depender de la asociación de otros protocolos.
Las rutas relativas y `mailto:` quedan fuera de alcance por ahora. Los espacios
codificados como `%20`, consultas, fragmentos y UTF-8 válido son admitidos.
La UI vuelve a comprobar el protocolo antes de entregar la URL a Windows.
No se guardan las URL en el historial de documentos de Chucao; el navegador gestiona
su propio historial. Se respeta la acción URI cuando una anotación incluye también
un destino interno contradictorio, sin ejecutar acciones encadenadas del PDF.

Arrastrar sigue seleccionando texto, incluso sobre un enlace. Los saltos pendientes
se descartan al cambiar de documento o tras otra interacción de navegación. No se
agregan botones ni paneles a la interfaz para esta función.

## Verificación

- `links-test.exe` usa los mismos helpers que el worker: geometría, hit testing,
  destinos directos y nombrados, posición XYZ, cancelación, URI válidas y acciones
  inactivas. Incluye UTF-8 inválido, NUL, límites, protocolos no admitidos y rutas
  ambiguas, tanto con strings como con anotaciones PDF sintéticas.
- Repite los casos con páginas rotadas y recortadas. Compara la posición de destino
  transformada nuevamente a coordenadas PDF, no solo el número de página.
- `benchmark-open.ps1 -Links` recorre el ejecutable oculto: cursor lógico/hit testing
  a zoom 300% con desplazamiento horizontal, salto, regreso a la posición anterior,
  rechazo de resultados de documentos/interacciones anteriores, envío de un enlace
  web y reapertura con limpieza de caché e historial. Comprueba que enumerar enlaces
  no envíe URL y que resultados web atrasados no produzcan nuevas aperturas.
  En modo diagnóstico el envío se simula: se cuenta y verifica el destino sin
  llamar a Windows para abrir aplicaciones. Cinco comprobaciones por ejecución.
  Se ejecuta también con `rotated-links.pdf` para páginas rotadas y recortadas.
- `test-reader.ps1` integra estas pruebas con las de índice, navegación y texto.
  No se abre un navegador, no se imprimen documentos ni se modifica el portapapeles.

Pendiente de comprobación manual: sensación del cursor y del clic físico, regreso
con teclado y enlaces de documentos reales. Los ajustes FitR y FitB/FitBH/FitBV
están cubiertos por pruebas automatizadas; ver [ajustes y límites](outline-navigation.md).
También faltan la prueba manual del clic web y del navegador predeterminado,
incluida la respuesta ante un fallo de apertura.
