param([string]$Pdf, [string]$Executable = 'Chucao.exe')
$ErrorActionPreference = 'Stop'
$root = [IO.Path]::GetFullPath([IO.Path]::Combine($PSScriptRoot, '..'))
& ([IO.Path]::Combine($PSScriptRoot, 'smoke-test.ps1'))
& ([IO.Path]::Combine($PSScriptRoot, 'test-reading-state.ps1')) -Executable $Executable
& ([IO.Path]::Combine($PSScriptRoot, 'test-recents.ps1')) -Executable $Executable
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
& ([IO.Path]::Combine($root, 'build', 'layout-test.exe'))
if ($LASTEXITCODE -ne 0) { throw 'Falló la geometría del documento.' }
& ([IO.Path]::Combine($root, 'build', 'outline-test.exe')) ([IO.Path]::Combine($root, 'test-results'))
if ($LASTEXITCODE -ne 0) { throw 'Falló el índice del documento.' }
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
foreach ($name in @('sample.pdf', 'mixed-pages.pdf')) {
    & ([IO.Path]::Combine($PSScriptRoot, 'benchmark-open.ps1')) -Executable $Executable -Pdf ([IO.Path]::Combine($root, 'test-results', $name)) -Runs 1 -Navigation
}
if ($Pdf) { & ([IO.Path]::Combine($PSScriptRoot, 'benchmark-open.ps1')) -Executable $Executable -Pdf $Pdf -Runs 1 -Navigation }
& ([IO.Path]::Combine($PSScriptRoot, 'benchmark-open.ps1')) -Executable $Executable -Pdf ([IO.Path]::Combine($root, 'test-results', 'sample.pdf')) -Runs 1 -Text
& ([IO.Path]::Combine($PSScriptRoot, 'benchmark-open.ps1')) -Executable $Executable -Pdf ([IO.Path]::Combine($root, 'test-results', 'rotated-text.pdf')) -Runs 1 -Text
