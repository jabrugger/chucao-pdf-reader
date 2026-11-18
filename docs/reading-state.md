# Recordar la lectura — 2026-11-13

Al cerrar el lector o cambiar de PDF se conserva el punto de lectura del documento
visible: página, zoom (incluido ajustar al ancho), posición vertical y centro
horizontal. Al volver a abrir esa ruta se intenta recuperar la misma vista dentro
de los límites de la ventana. Home permite volver al comienzo del documento.

El perfil se guarda por usuario y por PC en:

`%LOCALAPPDATA%\ChucaoPDFReader\reading-state.dat`

Se retienen hasta 128 documentos, con límite de archivo de 4 MiB. El perfil contiene
rutas de archivo, cantidad de páginas y coordenadas de lectura. Las rutas se
codifican en hexadecimal UTF-8 para serializarlas de forma inequívoca: esto **no es
cifrado ni anonimización**. No contiene contraseñas, contenido del PDF ni búsquedas.
No se sube a GitHub ni se sincroniza automáticamente entre PC. Para olvidar las
posiciones puede eliminarse ese archivo con todas las ventanas de Chucao cerradas.

## Apertura y recuperación

- La ventana vacía no lee el perfil ni abre documentos de la lista.
- El perfil se consulta después de abrir/autenticar el PDF. Los archivos protegidos
  vuelven a solicitar su contraseña antes de recuperar la posición.
- No se consultan las dimensiones de todas las páginas ni se inspeccionan archivos
  guardados en el perfil. La página recordada tiene prioridad de renderizado y la
  posición se ajusta cuando se conoce su geometría real.
- Una interacción de navegación del usuario cancela el ajuste pendiente para no
  devolverlo inesperadamente a una posición anterior.
- Si cambia la cantidad de páginas del archivo, se ignora la posición guardada.
  No se calcula un hash del PDF: otro contenido con la misma ruta y cantidad de
  páginas puede conservar la posición anterior. Renombrar/mover el archivo o usar
  un alias distinto no conserva necesariamente la asociación.

## Guardado y fallos

El guardado se hace al cerrar normalmente, cambiar de PDF o recibir el aviso de
fin de sesión de Windows. No hay autoguardado periódico; un cierre forzado o caída
puede perder los movimientos de la sesión actual. Aperturas fallidas, cancelaciones
de contraseña y vistas todavía no disponibles no reemplazan una posición válida.

Se escribe un temporal, se vacía al disco y se reemplaza el perfil. Un archivo
`.lock` coordina la combinación de entradas entre ventanas, con espera acotada de
unos dos segundos y pausas crecientes; el reemplazo final también se reintenta
ante bloqueos transitorios de lectores o antivirus. Distintos documentos no se
pisan; si dos ventanas guardan el mismo documento, prevalece la última escritura
completada. Si no se puede escribir, el lector sigue funcionando y se conserva el
perfil anterior.

Las lecturas están acotadas y descartan versiones desconocidas o registros
inválidos. Un guardado posterior reconstruye el perfil con el formato actual y
las entradas válidas recuperadas. La restauración puede ajustar la posición si
cambia la ventana, el zoom efectivo o la geometría estimada de páginas distintas.

## Verificación ejecutada

`scripts/test-reading-state.ps1`, integrado en `test-reader.ps1`, utiliza únicamente
perfiles aislados en test-results. No toca el perfil real de LocalAppData.

- El test del almacenamiento verifica Unicode, mayúsculas/separadores de ruta,
  ida y vuelta de coordenadas, cambio de cantidad de páginas, valores inválidos,
  fusión de escritores simultáneos, tolerancia a un bloqueo de escritura mantenido
  700 ms, límite de 128 entradas, archivos corruptos o demasiado grandes,
  recuperación y destino no escribible.
- Siete procesos reales del lector guardan y recuperan dos documentos distintos
  y un PDF cifrado, a zoom 300%, con posición vertical y horizontal conocidas.
  Los procesos de lectura no alteran el perfil. El PDF protegido vuelve a pedir
  contraseña en cada proceso y esta no aparece en el archivo de estado.
- Pasó también la regresión de contraseñas/permisos, texto, navegación, índice y
  enlaces. Se verificó la compilación con build.cmd y CMake.

Los modos de diagnóstico normales deshabilitan la persistencia para que las
pruebas no alteren las posiciones del usuario ni restauren páginas inesperadas.
Los modos `--benchmark-state-write/read` requieren una ruta de perfil explícita.
Pendiente: validación manual con documentos reales, varias ventanas y pantallas.
