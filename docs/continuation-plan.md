# Estado y orden de continuación — 2026-11-18

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

`RunTabsTest` comprueba los comandos y el modelo de pestañas de forma sincrónica.
No espera los bitmaps de A/B ni verifica la recuperación real de sus posiciones.
Que pase esa prueba no valida aún el ciclo completo de lectura entre pestañas.

**Cierre:** en el código revisado, `CloseDocument` llama a `ResetDocument` para
vaciar la UI, pero no encola un cierre del documento de PDFium. El worker cierra su
`pdf_` al cargar el siguiente o al destruirse. Falta verificar y resolver la liberación
efectiva al cerrar la última pestaña, junto con la cancelación de trabajos anteriores.
Una lectura de I/O bloqueante sigue siendo un límite separado del worker único.

**Validación:** los modos de prueba de DPI y alto contraste simulan métricas/paleta;
no sustituyen monitores reales ni cambiar Windows a alto contraste. Impresión,
IME, lectores de pantalla, archivos remotos y sensaciones de interacción siguen
requiriendo las pruebas manuales registradas.

**Entrega:** `src/app-info.h` todavía indica 0.1.0 y fecha de release 16/11/2026.
Existe un ZIP portable; esta revisión no lo declara actualizado respecto de todos
los cambios posteriores. Hay que regenerar y verificar el paquete al cortar una
entrega, con versión/fecha y ejecutable coherentes.

## Orden de trabajo

1. **Estabilizar pestañas.** Esperar cargas reales A→B→A, comprobar página/zoom y
   posición, cierre de activa/inactiva/última, resultados atrasados y recuperación
   ante archivo ausente o contraseña cancelada. Resolver el cierre real en el worker.
   Mantener inicialmente el modelo liviano; cualquier caché de documentos debe
   justificarse con mediciones de memoria y tiempos.
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
