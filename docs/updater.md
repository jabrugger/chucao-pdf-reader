# Buscar actualizaciones al abrir — 2026-11-23

Chucao puede consultar, al abrirse, si hay una versión nueva publicada en GitHub.
La opción está **desactivada por defecto** y se activa en **Opciones → Buscar
actualizaciones al abrir**. No descarga ni instala nada y no envía telemetría.

## Comportamiento

- Al arrancar (solo si la opción está activada y como máximo una vez por día), un
  hilo corto consulta la API `releases/latest` del repositorio y compara el tag
  publicado con `CHUCAO_VERSION_STRING`.
- Si hay una versión superior, aparece un aviso con botón que abre la página de
  releases en el navegador predeterminado (reutiliza la lógica de enlaces web).
- Cualquier fallo (sin red, DNS, TLS, HTTP 404/429, timeout o respuesta inválida)
  es silencioso: el lector abre igual. El GET tiene un límite de 3 segundos por
  fase y 64 KiB de cuerpo.

## Alcance y garantías

- **Solo lectura de metadatos.** No se descarga el ejecutable ni el instalador; la
  actualización real queda en manos del usuario (y del instalador Inno Setup).
- **Sin telemetría.** Es una única petición a la API pública de GitHub, sin IDs
  únicos ni contenido del documento.
- **Opt-in.** El valor por defecto es `updates=0`; la preferencia se guarda en
  `settings.dat` junto al resto.

## Implementación

- `src/updater.h`: fragmento que usa WinHTTP (`winhttp.lib`) para el GET, extrae
  `tag_name` del JSON y compara versiones numéricamente (`v9.9.9` > `0.2.9`).
- `Reader::StartUpdateCheck` lanza el hilo; el resultado vuelve por el mensaje
  `UpdateReady` y `OnUpdateChecked` muestra el aviso si corresponde.
- La opción aparece en el diálogo de Opciones como una casilla más
  (`IDC_SETTINGS_UPDATES`).

## Verificación

El diagnóstico `--benchmark-updates <json> <informe>` evalúa un payload local con
el mismo parser y comparador, sin red. La suite lo ejecuta dos veces (tag `v9.9.9`
→ `newer=true`, tag `v0.0.1` → `newer=false`) y comprueba el informe. La
persistencia de la preferencia se cubre en `preferences-test.exe`.

Pendiente de validación manual: el aviso real con una release publicada y el
comportamiento con proxy/firewall corporativo.
