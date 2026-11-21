# Seguridad

Chucao abre archivos PDF que pueden venir de terceros, así que los errores de
procesamiento (cierres, uso excesivo de memoria, apertura de enlaces) son
relevantes para la seguridad.

## Cómo reportar una vulnerabilidad

Escribí a **jbrugger@ideaworks.com.ar** con una descripción, la versión de
Chucao y, si es posible, un PDF que reproduzca el problema. No abras un issue
público hasta que haya una corrección. Se responde lo antes posible.

## Alcance

- Chucao solo lee: no modifica los PDF ni se conecta a la red; el navegador solo
  se abre al hacer clic en un enlace web.
- El motor de renderizado es [PDFium](docs/pdfium-update.md). Los fallos del
  propio PDFium conviene reportarlos también al proyecto original.
- Versiones soportadas: la última publicada.
