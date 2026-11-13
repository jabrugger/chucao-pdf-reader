param([string]$Pdf)
$ErrorActionPreference = 'Stop'
$root = [IO.Path]::GetFullPath([IO.Path]::Combine($PSScriptRoot, '..'))
& ([IO.Path]::Combine($PSScriptRoot, 'smoke-test.ps1'))
& ([IO.Path]::Combine($root, 'build', 'layout-test.exe'))
if ($LASTEXITCODE -ne 0) { throw 'Falló la geometría del documento.' }
& ([IO.Path]::Combine($root, 'build', 'outline-test.exe')) ([IO.Path]::Combine($root, 'test-results'))
if ($LASTEXITCODE -ne 0) { throw 'Falló el índice del documento.' }
& ([IO.Path]::Combine($root, 'build', 'links-test.exe')) ([IO.Path]::Combine($root, 'test-results'))
if ($LASTEXITCODE -ne 0) { throw 'Fallaron los enlaces internos.' }
& ([IO.Path]::Combine($PSScriptRoot, 'benchmark-open.ps1')) -Pdf ([IO.Path]::Combine($root, 'test-results', 'links.pdf')) -Runs 1 -Links
& ([IO.Path]::Combine($PSScriptRoot, 'benchmark-open.ps1')) -Pdf ([IO.Path]::Combine($root, 'test-results', 'outline.pdf')) -Runs 1 -Outline
foreach ($name in @('sample.pdf', 'mixed-pages.pdf')) {
    & ([IO.Path]::Combine($PSScriptRoot, 'benchmark-open.ps1')) -Pdf ([IO.Path]::Combine($root, 'test-results', $name)) -Runs 1 -Navigation
}
if ($Pdf) { & ([IO.Path]::Combine($PSScriptRoot, 'benchmark-open.ps1')) -Pdf $Pdf -Runs 1 -Navigation }
& ([IO.Path]::Combine($PSScriptRoot, 'benchmark-open.ps1')) -Pdf ([IO.Path]::Combine($root, 'test-results', 'sample.pdf')) -Runs 1 -Text
& ([IO.Path]::Combine($PSScriptRoot, 'benchmark-open.ps1')) -Pdf ([IO.Path]::Combine($root, 'test-results', 'rotated-text.pdf')) -Runs 1 -Text
