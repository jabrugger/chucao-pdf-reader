param([string]$Pdf, [string]$Executable = 'Chucao.exe')
$ErrorActionPreference = 'Stop'
$root = [IO.Path]::GetFullPath([IO.Path]::Combine($PSScriptRoot, '..'))
& ([IO.Path]::Combine($PSScriptRoot, 'smoke-test.ps1'))
& ([IO.Path]::Combine($PSScriptRoot, 'test-reading-state.ps1')) -Executable $Executable
& ([IO.Path]::Combine($PSScriptRoot, 'test-recents.ps1')) -Executable $Executable
& ([IO.Path]::Combine($PSScriptRoot, 'test-preferences.ps1')) -Executable $Executable
$windowProfile = [IO.Path]::Combine($root, 'test-results', 'window-settings.dat')
Remove-Item -LiteralPath $windowProfile -ErrorAction SilentlyContinue
$windowReport = [IO.Path]::Combine($root, 'test-results', 'window.json')
$windowProcess = Start-Process -FilePath ([IO.Path]::Combine($root, 'build', $Executable)) -ArgumentList @('--benchmark-window', 'test-results\window-settings.dat', 'test-results\window.json') -WorkingDirectory $root -WindowStyle Hidden -PassThru
if (!$windowProcess.WaitForExit(15000)) { $windowProcess.Kill(); throw 'La prueba de posición de ventana excedió 15 segundos.' }
if ($windowProcess.ExitCode -ne 0) { throw 'Falló la prueba de posición de ventana.' }
$windowResult = [IO.File]::ReadAllText($windowReport) | ConvertFrom-Json
if (!$windowResult.success) { throw 'La posición/tamaño de ventana no se guardó o restauró correctamente.' }
$robustFolder = 'test-results\robustness-' + [Guid]::NewGuid().ToString('N')
$robustProcess = Start-Process -FilePath ([IO.Path]::Combine($root, 'build', 'robustness-test.exe')) -ArgumentList @($robustFolder) -WorkingDirectory $root -WindowStyle Hidden -PassThru
if (!$robustProcess.WaitForExit(30000)) { $robustProcess.Kill(); throw 'La prueba de PDF dañados/extremos excedió 30 segundos.' }
if ($robustProcess.ExitCode -ne 0) { throw 'Falló la prueba de PDF dañados o dimensiones extremas.' }
$passwordFixtures = [IO.Path]::Combine($root, 'tests', 'fixtures', 'passwords')
& ([IO.Path]::Combine($root, 'build', 'password-test.exe')) $passwordFixtures
if ($LASTEXITCODE -ne 0) { throw 'Falló la validación de contraseñas del motor.' }
$passwordOutput = [IO.Path]::Combine($root, 'test-results', 'passwords')
[IO.Directory]::CreateDirectory($passwordOutput) | Out-Null
foreach ($name in @('aes256-ascii.pdf', 'aes256-unicode.pdf', 'aes128.pdf', 'rc4.pdf', 'restricted.pdf')) {
    [IO.File]::Copy([IO.Path]::Combine($passwordFixtures, $name), [IO.Path]::Combine($passwordOutput, $name), $true)
}
[IO.File]::Copy([IO.Path]::Combine($root, 'test-results', 'sample.pdf'), [IO.Path]::Combine($passwordOutput, 'plain.pdf'), $true)
& ([IO.Path]::Combine($PSScriptRoot, 'benchmark-open.ps1')) -Executable $Executable -Pdf ([IO.Path]::Combine($passwordOutput, 'aes256-ascii.pdf')) -Runs 1 -Passwords
& ([IO.Path]::Combine($root, 'build', 'messages-test.exe'))
if ($LASTEXITCODE -ne 0) { throw 'Fallaron los mensajes de error.' }
$results = [IO.Path]::Combine($root, 'test-results')
$themeReport = [IO.Path]::Combine($results, 'theme.json')
$themeProfile = [IO.Path]::Combine($results, 'theme-settings.dat')
$themeProcess = Start-Process -FilePath ([IO.Path]::Combine($root, 'build', $Executable)) -ArgumentList @('--benchmark-theme', 'test-results\theme-settings.dat', 'test-results\theme.json') -WorkingDirectory $root -WindowStyle Hidden -PassThru
if (!$themeProcess.WaitForExit(15000)) { $themeProcess.Kill(); throw 'La prueba de temas excedió 15 segundos.' }
if ($themeProcess.ExitCode -ne 0) { throw 'Falló la prueba de temas.' }
$themeResult = [IO.File]::ReadAllText($themeReport) | ConvertFrom-Json
if (!$themeResult.success) { throw 'El cambio de tema o su persistencia no coincidió.' }
$dpiReport = [IO.Path]::Combine($results, 'dpi.json')
$dpiProcess = Start-Process -FilePath ([IO.Path]::Combine($root, 'build', $Executable)) -ArgumentList @('--benchmark-dpi', 'test-results\dpi.json') -WorkingDirectory $root -WindowStyle Hidden -PassThru
if (!$dpiProcess.WaitForExit(15000)) { $dpiProcess.Kill(); throw 'La prueba de DPI excedió 15 segundos.' }
if ($dpiProcess.ExitCode -ne 0) { throw 'Falló la prueba de DPI.' }
$dpiResult = [IO.File]::ReadAllText($dpiReport) | ConvertFrom-Json
if (!$dpiResult.success) { throw 'Las métricas de interfaz no se adaptaron a 144/192 DPI.' }
$keysReport = [IO.Path]::Combine($results, 'keys.json')
$keysProcess = Start-Process -FilePath ([IO.Path]::Combine($root, 'build', $Executable)) -ArgumentList @('--benchmark-keys', 'test-results\keys.json') -WorkingDirectory $root -WindowStyle Hidden -PassThru
if (!$keysProcess.WaitForExit(15000)) { $keysProcess.Kill(); throw 'La prueba de teclado excedió 15 segundos.' }
if ($keysProcess.ExitCode -ne 0) { throw 'Falló la prueba de teclado.' }
$keysResult = [IO.File]::ReadAllText($keysReport) | ConvertFrom-Json
if (!$keysResult.success) { throw 'El orden de tabulación, el foco o los nombres accesibles no coincidieron.' }
$hcReport = [IO.Path]::Combine($results, 'highcontrast.json')
$hcProcess = Start-Process -FilePath ([IO.Path]::Combine($root, 'build', $Executable)) -ArgumentList @('--benchmark-highcontrast', 'test-results\highcontrast.json') -WorkingDirectory $root -WindowStyle Hidden -PassThru
if (!$hcProcess.WaitForExit(15000)) { $hcProcess.Kill(); throw 'La prueba de alto contraste excedió 15 segundos.' }
if ($hcProcess.ExitCode -ne 0) { throw 'Falló la prueba de alto contraste.' }
$hcResult = [IO.File]::ReadAllText($hcReport) | ConvertFrom-Json
if (!$hcResult.success) { throw 'La paleta de alto contraste no usó los colores del sistema.' }
$rapidReport = [IO.Path]::Combine($results, 'rapid.json')
$rapidProcess = Start-Process -FilePath ([IO.Path]::Combine($root, 'build', $Executable)) -ArgumentList @('--benchmark-rapid', 'test-results\sample.pdf', 'test-results\mixed-pages.pdf', 'test-results\rapid.json') -WorkingDirectory $root -WindowStyle Hidden -PassThru
if (!$rapidProcess.WaitForExit(15000)) { $rapidProcess.Kill(); throw 'La prueba de apertura consecutiva excedió 15 segundos.' }
if ($rapidProcess.ExitCode -ne 0) { throw 'Falló la prueba de apertura consecutiva.' }
$rapidResult = [IO.File]::ReadAllText($rapidReport) | ConvertFrom-Json
if (!$rapidResult.success -or $rapidResult.pages -ne 4) { throw 'La apertura consecutiva no conservó el segundo documento.' }
& ([IO.Path]::Combine($PSScriptRoot, 'test-tabs.ps1')) -Executable $Executable
& ([IO.Path]::Combine($root, 'build', 'layout-test.exe'))
if ($LASTEXITCODE -ne 0) { throw 'Falló la geometría del documento.' }
$startup = & ([IO.Path]::Combine($PSScriptRoot, 'benchmark-open.ps1')) -Executable $Executable -Pdf ([IO.Path]::Combine($root, 'test-results', 'sample.pdf')) -Runs 1 -Startup
if (!$startup -or $startup.first_bitmap_ms -le 0 -or $startup.first_compose_ms -lt $startup.first_bitmap_ms) { throw 'Falló la medición del arranque.' }
& ([IO.Path]::Combine($root, 'build', 'outline-test.exe')) ([IO.Path]::Combine($root, 'test-results'))
if ($LASTEXITCODE -ne 0) { throw 'Falló el índice del documento.' }
$filterReport = [IO.Path]::Combine($results, 'outline-filter.json')
$filterProcess = Start-Process -FilePath ([IO.Path]::Combine($root, 'build', $Executable)) -ArgumentList @('--benchmark-outline-filter', 'test-results\outline.pdf', 'test-results\outline-filter.json') -WorkingDirectory $root -WindowStyle Hidden -PassThru
if (!$filterProcess.WaitForExit(15000)) { $filterProcess.Kill(); throw 'La prueba del filtro del índice excedió 15 segundos.' }
if ($filterProcess.ExitCode -ne 0) { throw 'Falló la prueba del filtro del índice.' }
$filterResult = [IO.File]::ReadAllText($filterReport) | ConvertFrom-Json
if (!$filterResult.success) { throw 'El filtro del índice no conservó padres, recuento o limpieza.' }
& ([IO.Path]::Combine($PSScriptRoot, 'benchmark-open.ps1')) -Executable $Executable -Pdf ([IO.Path]::Combine($root, 'test-results', 'outline.pdf')) -Runs 1 -Interface
& ([IO.Path]::Combine($root, 'build', 'links-test.exe')) ([IO.Path]::Combine($root, 'test-results'))
if ($LASTEXITCODE -ne 0) { throw 'Fallaron los enlaces internos.' }
& ([IO.Path]::Combine($PSScriptRoot, 'benchmark-open.ps1')) -Executable $Executable -Pdf ([IO.Path]::Combine($root, 'test-results', 'links.pdf')) -Runs 1 -Links
& ([IO.Path]::Combine($PSScriptRoot, 'benchmark-open.ps1')) -Executable $Executable -Pdf ([IO.Path]::Combine($root, 'test-results', 'rotated-links.pdf')) -Runs 1 -Links
& ([IO.Path]::Combine($PSScriptRoot, 'benchmark-open.ps1')) -Executable $Executable -Pdf ([IO.Path]::Combine($root, 'test-results', 'outline.pdf')) -Runs 1 -Outline
& ([IO.Path]::Combine($PSScriptRoot, 'benchmark-open.ps1')) -Executable $Executable -Pdf ([IO.Path]::Combine($root, 'test-results', 'rotated-outline.pdf')) -Runs 1 -Outline
foreach ($mode in @('Fit', 'FitH-200', 'FitV-150', 'XYZ-150-200-2', 'XYZ-150-200-0', 'FitR-50-100-300-400', 'FitB', 'FitBH-200', 'FitBV-150')) {
    & ([IO.Path]::Combine($PSScriptRoot, 'benchmark-open.ps1')) -Executable $Executable -Pdf ([IO.Path]::Combine($root, 'test-results', ('destination-' + $mode + '.pdf'))) -Runs 1 -Destinations
    & ([IO.Path]::Combine($PSScriptRoot, 'benchmark-open.ps1')) -Executable $Executable -Pdf ([IO.Path]::Combine($root, 'test-results', ('rotated-destination-' + $mode + '.pdf'))) -Runs 1 -Destinations
}
$fitReport = [IO.Path]::Combine($results, 'fit.json')
$fitProcess = Start-Process -FilePath ([IO.Path]::Combine($root, 'build', $Executable)) -ArgumentList @('--benchmark-fit', 'test-results\mixed-pages.pdf', 'test-results\fit.json') -WorkingDirectory $root -WindowStyle Hidden -PassThru
if (!$fitProcess.WaitForExit(15000)) { $fitProcess.Kill(); throw 'La prueba de ajuste de página/alto excedió 15 segundos.' }
if ($fitProcess.ExitCode -ne 0) { throw 'Falló la prueba de ajuste de página/alto.' }
$fitResult = [IO.File]::ReadAllText($fitReport) | ConvertFrom-Json
if (!$fitResult.success) { throw 'Los modos ajustar a página, alto y ancho no coincidieron.' }
foreach ($name in @('sample.pdf', 'mixed-pages.pdf')) {
    & ([IO.Path]::Combine($PSScriptRoot, 'benchmark-open.ps1')) -Executable $Executable -Pdf ([IO.Path]::Combine($root, 'test-results', $name)) -Runs 1 -Navigation
}
if ($Pdf) { & ([IO.Path]::Combine($PSScriptRoot, 'benchmark-open.ps1')) -Executable $Executable -Pdf $Pdf -Runs 1 -Navigation }
& ([IO.Path]::Combine($PSScriptRoot, 'benchmark-open.ps1')) -Executable $Executable -Pdf ([IO.Path]::Combine($root, 'test-results', 'sample.pdf')) -Runs 1 -Text
& ([IO.Path]::Combine($PSScriptRoot, 'benchmark-open.ps1')) -Executable $Executable -Pdf ([IO.Path]::Combine($root, 'test-results', 'rotated-text.pdf')) -Runs 1 -Text
