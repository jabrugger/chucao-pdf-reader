# Avisos de terceros

Chucao PDF Reader incluye el motor PDFium y sus dependencias. El código propio
de Chucao se distribuye con la licencia MIT incluida en [LICENSE](LICENSE).

## PDFium

- Licencia: BSD de tres cláusulas (estilo BSD).
- Origen del binario: distribución comunitaria
  [`bblanchon/pdfium-binaries`](https://github.com/bblanchon/pdfium-binaries),
  versión `chromium/8152`, Windows x64, compilada sin V8.
- Uso: `pdfium.dll` se carga dinámicamente al abrir el primer documento.
- Texto completo: `licenses/pdfium.txt` dentro del paquete y
  `deps/pdfium/LICENSE` en el repositorio de trabajo.

El archivo `pdfium.dll` no se versiona en Git. La versión fijada, su URL y su
SHA-256 están en `scripts/setup-pdfium.ps1`; el proceso de actualización está en
[pdfium-update.md](docs/pdfium-update.md).

## Dependencias de PDFium

La distribución de PDFium incluye, entre otras, las siguientes bibliotecas. Cada
texto de licencia se copia al paquete dentro de `licenses/`.

| Biblioteca | Archivo de licencia |
| --- | --- |
| Abseil | `abseil.txt` |
| AGG (agg23) | `agg23.txt` |
| Dragonbox | `dragonbox-Apache2-LLVM.txt`, `dragonbox-Boost.txt` |
| fast_float | `fast_float.txt` |
| FreeType | `freetype.txt` |
| HarfBuzz | `harfbuzz.txt` |
| Highway | `highway.txt` |
| ICU | `icu.txt` |
| Little CMS | `lcms.txt` |
| libjpeg-turbo | `libjpeg_turbo.ijg`, `libjpeg_turbo.md` |
| OpenJPEG | `libopenjpeg.txt` |
| libpng | `libpng.txt` |
| llvm-libc | `llvm-libc.txt` |
| simdutf | `simdutf.txt` |
| zlib | `zlib.txt` |

Los avisos se obtienen del propio paquete de PDFium descargado, de modo que una
actualización del motor actualiza también esta lista. Revisar el contenido de
`deps/pdfium/licenses` al cambiar de versión y ajustar la tabla si aparecen o
desaparecen dependencias.

## Marcas y arte

El icono y la imagen de bienvenida fueron aportados por el usuario del proyecto.
Microsoft Windows, Visual Studio y las bibliotecas mencionadas pertenecen a sus
respectivos titulares. Chucao no está afiliado a Google, PDFium ni a los autores
de esas bibliotecas.
