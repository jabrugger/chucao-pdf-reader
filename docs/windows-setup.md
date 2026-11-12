# Preparación en otra PC Windows

La copia nueva de `main` partió de `cd18f56`, sin commits posteriores en el remoto.
Se instaló la carga de desarrollo de escritorio con C++ en Visual Studio Community
2026 y el Windows SDK. PDFium chromium/8152 x64 se descargó con el script existente
y su SHA-256 coincidió. No se cambiaron asociaciones de archivos ni servicios.

`scripts/build.cmd` detecta Visual Studio con `vswhere`, inicializa el entorno x64
y mantiene el respaldo para la instalación original de Visual Studio 2017.
También acepta un Developer Command Prompt x64 ya inicializado.

La compilación Release con `/W4` y las pruebas existentes pasaron antes de modificar
la navegación. Tras añadir desplazamiento horizontal se repitieron motor, geometría,
navegación y texto. El ejecutable y `pdfium.dll` quedan juntos en `build/`.
Los fixtures, informes e imágenes permanecen en `test-results/`, ignorado por Git.

Estas pruebas sintéticas no miden primera pintura visible ni demuestran mejoras
de velocidad respecto de otra PC. Siguen pendientes las validaciones manuales de
contraseñas, impresión, portapapeles y restricciones documentadas en el roadmap.
