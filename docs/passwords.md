# Contraseñas y permisos — 2026-11-13

El lector distingue el primer pedido de contraseña de un intento incorrecto. El
campo está enmascarado; Enter confirma y Cancelar/Escape/cerrar la ventana permiten
abandonar la apertura. Después de cancelar puede abrirse otro PDF normalmente.

Las contraseñas no se guardan en archivos de preferencias, informes ni logs. Los
buffers propios UTF-16 y UTF-8 del worker se sobrescriben al terminar el intento;
esto no pretende garantizar el borrado de todas las copias internas de Windows,
la biblioteca estándar o PDFium. El campo del diálogo se vacía antes de destruirlo.

Al abrir un documento, la UI conoce sus permisos de copia e impresión. Un archivo
restringido informa el motivo antes de intentar copiar o abrir el diálogo de
impresora. El worker vuelve a comprobar los permisos antes de extraer o imprimir.

## Pruebas ejecutadas

Los cinco archivos en tests/fixtures/passwords son sintéticos, con contraseñas
públicas de prueba. No contienen documentos ni claves del usuario. Se incluyen en
el repositorio para ejecutar pruebas en ambas PC sin descargar bibliotecas Python.
Su generador opcional usa pypdf y cryptography, aislados del lector.

`password-test.exe` verifica:

- AES-256, AES-128 y RC4 como compatibilidad con PDF antiguos.
- Contraseña ausente, vacía, incorrecta y correcta.
- Contraseña Unicode con acento y caracteres chinos.
- Apertura de páginas, búsqueda y renderizado no vacío después de autenticar.
- Restricciones de copia/impresión con la clave de lector y permisos con la de propietario.

`benchmark-open.ps1 -Passwords`, integrado en test-reader.ps1, realiza 22
comprobaciones en el ejecutable real con ventanas ocultas:

- Entrada vacía e incorrecta, aviso de error y reintento correcto.
- Autenticación Unicode y campo de contraseña enmascarado.
- Cancelación por botón, Escape y cierre, y apertura posterior de un PDF sin clave.
- Conservación del estado habilitado de la ventana propietaria.
- Rechazo de copia por el worker y de impresión por UI y worker en un archivo restringido.

La automatización envía mensajes a los controles creados por el propio programa;
no muestra diálogos, no toma foco ni simula entrada en el escritorio del usuario.
No toca su portapapeles ni crea trabajos en el spooler. Los informes contienen
contadores y tiempos, nunca las claves ingresadas.

Pasó también la regresión de navegación, índice, enlaces y selección normal/rotada.
Pendiente: validación manual del diálogo visible, teclado/IME, pegado en otras
aplicaciones y pruebas de impresoras físicas/virtuales. Los controles de permisos
probados no equivalen a haber validado una impresión real.
