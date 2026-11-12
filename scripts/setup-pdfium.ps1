$ErrorActionPreference = 'Stop'
$projectRoot = [IO.Path]::GetFullPath([IO.Path]::Combine($PSScriptRoot, '..'))
$dependencyRoot = [IO.Path]::Combine($projectRoot, 'deps')
$destination = [IO.Path]::Combine($dependencyRoot, 'pdfium')
$archive = [IO.Path]::Combine($dependencyRoot, 'pdfium-win-x64-8152.tgz')
$expected = 'cee2b0e9990bc67c0721774932961b9dbf2766046c10eec925acd4cfba6fdfe3'
$url = 'https://github.com/bblanchon/pdfium-binaries/releases/download/chromium/8152/pdfium-win-x64.tgz'
[IO.Directory]::CreateDirectory($destination) | Out-Null
if (![IO.File]::Exists($archive)) {
    Write-Host 'Descargando PDFium chromium/8152 x64 (sin V8)...'
    $client = New-Object Net.WebClient
    try { $client.DownloadFile($url, $archive) } finally { $client.Dispose() }
}
$stream = [IO.File]::OpenRead($archive)
$sha = [Security.Cryptography.SHA256]::Create()
try { $actual = [BitConverter]::ToString($sha.ComputeHash($stream)).Replace('-', '').ToLowerInvariant() }
finally { $stream.Dispose(); $sha.Dispose() }
if ($actual -ne $expected) { throw 'SHA-256 incorrecto. No se extrae ni ejecuta la descarga.' }
& tar.exe -xzf $archive -C $destination
if ($LASTEXITCODE -ne 0) { throw 'No se pudo extraer PDFium.' }
if (![IO.File]::Exists([IO.Path]::Combine($destination, 'include', 'fpdfview.h'))) {
    throw 'El archivo de PDFium no tiene la estructura esperada.'
}
Write-Host 'PDFium descargado y verificado.'
