# Plan de trabajo

La lista consolidada y verificable de pendientes está en [TODO.md](TODO.md).

1. **Base funcional (implementada)**: C++, PDFium, abrir, zoom,
   contraseña, búsqueda e impresión. Compilar y verificar el motor con fixtures
   sintéticos. Git local y repo remoto privado creados. Contraseñas y permisos
   cuentan con pruebas automatizadas; el diálogo visible y la impresión real
   todavía requieren validación manual.
2. **Experiencia de lectura (en curso)**: implementados desplazamiento continuo,
   caché acotada, precarga de páginas vecinas, cancelación de render progresivo y
   prioridad de página visible, selección/copia entre páginas y búsqueda con
   resaltado y salto preciso, y desplazamiento horizontal con zoom alto.
   Índice jerárquico bajo demanda con navegación a coordenadas XYZ y regreso;
   Fit/FitH/FitV y zoom XYZ compartidos con enlaces internos.
   Enlaces internos con
   destinos directos/nombrados y regreso al punto de lectura. Persistencia local de
   página, zoom y posición por documento. Enlaces web HTTP/HTTPS por clic explícito.
3. **Interfaz**: propuesta visual inspirada en MarkPad, toolbar compacta, panel
   opcional, pestañas, claro/oscuro, DPI por monitor y navegación por teclado.
   Validar con el usuario antes de convertir la propuesta en diseño definitivo.
4. **Validación de motor**: corpus y métricas reproducibles contra Sumatra/KillerPDF.
   Comparar MuPDF si hay fallos que justifiquen el coste; motor KillerPDF aislado
   si aporta ventajas medibles. No asumir que un lenguaje determina el resultado.
5. **Versión de uso diario (en curso)**: implementados archivos recientes y permisos
   de impresión/copia. Pendientes errores claros, accesibilidad, pruebas con impresoras físicas
   y virtuales, documentos grandes/remotos, empaquetado y avisos de dependencias.

Fuera de alcance permanente: editar PDF, anotaciones, OCR, combinar/dividir,
redactar, firmar o crear documentos. La impresión no modifica el archivo de origen.

## Criterios manuales antes de distribuir

- Contraseña correcta, incorrecta, cancelar y caracteres no ASCII.
- Unicode en rutas y texto; documentos con restricciones de impresión/copia.
- Abrir A y luego B rápidamente; cambiar zoom y tamaño mientras se dibuja.
- Cerrar durante lectura, búsqueda o renderizado, sin resultados tardíos ni fugas.
- No ejecutar acciones JavaScript ni abrir enlaces web automáticamente; comprobar
  el clic explícito y la apertura en el navegador predeterminado.
- Impresión: rangos, orientación, tamaño, copias, cancelar y errores del spooler.
- CPU/memoria tras muchas aperturas; documentos dañados o de dimensiones extremas.
- En esta fase no cambiar asociaciones de PDF ni instalar servicios en Windows.
