# Identidad de Chucao

Imagen del chucao aportada y elegida por el usuario el 2026-11-13, para usar como
icono e imagen del software. Se conserva el dibujo original, incluidos sus colores,
fondo, proporciones y márgenes. No se rediseñó ni se generó una variante del personaje.

- chucao-original.jpg: archivo original aportado por el usuario.
- chucao.png: versión PNG de 1024 píxeles para documentación y usos gráficos.
- chucao-icon-256.png: vista previa de 256 píxeles.
- chucao.ico: imágenes de 16, 20, 24, 32, 40, 48, 64, 96, 128 y 256 píxeles.
- chucao-home.bmp: recurso de 256 píxeles para pintar la bienvenida con GDI.

Los derivados se preparan con scripts/prepare-branding.ps1 usando System.Drawing
de Windows. Son conversiones y cambios de tamaño, sin modificación del contenido.
El script puede ejecutarse con el original ya versionado; no requiere la carpeta
Downloads de la PC donde se recibió la imagen.

El icono y el bitmap se incluyen en el ejecutable mediante src/chucao.rc, tanto
en el build.cmd como en CMake. La aplicación no necesita archivos de imagen junto
al EXE. El bitmap de bienvenida se carga bajo demanda y se libera al cerrar.

Verificación: compilación x64 con build.cmd y CMake, extracción del icono de 32 px
desde el EXE e inspección de la bienvenida pintada fuera de pantalla. Pasó la
regresión de lectura, navegación, texto, índice, enlaces, contraseñas y permisos.
La inspección en Explorer/barra de tareas y a distintos DPI sigue en el TODO.
`Chucao.exe --preview-home informe.json` genera una vista local sin mostrar ventanas;
es un modo de diagnóstico, separado de la medición normal de arranque.
