# Verificación inicial — 2026-11-12

Registro histórico del primer prototipo. Para el estado actual consultar [TODO.md](TODO.md);
las pruebas posteriores de contraseñas y permisos están en [passwords.md](passwords.md).

- Compilación Release x64 con MSVC 19.16 (Visual Studio 2017): correcta, /W4.
- PDFium chromium/8152: descarga fijada y SHA-256 comprobado.
- Fixture generado localmente: PDF de dos páginas con texto Helvetica.
- Apertura: correcta, dos páginas detectadas.
- Render a 800 × 1000: 2723 píxeles oscuros, imagen no vacía.
- Búsqueda de `Chucao`: coincidencia encontrada.
- Archivo inexistente: error 2 manejado, código de salida de prueba esperado.
- Primera medición de proceso nuevo: apertura 9,949 ms; apertura + primera página
  254,188 ms. Es una sola muestra sintética, no un benchmark ni una comparación.

Pendientes: inspección visual del visor (control de aplicaciones interrumpido con
Escape), flujo de contraseñas, impresión virtual/física, corpus real y rendimiento
comparativo. No presentar esta base como versión lista para uso diario.
