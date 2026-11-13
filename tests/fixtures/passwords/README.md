# PDF sintéticos de contraseñas

Estos archivos se generaron con el PDF sintético de `scripts/smoke-test.ps1`.
No provienen de la biblioteca del usuario. Se incluyen en Git para poder probar
PDFium sin Python, red ni herramientas de edición en cada PC.

Las claves siguientes son **datos públicos de prueba**, no credenciales reales:

| Archivo | Contraseña de lectura | Contraseña de propietario | Cifrado |
| --- | --- | --- | --- |
| aes256-ascii.pdf | Chucao-test | Chucao-owner | AES-256 |
| aes256-unicode.pdf | Chucao-café-漢字 | Chucao-owner | AES-256 |
| aes128.pdf | Chucao-test | Chucao-owner | AES-128 |
| rc4.pdf | Chucao-test | Chucao-owner | RC4-128, compatibilidad con archivos antiguos |
| restricted.pdf | reader | owner | AES-256, copia e impresión restringidas al lector |

Regeneración opcional: `py tests/fixtures/passwords/generate.py` después de generar
sample.pdf. Requiere pypdf 6.1.1 y cryptography 46.0.3; pueden instalarse solo en
`.tools/pdf-test-python`, excluido de Git. No son dependencias de Chucao. El cifrado
usa aleatoriedad: regenerar cambia los binarios aunque mantenga su contenido.

API usada: https://pypdf.readthedocs.io/en/6.1.1/user/encryption-decryption.html
