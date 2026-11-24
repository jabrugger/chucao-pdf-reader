param(
    [string]$Version,
    [switch]$SkipBuild
)
$ErrorActionPreference = 'Stop'
$root = [IO.Path]::GetFullPath([IO.Path]::Combine($PSScriptRoot, '..'))
$info = [IO.File]::ReadAllText([IO.Path]::Combine($root, 'src', 'app-info.h'))
if (!$Version) {
    $match = [Text.RegularExpressions.Regex]::Match($info, '#define\s+CHUCAO_VERSION_STRING\s+"([^"]+)"')
    if (!$match.Success) { throw 'No se pudo leer la versión desde src/app-info.h.' }
    $Version = $match.Groups[1].Value
}
if ($Version -notmatch '^[0-9]+(\.[0-9]+){1,3}(-[0-9A-Za-z.-]+)?$') { throw "Versión no válida: $Version" }
$executable = [IO.Path]::Combine($root, 'build', 'Chucao.exe')
$engine = [IO.Path]::Combine($root, 'build', 'pdfium.dll')
if (!$SkipBuild) {
    & cmd.exe /c ([IO.Path]::Combine($PSScriptRoot, 'build.cmd'))
    if ($LASTEXITCODE -ne 0) { throw 'Falló la compilación.' }
}
foreach ($file in @($executable, $engine)) {
    if (![IO.File]::Exists($file)) { throw "Falta $file. Compilar o revisar scripts/setup-pdfium.ps1." }
}
$licenseRoot = [IO.Path]::Combine($root, 'deps', 'pdfium', 'licenses')
$engineLicense = [IO.Path]::Combine($root, 'deps', 'pdfium', 'LICENSE')
if (![IO.Directory]::Exists($licenseRoot)) {
    throw 'Faltan los avisos de PDFium en deps/pdfium. Ejecutar scripts/setup-pdfium.ps1.'
}
if (![IO.File]::Exists($engineLicense)) {
    throw 'Falta la licencia de PDFium en deps/pdfium. Ejecutar scripts/setup-pdfium.ps1.'
}
foreach ($required in @('LICENSE', 'THIRD-PARTY-NOTICES.md')) {
    if (![IO.File]::Exists([IO.Path]::Combine($root, $required))) { throw "Falta $required." }
}
$name = "Chucao-$Version-win-x64"
$dist = [IO.Path]::Combine($root, 'dist')
$folder = [IO.Path]::Combine($dist, $name)
$zip = [IO.Path]::Combine($dist, "$name.zip")
[IO.Directory]::CreateDirectory($dist) | Out-Null
if ([IO.Directory]::Exists($folder)) { Remove-Item -LiteralPath $folder -Recurse -Force }
[IO.Directory]::CreateDirectory($folder) | Out-Null
Copy-Item -LiteralPath $executable -Destination $folder
Copy-Item -LiteralPath $engine -Destination $folder
Copy-Item -LiteralPath ([IO.Path]::Combine($root, 'LICENSE')) -Destination $folder
Copy-Item -LiteralPath ([IO.Path]::Combine($root, 'THIRD-PARTY-NOTICES.md')) -Destination $folder
$destinationLicenses = [IO.Path]::Combine($folder, 'licenses')
[IO.Directory]::CreateDirectory($destinationLicenses) | Out-Null
Copy-Item -Path ([IO.Path]::Combine($licenseRoot, '*')) -Destination $destinationLicenses
Copy-Item -LiteralPath $engineLicense -Destination ([IO.Path]::Combine($destinationLicenses, 'pdfium-LICENSE.txt'))
$readme = @"
Chucao PDF Reader $Version - portable Windows x64
=================================================

Requisitos
  Windows 10 u 11 de 64 bits. No requiere instalador ni el redistribuible de
  Visual C++.

Inicio
  Ejecutar Chucao.exe. pdfium.dll debe permanecer en la misma carpeta: es el
  motor que lee los PDF. No hay asociaciones de archivos, servicios ni claves
  de registro.

Funciones
  Abrir con Ctrl+O, arrastrar un PDF o pasar su ruta como argumento. El botón
  Abrir incluye los archivos recientes y la opción de olvidar el historial.
  Índice, búsqueda, selección/copia, impresión y enlaces internos o web se
  describen en el README del repositorio.

Datos locales
  %LOCALAPPDATA%\ChucaoPDFReader\reading-state.dat guarda las rutas recientes y
  la posición de lectura. No contiene contraseñas ni el texto del documento.
  Se puede borrar desde Abrir -> Olvidar historial y posiciones, o eliminar el
  archivo con Chucao cerrado.

Privacidad
  El visor no tiene telemetría. Con la opción "Buscar actualizaciones al abrir"
  (desactivada por defecto) se consulta una sola vez la última release en GitHub,
  sin descargar ni instalar nada y sin enviar identificadores. Solo un clic
  explícito en un enlace HTTP/HTTPS abre el navegador predeterminado.

Licencias
  Código propio: MIT (LICENSE).
  PDFium y sus dependencias: ver THIRD-PARTY-NOTICES.md y la carpeta licenses.

Código fuente
  https://github.com/jabrugger/chucao-pdf-reader

Este paquete no está firmado digitalmente; Windows puede mostrar un aviso de
SmartScreen al abrirlo en otra PC.
"@
[IO.File]::WriteAllText([IO.Path]::Combine($folder, 'LEEME.txt'), ($readme -replace "`r?`n", "`r`n"), (New-Object Text.UTF8Encoding($false)))
if ([IO.File]::Exists($zip)) { Remove-Item -LiteralPath $zip -Force }
Compress-Archive -LiteralPath $folder -DestinationPath $zip -CompressionLevel Optimal
$hash = (Get-FileHash -Algorithm SHA256 -LiteralPath $zip).Hash.ToLowerInvariant()
$size = [math]::Round((Get-Item -LiteralPath $zip).Length / 1MB, 2)
Write-Host "Paquete portable: $zip"
Write-Host "Tamaño: $size MiB"
Write-Host "SHA-256: $hash"
