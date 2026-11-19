param([switch]$SkipPackage)
$ErrorActionPreference = 'Stop'
$root = [IO.Path]::GetFullPath([IO.Path]::Combine($PSScriptRoot, '..'))
$info = [IO.File]::ReadAllText([IO.Path]::Combine($root, 'src', 'app-info.h'))
$match = [Text.RegularExpressions.Regex]::Match($info, '#define\s+CHUCAO_VERSION_STRING\s+"([^"]+)"')
if (!$match.Success) { throw 'No se pudo leer la versión desde src/app-info.h.' }
$version = $match.Groups[1].Value
$portable = [IO.Path]::Combine($root, 'dist', "Chucao-$version-win-x64")
if (!$SkipPackage -or !(Test-Path ([IO.Path]::Combine($portable, 'Chucao.exe')))) {
    & ([IO.Path]::Combine($PSScriptRoot, 'package.ps1'))
    if ($LASTEXITCODE -ne 0) { throw 'Falló el empaquetado portable.' }
}
if (!(Test-Path ([IO.Path]::Combine($portable, 'Chucao.exe')))) {
    throw "Falta la carpeta portable $portable."
}
$candidates = @(
    'C:\Program Files\Inno Setup 7\ISCC.exe',
    'C:\Program Files (x86)\Inno Setup 7\ISCC.exe',
    'C:\Program Files\Inno Setup 6\ISCC.exe',
    'C:\Program Files (x86)\Inno Setup 6\ISCC.exe'
)
$iscc = $candidates | Where-Object { Test-Path $_ } | Select-Object -First 1
if (!$iscc) {
    $command = Get-Command iscc.exe -ErrorAction SilentlyContinue
    if ($command) { $iscc = $command.Source }
}
if (!$iscc) { throw 'No se encontró Inno Setup (ISCC.exe).' }
$script = [IO.Path]::Combine($root, 'installer', 'Chucao.iss')
& $iscc "/DMyVersion=$version" $script
if ($LASTEXITCODE -ne 0) { throw 'Falló la compilación del instalador.' }
$output = [IO.Path]::Combine($root, 'dist', "Chucao-$version-setup.exe")
if (!(Test-Path $output)) { throw "No se generó $output." }
$hash = (Get-FileHash -Algorithm SHA256 -LiteralPath $output).Hash.ToLowerInvariant()
$size = [math]::Round((Get-Item -LiteralPath $output).Length / 1MB, 2)
Write-Host "Instalador: $output"
Write-Host "Tamaño: $size MiB"
Write-Host "SHA-256: $hash"
