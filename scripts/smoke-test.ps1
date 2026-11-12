$ErrorActionPreference = 'Stop'
$root = [IO.Path]::GetFullPath([IO.Path]::Combine($PSScriptRoot, '..'))
$output = [IO.Path]::Combine($root, 'test-results')
[IO.Directory]::CreateDirectory($output) | Out-Null
$fixture = [IO.Path]::Combine($output, 'sample.pdf')
$ascii = [Text.Encoding]::ASCII
$content = "BT /F1 24 Tf 72 740 Td (Chucao PDF Reader - smoke test) Tj ET"
$objects = @(
    '<< /Type /Catalog /Pages 2 0 R >>',
    '<< /Type /Pages /Kids [3 0 R 6 0 R] /Count 2 >>',
    '<< /Type /Page /Parent 2 0 R /MediaBox [0 0 612 792] /Resources << /Font << /F1 4 0 R >> >> /Contents 5 0 R >>',
    '<< /Type /Font /Subtype /Type1 /BaseFont /Helvetica >>',
    "<< /Length $($ascii.GetByteCount($content)) >>`nstream`n$content`nendstream",
    '<< /Type /Page /Parent 2 0 R /MediaBox [0 0 612 792] /Resources << /Font << /F1 4 0 R >> >> /Contents 5 0 R >>'
)
$pdf = "%PDF-1.4`n"
$offsets = @()
for ($i=0; $i -lt $objects.Count; $i++) {
    $offsets += $ascii.GetByteCount($pdf)
    $pdf += "$($i+1) 0 obj`n$($objects[$i])`nendobj`n"
}
$xref = $ascii.GetByteCount($pdf)
$pdf += "xref`n0 7`n0000000000 65535 f `n"
foreach ($offset in $offsets) { $pdf += $offset.ToString('0000000000') + " 00000 n `n" }
$pdf += "trailer`n<< /Size 7 /Root 1 0 R >>`nstartxref`n$xref`n%%EOF`n"
[IO.File]::WriteAllBytes($fixture, $ascii.GetBytes($pdf))
$exe = [IO.Path]::Combine($root, 'build', 'pdf-smoke.exe')
& $exe $fixture
if ($LASTEXITCODE -ne 0) { throw 'Falló apertura/renderizado/búsqueda.' }
& $exe ([IO.Path]::Combine($output, 'missing.pdf'))
if ($LASTEXITCODE -ne 1) { throw 'Falló el manejo de archivo inexistente.' }
Write-Host 'Pruebas de apertura, renderizado, búsqueda y archivo inexistente: OK.'
Write-Host 'Contraseñas: ejecutar pdf-smoke con un fixture cifrado; pendiente si no hay generador disponible.'
