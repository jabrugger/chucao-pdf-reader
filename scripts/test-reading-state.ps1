$ErrorActionPreference = 'Stop'
$root = [IO.Path]::GetFullPath([IO.Path]::Combine($PSScriptRoot, '..'))
$folder = [IO.Path]::Combine($root, 'test-results', ('state-' + [Guid]::NewGuid().ToString('N')))
[IO.Directory]::CreateDirectory($folder) | Out-Null
$source = [IO.Path]::Combine($root, 'test-results', 'sample.pdf')
if (![IO.File]::Exists($source)) { throw 'Ejecutar smoke-test.ps1 primero.' }
& ([IO.Path]::Combine($root, 'build', 'reading-state-test.exe')) ([IO.Path]::Combine($folder, 'unit'))
if ($LASTEXITCODE -ne 0) { throw 'Falló el almacenamiento del estado de lectura.' }
$fileA = [IO.Path]::Combine($folder, 'lectura-café.pdf')
$fileB = [IO.Path]::Combine($folder, 'otro.pdf')
$protected = [IO.Path]::Combine($folder, 'protegido.pdf')
[IO.File]::Copy($source, $fileA)
[IO.File]::Copy($source, $fileB)
[IO.File]::Copy([IO.Path]::Combine($root, 'tests', 'fixtures', 'passwords', 'aes256-ascii.pdf'), $protected)
$state = [IO.Path]::Combine($folder, 'reading-state.dat')
$exe = [IO.Path]::Combine($root, 'build', 'Chucao.exe')
$sequence = @(
    @{mode='write'; pdf=$fileA}, @{mode='read'; pdf=$fileA},
    @{mode='write'; pdf=$fileB}, @{mode='read'; pdf=$fileA}, @{mode='read'; pdf=$fileB},
    @{mode='write'; pdf=$protected}, @{mode='read'; pdf=$protected}
)
foreach ($step in $sequence) {
    $report = [IO.Path]::Combine($folder, ([Guid]::NewGuid().ToString('N') + '.json'))
    $before = if ($step.mode -eq 'read') { [IO.File]::ReadAllText($state) } else { '' }
    $arguments = @(('--benchmark-state-' + $step.mode), ('"' + $step.pdf + '"'), ('"' + $report + '"'), ('"' + $state + '"'))
    $process = Start-Process -FilePath $exe -ArgumentList $arguments -WindowStyle Hidden -PassThru
    if (!$process.WaitForExit(35000)) { $process.Kill(); $process.WaitForExit(); throw 'La prueba de recuperación excedió 35 segundos.' }
    if (![IO.File]::Exists($report)) { throw 'No se generó el informe de recuperación.' }
    $result = [IO.File]::ReadAllText($report) | ConvertFrom-Json
    if (!$result.success -or $result.state_checks -lt 2) { throw 'Falló el guardado o la recuperación de posición.' }
    if ($step.pdf -eq $protected -and $result.state_checks -lt 3) { throw 'El PDF protegido no volvió a solicitar contraseña.' }
    if ($step.mode -eq 'read' -and $before -cne [IO.File]::ReadAllText($state)) { throw 'La prueba de lectura alteró el perfil.' }
    if ([IO.File]::ReadAllText($state).Contains('Chucao-test')) { throw 'El perfil contiene la contraseña de prueba.' }
    [PSCustomObject]@{mode=$step.mode; checks=$result.state_checks; ready_ms=$result.ready_ms}
}
