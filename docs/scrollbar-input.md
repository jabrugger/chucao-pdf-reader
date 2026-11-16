# Arrastre de las barras de desplazamiento — 2026-11-15

Se corrigió un fallo que permitía ver la posición en la barra vertical pero
cancelaba el arrastre al intentar moverla con el mouse.

La ventana principal usa captura del mouse tanto para seleccionar texto como
para las barras nativas de Windows. Durante el seguimiento de la barra, Windows
puede emitir `WM_CAPTURECHANGED` indicando como nuevo propietario la misma ventana.
El manejador anterior terminaba la selección y liberaba la captura incluso cuando
no había un arrastre de texto. Esto interrumpía el seguimiento nativo de la barra.

Ahora la selección solo libera la captura si había iniciado su propio arrastre.
Un aviso que conserva el mismo propietario no cancela la selección ni interfiere
con la barra. Al perder realmente la captura se limpia la selección en curso sin
intentar liberar una captura que ya puede pertenecer a otra operación.

## Verificación

- Reproducción del fallo con arrastre real en la versión anterior y en un fixture.
- Versión corregida: arrastre vertical desde la página 1 hasta la 4 y regreso
  hasta la 1, usando el PDF sintético de tamaños mixtos.
- Selección física de texto con mouse después del arrastre, sin tocar el portapapeles.
- Regresión oculta de navegación: captura nativa conservada al notificar el mismo
  propietario, avance desde la barra, limpieza al perder captura y liberación al
  terminar una selección. Se mantiene el recorrido de navegación y caché anterior.
- Compilación Release x64 y batería completa de motor, contraseñas/permisos,
  persistencia, recientes, índice/destinos, enlaces, geometría, navegación y texto.

Las comprobaciones de interacción usaron una instancia de prueba sin persistencia.
No se incorporan al repositorio documentos del usuario ni registros de diagnóstico.
