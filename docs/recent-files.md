# Archivos recientes

El botón **Abrir ▾** abre un menú nativo con **Abrir PDF…**, hasta diez documentos
recientes y **Olvidar historial y posiciones…**. Ctrl+O abre directamente el selector;
Ctrl+Shift+O abre el menú. Las teclas 1–9 y 0 seleccionan entradas dentro del menú.
Cada entrada incluye nombre y carpeta, con truncado central para rutas largas.

La lista reutiliza el perfil de [estado de lectura](reading-state.md), que conserva
hasta 128 documentos. El documento actual aparece primero una vez renderizado;
su posición se guarda al cambiar de archivo o cerrar normalmente. La lista se lee
solo al pedir el menú: no se recorren carpetas ni se consulta la existencia de los
PDF. El arranque vacío sigue sin cargar PDFium.

Elegir una entrada utiliza la apertura normal y recupera su posición. Los PDF
protegidos vuelven a pedir contraseña. Si el archivo se movió o no está disponible,
se informa el error y se conserva la entrada: una unidad remota puede regresar.

El perfil local contiene rutas y posiciones, sin contraseñas, consultas ni texto.
**Olvidar historial y posiciones…** pide confirmación y vacía ese perfil mediante
la misma escritura atómica y exclusión entre procesos que el guardado de posición.
No elimina ni modifica documentos. El PDF actual permanece abierto y esta ventana
no vuelve a guardarlo al cerrar; abrir otro PDF reanuda el historial. Otras ventanas
abiertas pueden guardar nuevamente sus documentos al cambiar de archivo o cerrar.

## Verificación automatizada

`scripts/test-recents.ps1` utiliza un perfil exclusivo bajo `test-results`, dos PDF
sintéticos con el mismo nombre en carpetas distintas y una ruta local inexistente.
No toca el perfil del usuario ni muestra ventanas o menús en el escritorio.

- `recent-test.exe`: orden, límite de diez, documento activo y deduplicación,
  carpetas distintas, escape de `&`, Unicode y truncado sin separar pares UTF-16.
  Comprueba vaciado y reutilización del almacenamiento sin abrir los PDF.
- `Chucao.exe --benchmark-recents PDF INFORME PERFIL`: inspecciona el menú nativo,
  abre el segundo documento usando sus comandos, verifica el fallo del archivo
  ausente sin eliminarlo, vuelve a abrir el documento y vacía el historial.
  Cinco comprobaciones; al terminar se verifica que cerrar no reinserte entradas.
- Se genera una imagen fuera de pantalla de la barra y el PDF para inspección.
- La suite general verifica además recuperación de posición, escritores simultáneos,
  contraseñas, navegación, selección/búsqueda, índice y enlaces internos.

Pendientes manuales: interacción visible del menú y confirmación, teclado/foco,
DPI y reapertura de documentos reales remotos y protegidos.
