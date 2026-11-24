# Distribución portable

Chucao se distribuye como una carpeta portable con el ejecutable, el motor
PDFium y los avisos legales. No requiere instalador, no escribe claves de
registro, no asocia archivos y no instala servicios.

## Generar el paquete

```powershell
powershell -ExecutionPolicy Bypass -File scripts/package.ps1
```

El script compila con `scripts/build.cmd`, toma la versión de `src/app-info.h` y
crea:

```text
dist/
  Chucao-0.2.17-win-x64/
    Chucao.exe
    pdfium.dll
    LEEME.txt
    LICENSE
    THIRD-PARTY-NOTICES.md
    licenses/…
  Chucao-0.2.17-win-x64.zip
```

`Chucao.exe` lleva el manifiesto de Windows incrustado (Common Controls v6 y
`asInvoker`), así que no necesita ningún `Chucao.exe.manifest` externo. El
paquete y el instalador no lo copian; ver [instalador](installer.md).

La carpeta `dist/` está excluida de Git. El nombre de la versión sale de
`CHUCAO_VERSION_STRING`; recordar actualizar también `CHUCAO_VERSION_NUMBERS` y
`CHUCAO_RELEASE_DATE`.

Para reempaquetar una compilación existente sin recompilar:

```powershell
powershell -ExecutionPolicy Bypass -File scripts/package.ps1 -SkipBuild
```

## Verificar el paquete

```powershell
powershell -ExecutionPolicy Bypass -File scripts/test-package.ps1
```

El script descomprime el paquete más reciente en `test-results/package-check`,
comprueba la presencia de los archivos obligatorios y ejecuta el lector
empaquetado con la prueba oculta de apertura y renderizado. Genera
`sample.pdf` si todavía no existe.

La verificación automática no reemplaza estas pruebas manuales antes de
publicar:

- Ejecutar desde una carpeta con espacios y caracteres Unicode en la ruta.
- Comprobar que `Chucao.exe` abre sin `pdfium.dll` (debe fallar de forma
  controlada) y que funciona con el DLL junto a él.
- Probar un PDF normal, uno protegido y uno con índice y enlaces.
- Revisar la ventana a 100 %, 150 % y 200 % de escala.
- Confirmar que no se crean asociaciones ni se escribe en el registro.
- Revisar los avisos y licencias incluidos.

## Requisitos y límites

- Windows 10 u 11 x64. La compilación es Release x64 con el runtime estático de
  MSVC; no requiere redistribuibles de Visual C++.
- No hay telemetría ni conexión de red propia. Un clic explícito en un enlace
  HTTP/HTTPS abre el navegador predeterminado.
- El ejecutable no está firmado. Windows SmartScreen puede mostrar una
  advertencia al abrirlo en otra PC; firmar el binario queda pendiente.
- Datos locales: `%LOCALAPPDATA%\ChucaoPDFReader\reading-state.dat` guarda rutas
  recientes y posiciones, y `last-session.dat` la última sesión si se activó esa
  opción (no contraseñas ni contenido). Se pueden borrar desde **Opciones →
  Olvidar recientes y posiciones guardadas** o eliminando los archivos con el
  lector cerrado.
- Espacio en disco del paquete descomprimido: alrededor de 11 MiB.

## Licencia y terceros

El código propio usa la licencia MIT de [LICENSE](../LICENSE). El motor PDFium
y sus dependencias conservan sus licencias; ver
[THIRD-PARTY-NOTICES.md](../THIRD-PARTY-NOTICES.md). El paquete copia los textos
desde `deps/pdfium/licenses`, por lo que hay que regenerarlo después de
actualizar el motor (ver [pdfium-update.md](pdfium-update.md)).

## Instalador

`scripts/package-installer.ps1` genera un instalador con Inno Setup 7 a partir del
paquete portable. Instala por usuario, ofrece la asociación opcional de PDF y un
acceso directo opcional, y desinstala archivos, accesos y sus entradas de
asociación. El instalador no está firmado. Ver [installer.md](installer.md) para
el detalle y las pruebas manuales pendientes.
