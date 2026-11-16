param([string]$Executable = 'Chucao.exe')
$ErrorActionPreference = 'Stop'
$root = [IO.Path]::GetFullPath([IO.Path]::Combine($PSScriptRoot, '..'))
$folder = [IO.Path]::Combine($root, 'test-results', ('recents-' + [Guid]::NewGuid().ToString('N')))
foreach ($name in @('A', 'B')) {
    [IO.Directory]::CreateDirectory([IO.Path]::Combine($folder, $name)) | Out-Null
    [IO.File]::Copy([IO.Path]::Combine($root, 'test-results', 'sample.pdf'), [IO.Path]::Combine($folder, $name, 'libro.pdf'))
}
& ([IO.Path]::Combine($root, 'build', 'recent-test.exe')) $folder
if ($LASTEXITCODE -ne 0) { throw 'Falló la prueba de recientes.' }
$profile = [IO.Path]::Combine($folder, 'reading-state.dat')
$report = [IO.Path]::Combine($folder, 'report.json')
$arguments = @('--benchmark-recents', ('"' + [IO.Path]::Combine($folder, 'A', 'libro.pdf') + '"'), ('"' + $report + '"'), ('"' + $profile + '"'))
$process = Start-Process -FilePath ([IO.Path]::Combine($root, 'build', $Executable)) -ArgumentList $arguments -WindowStyle Hidden -PassThru
if (!$process.WaitForExit(35000)) { $process.Kill(); $process.WaitForExit(); throw 'Recientes excedió 35 segundos.' }
$result = [IO.File]::ReadAllText($report) | ConvertFrom-Json
if (!$result.success -or $result.recent_checks -ne 5) { throw 'Falló el menú, la apertura o el borrado del historial.' }
if ([IO.File]::ReadAllText($profile) -cne "ChucaoReadingState 1`n") { throw 'Cerrar volvió a guardar el historial borrado.' }
$result
