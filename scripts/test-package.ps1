param([string]$Zip)
$ErrorActionPreference = 'Stop'
$root = [IO.Path]::GetFullPath([IO.Path]::Combine($PSScriptRoot, '..'))
$output = [IO.Path]::Combine($root, 'test-results')
[IO.Directory]::CreateDirectory($output) | Out-Null
if (!$Zip) {
    $candidates = @(Get-ChildItem -Path ([IO.Path]::Combine($root, 'dist')) -Filter 'Chucao-*-win-x64.zip' -ErrorAction SilentlyContinue |
        Sort-Object LastWriteTime -Descending)
    if ($candidates.Count -eq 0) { throw 'No hay paquetes en dist. Ejecutar scripts/package.ps1.' }
    $Zip = $candidates[0].FullName
}
if (![IO.File]::Exists($Zip)) { throw "No existe el paquete: $Zip" }
$check = [IO.Path]::Combine($output, 'package-check')
if ([IO.Directory]::Exists($check)) { Remove-Item -LiteralPath $check -Recurse -Force }
[IO.Directory]::CreateDirectory($check) | Out-Null
Expand-Archive -LiteralPath $Zip -DestinationPath $check
$packaged = @(Get-ChildItem -Path $check -Directory | Select-Object -First 1)
if ($packaged.Count -ne 1) { throw 'El paquete no contiene una carpeta raíz.' }
$folder = $packaged[0].FullName
foreach ($required in @('Chucao.exe', 'pdfium.dll', 'LEEME.txt', 'LICENSE', 'THIRD-PARTY-NOTICES.md',
        'licenses\pdfium.txt', 'licenses\pdfium-LICENSE.txt', 'licenses\icu.txt', 'licenses\zlib.txt')) {
    $path = [IO.Path]::Combine($folder, $required)
    if (![IO.File]::Exists($path)) { throw "Falta $required en el paquete." }
    if ((Get-Item -LiteralPath $path).Length -eq 0) { throw "$required está vacío." }
}
$sample = [IO.Path]::Combine($output, 'sample.pdf')
if (![IO.File]::Exists($sample)) { & ([IO.Path]::Combine($PSScriptRoot, 'smoke-test.ps1')) }
$report = [IO.Path]::Combine($output, 'package-open.json')
$exe = [IO.Path]::Combine($folder, 'Chucao.exe')
$process = Start-Process -FilePath $exe -ArgumentList @('--benchmark-open', ('"' + $sample + '"'), ('"' + $report + '"')) -WindowStyle Hidden -PassThru
if (!$process.WaitForExit(35000)) {
    $process.Kill()
    $process.WaitForExit()
    throw 'El lector empaquetado excedió 35 segundos.'
}
if (![IO.File]::Exists($report)) { throw "El lector empaquetado no generó informe; exit=$($process.ExitCode)." }
$result = [IO.File]::ReadAllText($report) | ConvertFrom-Json
if (!$result.success -or $result.pages -ne 2) { throw 'El lector empaquetado no abrió ni renderizó el PDF de prueba.' }
$aboutReport = [IO.Path]::Combine($output, 'package-about.json')
$aboutProcess = Start-Process -FilePath $exe -ArgumentList @('--preview-about', ('"' + $aboutReport + '"')) -WindowStyle Hidden -PassThru
if (!$aboutProcess.WaitForExit(15000)) {
    $aboutProcess.Kill()
    $aboutProcess.WaitForExit()
    throw 'La vista previa de Acerca de del paquete excedió 15 segundos.'
}
if ($aboutProcess.ExitCode -ne 0 -or ![IO.File]::Exists($aboutReport + '.about.bmp')) {
    throw 'El lector empaquetado no pudo crear Acerca de: falta el manifiesto incrustado (Common Controls v6).'
}
Write-Host "Paquete verificado: $([IO.Path]::GetFileName($Zip)) abre, renderiza, muestra Acerca de y contiene avisos y licencias."
