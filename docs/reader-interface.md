# Interfaz de lectura — 2026-11-15

Primera propuesta funcional según las preferencias del usuario. Conserva Win32/GDI
y PDFium; no incorpora un framework visual ni dependencias gráficas nuevas.

## Controles

- Barra superior de iconos vectoriales para abrir/recientes, página anterior y
  siguiente, zoom, ajuste al ancho, índice, búsqueda e impresión. Cada botón tiene
  nombre accesible, ayuda al pasar el mouse y foco visible al recorrerlo con Tab.
- Búsqueda inferior a demanda con Ctrl+F o la lupa. Consulta incremental con espera
  de 220 ms, anterior/siguiente, indicador de resultado y cierre con Escape o ×.
  Cerrar cancela la búsqueda pendiente, limpia el resaltado y conserva la consulta.
- Índice como panel hijo de la ventana principal. Se abre al detectar marcadores;
  permanece cerrado al abrir un PDF sin ellos. Botón, Ctrl+B y plegado en la cabecera.
- Vista temporal al acercarse al borde izquierdo, sin cambiar el ancho del documento
  ni generar otro render. Se retira al alejarse el puntero y se puede fijar con la
  chincheta o al elegir un capítulo. Clic único y Enter navegan; expandir una rama
  no activa su destino.

El panel fijo cambia el ancho disponible, preservando página, posición vertical
y centro horizontal al ampliar. La búsqueda reduce solamente la altura disponible.
Pintura, selección, hit testing, enlaces y destinos comparten ese espacio de lectura.
Las llamadas a PDFium continúan exclusivamente en el worker; las respuestas de
documentos anteriores y los resultados recibidos con la búsqueda cerrada se descartan.

## Verificación

`benchmark-open.ps1 -Interface`, integrado en `test-reader.ps1`, comprueba con
ventanas ocultas: panel hijo y apertura automática, búsqueda inicialmente oculta,
plegado y recuperación de ancho, vista temporal sin render, centro horizontal con
zoom, búsqueda visible y hit testing del resultado, cierre con conservación de
consulta, descarte de un resultado tardío y cambio a un documento sin índice.
Genera pinturas locales ignoradas por Git para inspeccionar el conjunto de controles.

Una instancia visible con el fixture sintético verificó clic único, Ctrl+F desde
el árbol, escritura y búsqueda incremental, Escape, Ctrl+B y aparición/retiro del
panel al mover el puntero por el borde izquierdo. No se alteró el historial real.
Se ejecuta además la regresión completa de lectura, texto, navegación, enlaces,
destinos, contraseñas/permisos, recientes y persistencia.

Pendientes: validación visual del usuario, DPI por monitor, temas, pestañas y
revisión más amplia de accesibilidad. Esta entrega no declara definitivo el diseño.

## Validación local — 2026-11-16

Tras actualizar desde `8e46ece`, se recompiló Release x64 mediante `scripts/build.cmd`
en esta PC con Visual Studio 2026. `scripts/test-reader.ps1` terminó correctamente,
incluida la prueba `-Interface` y la regresión completa de motor, persistencia,
recientes, contraseñas, geometría, índice, destinos, enlaces, navegación y texto.
Se inspeccionaron las capturas fuera de pantalla con índice/búsqueda y sin paneles;
no se controlaron ventanas del escritorio ni se usó el perfil real del usuario.
El ejecutable actualizado queda en `build/Chucao.exe`, acompañado por `pdfium.dll`.
Esta comprobación no sustituye la prueba del usuario con sus PDF habituales.

## Foco de los botones — 2026-11-16

Se corrigió el borde punteado que quedaba visible tras usar un botón con el mouse.
La pintura de los iconos respeta `ODS_NOFOCUSRECT`; el mouse oculta la indicación
de foco y el teclado vuelve a habilitarla. Se conserva la navegación accesible
con Tab. El fondo activo de índice/búsqueda sigue indicando si están abiertos.
Compilación CMake Release x64 y diagnóstico `-Interface` aprobados. En esta entrega
el EXE corregido está en `build/cmake/Release/Chucao.exe`: la instancia abierta
impidió reemplazar `build/Chucao.exe`. Pendiente de comprobación del usuario con
mouse y Tab en la ventana visible.

## Marca gráfica de la barra — 2026-11-16

