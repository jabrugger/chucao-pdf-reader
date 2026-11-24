# Actualizar PDFium

Chucao no versiona binarios del motor. `scripts/setup-pdfium.ps1` descarga una
versión fijada desde [`bblanchon/pdfium-binaries`](https://github.com/bblanchon/pdfium-binaries),
comprueba su SHA-256 y extrae `deps/pdfium`. La compilación enlaza contra
`deps/pdfium/lib/pdfium.dll.lib` y copia `pdfium.dll` junto al ejecutable.

## Versión actual

| Dato | Valor |
| --- | --- |
| Versión | `chromium/8152` |
| Plataforma | Windows x64, sin V8 |
| Archivo | `deps/pdfium-win-x64-8152.tgz` |
| URL | `https://github.com/bblanchon/pdfium-binaries/releases/download/chromium/8152/pdfium-win-x64.tgz` |
| SHA-256 | `cee2b0e9990bc67c0721774932961b9dbf2766046c10eec925acd4cfba6fdfe3` |

## Procedimiento

1. Elegir una versión pública de `pdfium-binaries`. Preferir una que indique la
   revisión de Chromium y que se haya estabilizado; evitar builds de un día.
2. Descargar manualmente `pdfium-win-x64.tgz` y calcular su SHA-256:

   ```powershell
   Get-FileHash -Algorithm SHA256 pdfium-win-x64.tgz
   ```

3. Actualizar en `scripts/setup-pdfium.ps1` el nombre del archivo, la versión de
   la URL y el hash esperado. No se acepta un hash comodín.
4. Borrar `deps/pdfium` y el archivo `.tgz` anterior para no reutilizar restos.
5. Ejecutar:

   ```powershell
   powershell -ExecutionPolicy Bypass -File scripts/setup-pdfium.ps1
   cmd /c scripts\build.cmd
   ```

6. Revisar `deps/pdfium/VERSION`, `deps/pdfium/LICENSE` y la carpeta
   `deps/pdfium/licenses`. Actualizar [THIRD-PARTY-NOTICES.md](../THIRD-PARTY-NOTICES.md)
   si cambió la lista de dependencias.
7. Ejecutar la regresión completa y el empaquetado:

   ```powershell
   powershell -ExecutionPolicy Bypass -File scripts/test-reader.ps1
   powershell -ExecutionPolicy Bypass -File scripts/test-package.ps1
   ```

8. Probar manualmente al menos un documento real con contraseña, un enlace
   interno, un enlace web, búsqueda, selección, índice e impresión, porque esos
   flujos dependen del comportamiento del motor.
9. Registrar la versión, el hash, la fecha y el resultado en
   [performance.md](performance.md) cuando se midan tiempos, y en
   [TODO.md](TODO.md) cuando cambie el estado de las validaciones.

## Compatibilidad a revisar

- Firma y semántica de las funciones usadas en `src/` (destinos, texto,
  renderizado progresivo, permisos y metadatos del documento).
- Comportamiento de `FPDFDest_GetView` y `FPDFDest_GetLocationInPage`: el lector
  usa `FPDFDest_GetView` para saber si FitH/FitV/FitBH/FitBV llevan su coordenada
  (presente o ausente) y `FPDFDest_GetLocationInPage` para los campos XYZ. Si
  cambiara la semántica —por ejemplo, que `FPDFDest_GetLocationInPage` dejara de
  ser exclusivo de XYZ o que `FPDFDest_GetView` distinguiera null de cero— habría
  que revisar la lectura de la coordenada omitida.
- Cambios en el tamaño o las dependencias de `pdfium.dll`, y avisos de terceros.
- Que la carga diferida (`/DELAYLOAD:pdfium.dll`) siga funcionando: abrir la
  ventana vacía no debe cargar el motor, y abrir un PDF sí.
- Resultados de la búsqueda y del renderizado con los fixtures sintéticos.

## Reversión

Si la versión nueva falla, restaurar la URL y el SHA-256 anteriores en
`scripts/setup-pdfium.ps1`, borrar `deps/pdfium` y repetir la instalación y la
compilación. Los binarios no se recuperan de Git.
