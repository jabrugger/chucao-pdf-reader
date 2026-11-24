# Estado y orden de continuación — 2026-11-18

Actualización posterior: [revisión del 2026-11-19](review-2026-11-19.md), con
Opciones, sesión, bienvenida y resaltado del índice ya incorporados. El filtro
del índice sigue pendiente después de validar el resaltado con documentos reales.

Actualización 2026-11-24: versión 0.2.11 (tag `v0.2.11`, que corrige el arranque sin
botón en la barra de tareas; antes `v0.2.10` y `v0.2.9`, que incluyó el chequeo de
actualizaciones opt-in), `main` sincronizada con `origin/main` y árbol limpio.
Se recompiló con `scripts/build.cmd` y la suite completa pasó con código 0; el
alcance y la evidencia están en [TODO.md](TODO.md). El siguiente paso sigue
siendo la validación manual y las mediciones, sin agregar funciones grandes.

Base revisada: `3f55ece`, rama `main`, sincronizada con `origin/main` al iniciar.
El árbol estaba limpio. Se revisaron los 14 commits posteriores a `0c89272` y sus
documentos de alcance. No se revirtieron cambios de DS ni se modificó el motor.

## Lo incorporado

- Destinos FitR y de caja de contenido; guardado del historial con reintentos.
- Licencia MIT, avisos de terceros, empaquetado portable y actualización de PDFium.
- Mensajes de error más claros y medición hasta el primer cuadro compuesto.
- Temas claro/oscuro, preferencia local y paleta de alto contraste.
- DPI por monitor, accesibilidad de teclado/MSAA e iconos vectoriales suavizados.
- Aperturas consecutivas y una primera implementación de pestañas.

El alcance detallado y las validaciones manuales abiertas se conservan en [TODO.md](TODO.md).

## Límites que condicionan el siguiente paso

**Pestañas:** solo hay un PDF cargado en el worker. Al alternar se reabre desde disco;
los protegidos vuelven a pedir contraseña y los remotos dependen del I/O. Esto está
documentado en [tabs.md](tabs.md), pero no equivale a conmutación instantánea.

En la base `3f55ece`, `RunTabsTest` solo comprobaba los comandos sin esperar los
bitmaps ni la recuperación real. Ese límite de validación quedó resuelto por la
tanda de estabilización descrita abajo; no sustituye el uso con PDF reales/remotos.

**Cierre:** en la base revisada, `CloseDocument` vaciaba la UI sin ordenar el cierre
del PDF. Ahora encola un cierre explícito y el worker confirma su finalización,
invalidando revisiones y generaciones anteriores. Una lectura de I/O bloqueante
sigue siendo un límite separado del worker único.

**Validación:** los modos de prueba de DPI y alto contraste simulan métricas/paleta;
no sustituyen monitores reales ni cambiar Windows a alto contraste. Impresión,
IME, lectores de pantalla, archivos remotos y sensaciones de interacción siguen
requiriendo las pruebas manuales registradas.

**Entrega:** `src/app-info.h` y CMake indican 0.2.12, con el manifiesto de Windows
incrustado, el chequeo de actualizaciones opt-in y la corrección de causa raíz del
botón de la barra de tareas; el tag `v0.2.12` está empujado a `origin` (`v0.2.11`,
`v0.2.10`, `v0.2.9` en `34bd54c` y `v0.2.8` en `98b9056`). Falta probar el ZIP y el
instalador 0.2.12 en otra PC.

## Orden de trabajo

1. **Estabilizar pestañas.** Esperar cargas reales A→B→A, comprobar página/zoom y
   posición, cierre de activa/inactiva/última, resultados atrasados y recuperación
   ante archivo ausente o contraseña cancelada. Resolver el cierre real en el worker.
   Mantener inicialmente el modelo liviano; cualquier caché de documentos debe
   justificarse con mediciones de memoria y tiempos.
   Primera tanda automatizada completada el 2026-11-18; queda la validación manual.
2. **Validar el lector cotidiano.** PDF real normal/protegido/remoto, índice temporal,
   búsqueda, selección/copia, teclado, cambio de zoom/tamaño y cierre durante carga.
   Después, impresión y DPI/alto contraste reales. Registrar resultados sin marcar
   comprobaciones manuales como realizadas por una prueba oculta.
3. **Medir con un corpus pequeño.** Tiempos de apertura y cambio de pestaña, memoria
   y cierre. Distinguir caché caliente/fría y primer cuadro compuesto/presentación
   física. Comparar con Sumatra/KillerPDF cuando haya una secuencia reproducible.
4. **Cerrar una entrega portable coherente.** Fijar versión/fecha, generar el ZIP
   desde la compilación probada y ejecutarlo en otra PC. Preparar instalador después.

Los anuncios de accesibilidad y los destinos con coordenadas null siguen pendientes,
pero no desplazan la estabilización de pestañas ni la velocidad de lectura.
No agregar más funciones grandes hasta cerrar el primer bloque.

## Ajuste de las comprobaciones

Se corrigió `test-reader.ps1`: los modos nuevos de tema, DPI, teclado, alto contraste,
apertura rápida y pestañas tenían `build/Chucao.exe` fijo, ignorando `-Executable`.
Ahora todos usan la compilación elegida. Comando de validación local:

```powershell
scripts/test-reader.ps1 -Executable 'cmake\Release\Chucao.exe'
```

No se reempaquetó ni se modificó el perfil real del usuario durante esta revisión.

Resultado local del 2026-11-18: compilación completa CMake Release x64 y suite
aprobadas. Los informes de tema, DPI, teclado, alto contraste, apertura rápida y
pestañas se regeneraron correctamente con el ejecutable elegido; también pasaron
persistencia, recientes, permisos/contraseñas, errores, geometría, índice/destinos,
interfaz, enlaces, navegación y texto. Esto no amplía el alcance de las pruebas de
pestañas ni cierra las validaciones manuales pendientes descritas arriba.

## Tanda de estabilización completada — 2026-11-18

- Posición de cada pestaña en memoria, sin depender de que el perfil sea escribible.
- Cierre explícito del PDF en el worker y acuse de finalización; descarte de trabajo
  de generaciones anteriores.
- Reintento de pestaña fallida o con contraseña cancelada sin duplicarla.
- `test-tabs.ps1`: 27 comprobaciones asincrónicas con bitmaps reales, A/B a
  distintos zooms/posiciones, errores, tres pedidos de contraseña y dos acuses de cierre.
- Compilación CMake Release y regresión completa aprobadas sobre el ejecutable nuevo.

Siguiente bloque: probar el EXE con documentos habituales, medir el cambio entre
pestañas de archivos grandes/remotos y verificar cierre durante I/O. No se agregan
todavía varios documentos de PDFium en memoria ni se regenera el portable.
