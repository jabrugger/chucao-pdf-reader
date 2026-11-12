param(
    [Parameter(Mandatory=$true)][string]$Pdf,
    [int]$Runs = 5,
    [string]$Executable = 'Chucao.exe',
    [switch]$ShellOnly,
    [switch]$Navigation,
    [switch]$Text
)
$ErrorActionPreference = 'Stop'
if ($Runs -lt 1 -or $Runs -gt 20) { throw 'Runs debe estar entre 1 y 20.' }
$root = [IO.Path]::GetFullPath([IO.Path]::Combine($PSScriptRoot, '..'))
$output = [IO.Path]::Combine($root, 'test-results')
[IO.Directory]::CreateDirectory($output) | Out-Null
$exe = [IO.Path]::Combine($root, 'build', $Executable)
for ($i = 1; $i -le $Runs; $i++) {
    $report = [IO.Path]::Combine($output, "open-$([Guid]::NewGuid().ToString('N')).json")
    $clock = [Diagnostics.Stopwatch]::StartNew()
    $arguments = if ($ShellOnly) { @('--benchmark-shell', ('"' + $report + '"')) } else { @($(if ($Text) { '--benchmark-text' } elseif ($Navigation) { '--benchmark-navigation' } else { '--benchmark-open' }), ('"' + $Pdf + '"'), ('"' + $report + '"')) }
    $process = Start-Process -FilePath $exe -ArgumentList $arguments -WindowStyle Hidden -PassThru
    if (!$process.WaitForExit(35000)) {
        # Only terminate the diagnostic child created by this script.
        $process.Kill()
        $process.WaitForExit()
        throw 'Benchmark excedió 35 segundos; se cerró su proceso de diagnóstico.'
    }
    $clock.Stop()
    if (![IO.File]::Exists($report)) { throw "No se generó informe; exit=$($process.ExitCode)." }
    $result = [IO.File]::ReadAllText($report) | ConvertFrom-Json
    if (!$result.success) { throw 'El lector no pudo abrir/renderizar el documento.' }
    [PSCustomObject]@{exe=$Executable; run=$i; process_ms=[math]::Round($clock.Elapsed.TotalMilliseconds, 2); shell_ms=$result.shell_ms; opened_ms=$result.opened_ms; ready_ms=$result.ready_ms; pages=$result.pages; navigation_steps=$result.navigation_steps; cache_hits=$result.cache_hits; cache_bytes=$result.cache_bytes; text_checks=$result.text_checks}
}
