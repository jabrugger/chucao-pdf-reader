# Mensajes de error — 2026-11-18

Los mensajes visibles son breves, en una sola línea y sin códigos internos del
motor. La barra de estado conserva el último mensaje; los diálogos de contraseña
mantienen su propio texto. La meta es que cada fallo diga qué pasó y, cuando
corresponde, qué puede hacer el usuario.

## Apertura

La traducción de `FPDF_GetLastError` vive en `src/pdf-errors.h` y se comprueba en
`messages-test.exe` sin abrir documentos:

| Error del motor | Mensaje |
| --- | --- |
| `FPDF_ERR_PASSWORD` (sin intento previo) | Este documento está protegido. Ingresá la contraseña. |
| `FPDF_ERR_PASSWORD` (con intento previo) | La contraseña es incorrecta. Volvé a intentarlo. |
| `FPDF_ERR_FILE` | No se pudo abrir el archivo. Verificá que exista y que tengas permiso para leerlo. |
| `FPDF_ERR_FORMAT` | El archivo no es un PDF válido o está dañado. |
| `FPDF_ERR_SECURITY` | El PDF usa una protección que este lector no admite. |
| `FPDF_ERR_PAGE` | El PDF no tiene páginas válidas. |
| Otro | No se pudo abrir el PDF. Puede estar bloqueado por otra aplicación o no ser compatible. |

En la barra de estado los errores de apertura se prefijan con el nombre del
archivo intentado, porque el documento anterior puede seguir visible. Un PDF sin
páginas usa el mismo texto que `FPDF_ERR_PAGE`.

## Páginas y acciones

| Situación | Mensaje |
| --- | --- |
| Página no cargable | No se pudo leer la página N. El PDF puede estar incompleto. |
| Dimensiones inválidas | La página N tiene dimensiones no válidas. |
| Render fallido | No se pudo dibujar la página N. |
| Sin memoria para el bitmap | No hay memoria suficiente para dibujar la página N. |
| Impresión fallida | No se pudo completar la impresión. Revisá la impresora y la cola de impresión. |
| Historial no guardado | No se pudo guardar el cambio del historial. Se conserva el archivo anterior. |

Las restricciones de copia e impresión, el portapapeles ocupado, la selección sin
texto o demasiado grande y la falta de memoria al copiar ya tenían mensajes
específicos y se conservan. Los avisos de contraseña se mantienen separados del
resto porque el diálogo modal necesita distinguir el primer pedido de un reintento.

## Criterios

- No se muestran rutas completas ni detalles del motor en el diálogo de contraseña.
- No se promete una solución que el lector no puede ejecutar (por ejemplo, pedir
  permisos al autor); se indica la comprobación que el usuario sí puede hacer.
- No se registran contraseñas ni contenido del documento en los mensajes.
- Los textos no dependen de la longitud de la ruta: la barra usa elipsis.

## Verificación

`messages-test.exe` comprueba que cada código produzca un texto distinto y no
vacío, que el reintento de contraseña cambie el mensaje, que los errores
desconocidos sigan siendo accionables y que los fallos de página incluyan el
número correcto en base 1. La regresión de contraseñas y permisos sigue
verificando los textos exactos que usa el diálogo.

Pendiente de revisión manual: legibilidad de los mensajes a distintos DPI y con
nombres de archivo largos, y el comportamiento ante impresoras desconectadas o
spoolers bloqueados en la ventana visible.
