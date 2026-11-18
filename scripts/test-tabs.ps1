param([string]$Executable = 'Chucao.exe')
$ErrorActionPreference = 'Stop'
$root = [IO.Path]::GetFullPath([IO.Path]::Combine($PSScriptRoot, '..'))
$folder = [IO.Path]::Combine($root, 'test-results', ('tabs-' + [Guid]::NewGuid().ToString('N')))
[IO.Directory]::CreateDirectory($folder) | Out-Null
$a = [IO.Path]::Combine($folder, 'lectura-café.pdf')
$b = [IO.Path]::Combine($folder, 'tamaños-distintos.pdf')
$protected = [IO.Path]::Combine($folder, 'protegido.pdf')
[IO.File]::Copy([IO.Path]::Combine($root, 'test-results', 'sample.pdf'), $a)
[IO.File]::Copy([IO.Path]::Combine($root, 'test-results', 'mixed-pages.pdf'), $b)
[IO.File]::Copy([IO.Path]::Combine($root, 'tests', 'fixtures', 'passwords', 'aes256-ascii.pdf'), $protected)
$report = [IO.Path]::Combine($folder, 'report.json')
$arguments = @('--benchmark-tabs', ('"' + $a + '"'), ('"' + $b + '"'), ('"' + $protected + '"'), ('"' + $report + '"'))
$process = Start-Process -FilePath ([IO.Path]::Combine($root, 'build', $Executable)) -ArgumentList $arguments -WindowStyle Hidden -PassThru
if (!$process.WaitForExit(35000)) { $process.Kill(); $process.WaitForExit(); throw 'La prueba de pestañas excedió 35 segundos.' }
if (![IO.File]::Exists($report)) { throw "No se generó informe de pestañas; exit=$($process.ExitCode)." }
$result = [IO.File]::ReadAllText($report) | ConvertFrom-Json
if (!$result.success -or $result.tabs_step -ne 18 -or $result.tabs_checks -lt 27 -or $result.tabs_closed -ne 2 -or $result.tabs_password_prompts -ne 3) {
    throw "Falló la prueba de pestañas: etapa=$($result.tabs_step), comprobaciones=$($result.tabs_checks). Informe: $report"
}
$result