El texto CHUCAO de la esquina superior derecha se reemplazó por el icono del
pajarito elegido por el usuario, de 36 píxeles, integrado en los recursos del EXE.
El icono compartido se carga una sola vez y no requiere archivos externos.
Compilación CMake Release y diagnóstico de interfaz aprobados; se inspeccionó
la captura fuera de pantalla. Ejecutable en `build/cmake/Release/Chucao.exe`.

## Acerca de — 2026-11-16

El pajarito es un botón accesible con ayuda contextual; clic, activación por teclado
o F1 abre un diálogo nativo. Muestra versión 0.1.0, Jorge Andres Brugger,
jbrugger@ideaworks.com.ar, fecha de release 16/11/2026 y el repositorio privado
https://github.com/jabrugger/chucao-pdf-reader. Solo el enlace de GitHub abre el
navegador; consultar Acerca de no hace conexiones de red ni carga PDFium.
Escape y la X del diálogo regresan al lector.

Los metadatos se mantienen en `src/app-info.h`; versión y autor también se incluyen
en las propiedades del EXE de Windows. La fecha es explícita para cada entrega,
no la fecha de compilación ni una declaración de publicación de una release en GitHub.
Al versionar una nueva entrega se debe mantener también coherente la versión de CMake.
`--preview-about INFORME` permite generar una captura del diálogo oculto sin abrir
el navegador ni alterar el perfil real. Pendiente de prueba manual de F1, foco,
cierre y apertura del repositorio en el navegador del usuario.
Compilación CMake Release, vista previa del diálogo y diagnóstico `-Interface`
aprobados. Se comprobó también FileVersion/ProductVersion y autor del EXE.

Por preferencia del usuario se quitaron ambos botones y se compactó el diálogo.
El repositorio es un enlace nativo SysLink, activable con mouse o teclado;
Escape y la X cierran la ventana. Se habilitaron los controles comunes v6 de
Windows para ese enlace. Recompilación, captura y diagnóstico de interfaz aprobados.
Al abrir el diálogo el foco inicial queda en el título informativo, para que el único
enlace no aparezca enmarcado automáticamente. Tab permite acceder a él normalmente.

## Pulido de lectura — 2026-11-16

Se ordenó la barra en grupos de apertura, paneles, páginas, zoom e impresión,
con separadores discretos, indicador de página/total y zoom visible. Abrir conserva
un rótulo para facilitar su identificación; el acento naranja remite al chucao.
Se unificaron tipografía, espaciados y colores. El índice usa el tema Explorer
nativo, filas más amplias y sin líneas de jerarquía; muestra un resumen de secciones.
La búsqueda es un campo plano con lupa y borde de foco, integrado con la barra
inferior. El documento tiene una sombra sólida mínima sobre un fondo neutro.

Se mantienen Win32/GDI y PDFium, sin framework visual, animaciones, efectos de
desenfoque ni nuevos recursos gráficos de gran tamaño. UxTheme es una biblioteca
del propio Windows; se usa para el aspecto del árbol. Solo se agrega una fuente
reutilizable de énfasis, liberada al cerrar. Los presupuestos de renderizado y caché
del PDF no cambian. Los indicadores son informativos, no campos de edición.

Verificación: compilación CMake Release y suite completa sobre el ejecutable nuevo:

```powershell
scripts/test-reader.ps1 -Executable 'cmake\Release\Chucao.exe'
```

Los scripts de persistencia y recientes también reciben ese ejecutable; esto evita
probar accidentalmente el EXE anterior cuando una instancia abierta impide sustituirlo.
Se inspeccionaron las capturas con/sin índice y búsqueda. Sigue pendiente validar
la apariencia e interacción con el usuario y a diferentes DPI.
La compilación final también se copió a `build/Chucao.exe` al estar disponible el
archivo; las dos ubicaciones contienen el mismo ejecutable actualizado.

Comprobación orientativa en esta PC, tres ejecuciones antes y tres después por modo:

| Mediana | Antes | Después |
| --- | ---: | ---: |
| Ventana vacía (`shell_ms`) | 56,3 ms | 46,5 ms |
| Recepción del primer bitmap (`ready_ms`), PDF sintético | 389,0 ms | 299,1 ms |

La muestra es pequeña, con cachés sin vaciar y variación de carga del sistema;
no demuestra una mejora de velocidad ni mide la presentación física en pantalla.
No se observó una regresión en esta comprobación. La ventana vacía sigue sin
inicializar PDFium. Los informes e imágenes quedan en `test-results`, fuera de Git.
