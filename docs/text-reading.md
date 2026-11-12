# Selección y búsqueda — 2026-11-12

## Uso

Arrastrar con el mouse selecciona texto. La selección puede cruzar páginas y la
vista avanza al acercarse al borde superior/inferior. Ctrl+C copia texto Unicode;
Escape limpia la selección. Dentro del campo de búsqueda Ctrl+C conserva su
comportamiento habitual y copia el texto de ese campo.

Ctrl+F enfoca la búsqueda. Enter/F3 avanza; Shift+Enter/Shift+F3 retrocede. La
coincidencia se resalta y se desplaza la vista hasta su posición. Al modificar la
consulta se cancela la búsqueda anterior y se descarta su resaltado. Una selección
activa usa el resaltado azul para evitar superponerlo con el amarillo de búsqueda.

## Implementación y límites

PDFium sigue ejecutándose en un solo worker. Las posiciones de caracteres se
consultan al seleccionar, no al abrir o renderizar cada página. Las cajas se
transforman con FPDF_PageToDevice para respetar rotación y CropBox y se guardan en
coordenadas normalizadas, reutilizables al hacer zoom. Se limita la geometría a
200.000 caracteres por página, 16 MiB de caché y cuatro peticiones pendientes.

La copia consulta los permisos del documento antes de extraer texto, rechaza
resultados de documentos/selecciones anteriores y limita cada operación a un
millón de caracteres. Si falla no copia una selección parcial. Solo se escribe al
portapapeles tras Ctrl+C; no se guarda texto de documentos en disco durante el uso
normal. Las pruebas explícitas generan informes y bitmaps en test-results, ignorado
por Git. La extracción sigue el orden que entrega PDFium: columnas o diseños
inusuales pueden producir un orden distinto al esperado. No se añade OCR.

## Verificación ejecutada

- Geometría: carácter cercano, cambio de línea, página sin texto y unión de cajas.
- Ejecutable real, oculto: buscar Chucao, comprobar cajas, seleccionar/capturar el
  texto de la palabra, buscar hacia atrás con vuelta al documento, seleccionar entre
  páginas en ambos sentidos y comparar exactamente los resultados.
- Unicode: el fixture contiene café y dos líneas; la copia entre páginas conserva
  el acento y los saltos de línea.
- Repetición con páginas rotadas 90 grados y CropBox: búsqueda, hit testing y copia.
  Las pruebas usan el índice encontrado, sin asumir el orden de extracción al rotar.
- Imágenes de la propia función de pintura revisadas: selección azul y búsqueda
  amarilla alineadas en páginas normales y rotadas, sin interactuar con el escritorio.
- Regresión de apertura, renderizado, geometría de páginas y navegación continua.

La función de prueba consume los resultados antes de escribir al portapapeles, para
no alterar datos del usuario. Pendientes de comprobación manual: pegar en otra
aplicación, comportamiento del arrastre físico/autoscroll y documentos con copia
restringida. Contraseñas e impresión mantienen las limitaciones de validación
indicadas en el plan. No se inspeccionaron visualmente controles de Windows en las
imágenes de prueba: son una pintura fuera de pantalla del contenido del visor.
