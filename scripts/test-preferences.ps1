param([string]$Executable = 'Chucao.exe')
$ErrorActionPreference = 'Stop'
$root = [IO.Path]::GetFullPath([IO.Path]::Combine($PSScriptRoot, '..'))
$folder = [IO.Path]::Combine($root, 'test-results', ('preferences-' + [Guid]::NewGuid().ToString('N')))
[IO.Directory]::CreateDirectory($folder) | Out-Null
$exe = [IO.Path]::Combine($root, 'build', $Executable)
& ([IO.Path]::Combine([IO.Path]::GetDirectoryName($exe), 'preferences-test.exe')) ([IO.Path]::Combine($folder, 'unit'))
if ($LASTEXITCODE -ne 0) { throw 'Falló el almacenamiento de preferencias/sesión.' }
$a = [IO.Path]::Combine($folder, 'lectura-café.pdf')
$b = [IO.Path]::Combine($folder, 'otro.pdf')
[IO.File]::Copy([IO.Path]::Combine($root, 'test-results', 'sample.pdf'), $a)
[IO.File]::Copy([IO.Path]::Combine($root, 'test-results', 'mixed-pages.pdf'), $b)
$settings = [IO.Path]::Combine($folder, 'settings.dat')
[IO.File]::WriteAllText($settings, "ChucaoSettings 1`ntheme=light`nrestore=1`n")
foreach ($mode in @('write', 'read', 'off')) {
    if ($mode -eq 'off') { [IO.File]::WriteAllText($settings, "ChucaoSettings 1`ntheme=dark`nrestore=0`n") }
    $report = [IO.Path]::Combine($folder, ($mode + '.json'))
    $arguments = if ($mode -eq 'write') { @('--benchmark-session-write', ('"'+$a+'"'), ('"'+$b+'"'), ('"'+$folder+'"'), ('"'+$report+'"')) } else {
        @(('--benchmark-session-' + $mode), ('"'+$folder+'"'), ('"'+$report+'"'))
    }
    $p = Start-Process -FilePath $exe -ArgumentList $arguments -WindowStyle Hidden -PassThru
    if (!$p.WaitForExit(35000)) { $p.Kill(); $p.WaitForExit(); throw 'La prueba de sesión excedió 35 segundos.' }
    $result = [IO.File]::ReadAllText($report) | ConvertFrom-Json
    if (!$result.success -or $result.session_checks -lt 2) { throw "Falló la sesión en modo $mode. Informe: $report" }
    [PSCustomObject]@{mode=$mode; success=$result.success; checks=$result.session_checks; report=$report}
}
