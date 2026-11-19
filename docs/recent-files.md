# Archivos recientes

La pantalla de bienvenida muestra, debajo del chucao, una lista con hasta diez
PDF abiertos antes. Un clic o Enter abre el elegido y recupera su posición. La
lista se lee solo al mostrar la bienvenida: no se recorren carpetas ni se consulta
la existencia de los archivos. El arranque vacío sigue sin cargar PDFium.

El botón de carpeta y Ctrl+O abren directamente el selector de archivos, sin menú
intermedio. La lista de recientes se vacía desde **Opciones → Olvidar recientes y
posiciones guardadas**, que pide confirmación y también elimina las posiciones
guardadas del perfil.

La lista reutiliza el perfil de [estado de lectura](reading-state.md), que conserva
hasta 128 documentos. El documento actual aparece primero una vez renderizado; su
posición se guarda al cambiar de archivo o cerrar normalmente. Los PDF protegidos
vuelven a pedir contraseña al abrirlos desde la lista. Si el archivo se movió o no
está disponible, se informa el error y se conserva la entrada: una unidad remota
puede regresar.

El perfil local contiene rutas y posiciones, sin contraseñas, consultas ni texto.
Olvidar el historial no elimina ni modifica documentos. El PDF actual permanece
abierto y esta ventana no vuelve a guardarlo al cerrar; abrir otro PDF reanuda el
historial. Otras ventanas abiertas pueden guardar nuevamente sus documentos al
cambiar de archivo o cerrar.

## Verificación automatizada

`scripts/test-recents.ps1` utiliza un perfil exclusivo bajo `test-results`, dos PDF
sintéticos con el mismo nombre en carpetas distintas y una ruta local inexistente.
No toca el perfil del usuario ni muestra ventanas en el escritorio.

- `recent-test.exe`: orden, límite de diez, documento activo y deduplicación,
  carpetas distintas, escape de `&`, Unicode y truncado sin separar pares UTF-16.
  Comprueba vaciado y reutilización del almacenamiento sin abrir los PDF.
- `Chucao.exe --benchmark-recents PDF INFORME PERFIL`: inspecciona la lista de la
  bienvenida, abre el segundo documento usando sus comandos, verifica el fallo del
  archivo ausente sin eliminarlo, vuelve a abrir el documento y vacía el historial.
  Al terminar se verifica que cerrar no reinserte entradas.
- Se genera una imagen fuera de pantalla de la bienvenida con la lista.
- La suite general verifica además recuperación de posición, escritores simultáneos,
  contraseñas, navegación, selección/búsqueda, índice y enlaces internos.

Pendientes manuales: interacción visible de la lista y la confirmación de borrado,
teclado/foco, DPI y reapertura de documentos reales remotos y protegidos.
