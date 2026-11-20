# Instalador de Windows

El instalador se genera con **Inno Setup 7** a partir del paquete portable ya
verificado. No requiere administrador: instala por usuario en
`%LOCALAPPDATA%\Programs\Chucao`, no crea servicios ni asociaciones obligatorias.

## Generar

```powershell
powershell -ExecutionPolicy Bypass -File scripts/package-installer.ps1
```

El script:

1. Ejecuta `scripts/package.ps1` (compila y genera el ZIP portable) salvo que se
   use `-SkipPackage` con la carpeta portable ya presente.
2. Toma la versión de `src/app-info.h` y compila `installer/Chucao.iss` con
   `ISCC.exe`.
3. Deja `dist\Chucao-<versión>-setup.exe` e imprime tamaño y SHA-256.

Para recompilar solo el instalador sobre un paquete existente:

```powershell
powershell -ExecutionPolicy Bypass -File scripts/package-installer.ps1 -SkipPackage
```

El instalador no está firmado; Windows puede mostrar SmartScreen en otra PC.

## Manifiesto de Windows

El ejecutable lleva **incrustado** su manifiesto (`src/chucao.manifest`, recurso
`RT_MANIFEST` 1): declara Common Controls v6 y `asInvoker`. Eso mantiene
registrado el control `SysLink` que usan **Acerca de** y **Opciones**.

Antes de 0.2.1 el manifiesto era un archivo externo (`Chucao.exe.manifest`) que
solo existía en `build\`; el instalador y el ZIP no lo copiaban. Windows caía a
Common Controls v5, `DialogBoxParam` fallaba y esos botones parecían no hacer
nada, aunque Ctrl+O y el resto del lector siguieran funcionando.

- El manifiesto se compila desde `src/chucao.rc` y el enlazador usa
  `/MANIFEST:NO` para no generar un archivo externo.
- El instalador elimina cualquier `Chucao.exe.manifest` externo que haya quedado
  de una instalación anterior (`[InstallDelete]`).
- Si un diálogo no se pudiera crear igualmente, la app muestra un mensaje con la
  versión y el repositorio en vez de dejar el botón sin respuesta.
- `scripts/test-reader.ps1` y `scripts/test-package.ps1` ejecutan
  `--preview-about` sobre el EXE compilado y sobre el EXE extraído del ZIP, sin
  manifiesto externo, y fallan si el diálogo no se crea.

## Tareas opcionales

- **Abrir los archivos PDF con Chucao**: registra el ProgID `Chucao.PDF` y la
  entrada `OpenWithProgids` para `.pdf`, e intenta fijar `.pdf` a Chucao. Está
  desmarcada por defecto para no apropiarse de la asociación sin permiso.
- **Acceso directo en el escritorio**: también opcional.
- El menú Inicio y el desinstalador se crean siempre.

Windows 10/11 protege la aplicación predeterminada con `UserChoice`. Si el usuario
ya eligió otro lector, el instalador **no la elimina**: registra Chucao para
«Abrir con» y deja la elección final a Configuración. La app ofrece el mismo
control y un enlace directo en **Opciones → Abrir Configuración de aplicaciones
predeterminadas**.

## Asociación desde la aplicación

**Opciones → Abrir los PDF con Chucao** aplica la asociación al instante:

- Registra `HKCU\Software\Classes\Chucao.PDF` con descripción, icono y comando
  `"Chucao.exe" "%1"`.
- Agrega `Chucao.PDF` a `.pdf\OpenWithProgids`.
- Intenta fijar el valor predeterminado de `.pdf`.
- Consulta el estado efectivo (incluido `UserChoice`) y explica si Windows mantiene
  otra aplicación.
- Al desmarcar, quita solo las entradas de Chucao. Si `UserChoice` apuntaba a
  Chucao, la elimina; si apuntaba a otra app, no la toca.

La lógica vive en `src/file-association.h` y se prueba con una extensión desechable
en `association-test.exe`, sin modificar `.pdf`.

## Desinstalación

El desinstalador elimina archivos, accesos directos y, si siguen apuntando a
Chucao, el ProgID, la entrada `OpenWithProgids`, el valor predeterminado de `.pdf`
y `UserChoice`. Una asociación elegida para otra aplicación se conserva.

## Verificación pendiente

La compilación y la lógica de asociación están automatizadas. La instalación 0.2.3
se verificó localmente el 20/11/2026: tras actualizar una instalación anterior,
«Acerca de» y «Opciones» abren desde el menú Inicio y el EXE carga Common Controls
v6 sin ningún `Chucao.exe.manifest` externo. Antes de publicar el instalador hay
que probar manualmente en otra PC:

- Instalación per-usuario, inicio desde el menú y desde el escritorio.
- Apertura de **Acerca de** (icono o F1) y **Opciones** desde la instalación real.
- Asociación opcional, apertura de un PDF desde Explorer y «Abrir con».
- Comportamiento cuando Windows mantiene otro lector y uso del enlace a
  Configuración.
- Desinstalación completa y conservación de la asociación de otra app.
- Actualización sobre una instalación anterior y ejecución en otra PC.
- Firma Authenticode y aviso de SmartScreen.
