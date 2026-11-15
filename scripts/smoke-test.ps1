$ErrorActionPreference = 'Stop'
$root = [IO.Path]::GetFullPath([IO.Path]::Combine($PSScriptRoot, '..'))
$output = [IO.Path]::Combine($root, 'test-results')
[IO.Directory]::CreateDirectory($output) | Out-Null
$fixture = [IO.Path]::Combine($output, 'sample.pdf')
$ascii = [Text.Encoding]::ASCII
$content = 'BT /F1 24 Tf 72 740 Td (Chucao PDF Reader - caf\351 - smoke test) Tj 0 -32 Td (Second line - selection test) Tj ET'
$objects = @(
    '<< /Type /Catalog /Pages 2 0 R >>',
    '<< /Type /Pages /Kids [3 0 R 6 0 R] /Count 2 >>',
    '<< /Type /Page /Parent 2 0 R /MediaBox [0 0 612 792] /Resources << /Font << /F1 4 0 R >> >> /Contents 5 0 R >>',
    '<< /Type /Font /Subtype /Type1 /BaseFont /Helvetica /Encoding /WinAnsiEncoding >>',
    "<< /Length $($ascii.GetByteCount($content)) >>`nstream`n$content`nendstream",
    '<< /Type /Page /Parent 2 0 R /MediaBox [0 0 612 792] /Resources << /Font << /F1 4 0 R >> >> /Contents 5 0 R >>'
)
function Write-PdfFixture([string]$destination, [string[]]$pdfObjects) {
$pdf = "%PDF-1.4`n"
$offsets = @()
for ($i=0; $i -lt $pdfObjects.Count; $i++) {
    $offsets += $ascii.GetByteCount($pdf)
    $pdf += "$($i+1) 0 obj`n$($pdfObjects[$i])`nendobj`n"
}
$xref = $ascii.GetByteCount($pdf)
$size = $pdfObjects.Count + 1
$pdf += "xref`n0 $size`n0000000000 65535 f `n"
foreach ($offset in $offsets) { $pdf += $offset.ToString('0000000000') + " 00000 n `n" }
$pdf += "trailer`n<< /Size $size /Root 1 0 R >>`nstartxref`n$xref`n%%EOF`n"
[IO.File]::WriteAllBytes($destination, $ascii.GetBytes($pdf))
}
Write-PdfFixture $fixture $objects
$linkObjects = @($objects)
$linkObjects[0] = '<< /Type /Catalog /Pages 2 0 R /Dests << /capitulo [6 0 R /XYZ 150 500 null] >> >>'
$linkObjects[2] = $linkObjects[2].Replace('/MediaBox', '/Annots [7 0 R 8 0 R 9 0 R 10 0 R 11 0 R 12 0 R 13 0 R 14 0 R 15 0 R 16 0 R 17 0 R 18 0 R 19 0 R] /MediaBox')
$linkObjects += @(
    '<< /Type /Annot /Subtype /Link /Rect [72 736 160 764] /Border [0 0 0] /Dest [6 0 R /XYZ 150 500 null] >>',
    '<< /Type /Annot /Subtype /Link /Rect [72 700 160 732] /Border [0 0 0] /A << /S /GoTo /D /capitulo >> >>',
    '<< /Type /Annot /Subtype /Link /Rect [72 640 160 670] /A << /S /URI /URI (https://example.invalid) >> >>',
    '<< /Type /Annot /Subtype /Link /Rect [72 600 160 630] /A << /S /JavaScript /JS (app.alert\(1\)) >> >>',
    '<< /Type /Annot /Subtype /Link /Rect [72 560 160 590] /A << /S /Launch /F (never-run.exe) >> >>',
    '<< /Type /Annot /Subtype /Link /Rect [72 520 160 550] /Dest [999 /Fit] >>',
    '<< /Type /Annot /Subtype /Link /Rect [72 480 160 510] /Dest [6 0 R /Fit] /A << /S /URI /URI (https://example.invalid) >> >>',
    '<< /Type /Annot /Subtype /Link /Rect [72 440 160 470] /A << /S /URI /URI (file:///C:/never-run.exe) >> >>',
    '<< /Type /Annot /Subtype /Link /Rect [72 400 160 430] /A << /S /URI /URI (javascript:alert\(1\)) >> >>',
    '<< /Type /Annot /Subtype /Link /Rect [72 360 160 390] /A << /S /URI /URI (https:///missing-host) >> >>',
    '<< /Type /Annot /Subtype /Link /Rect [72 320 160 350] /A << /S /URI /URI (https://example.invalid/ bad) >> >>',
    '<< /Type /Annot /Subtype /Link /Rect [72 280 160 310] /A << /S /URI /URI <68747470733a2f2f6578616d706c652e696e76616c69642fff> >> >>',
    '<< /Type /Annot /Subtype /Link /Rect [72 240 160 270] /A << /S /URI /URI (mailto:never@example.invalid) >> >>'
)
Write-PdfFixture ([IO.Path]::Combine($output, 'links.pdf')) $linkObjects
$rotatedLinks = @($linkObjects)
foreach ($index in @(2, 5)) { $rotatedLinks[$index] = $rotatedLinks[$index].Replace('/MediaBox', '/Rotate 90 /CropBox [18 18 594 774] /MediaBox') }
Write-PdfFixture ([IO.Path]::Combine($output, 'rotated-links.pdf')) $rotatedLinks
$rotated = @($objects)
$rotated[2] = $rotated[2].Replace('/MediaBox', '/Rotate 90 /CropBox [18 18 594 774] /MediaBox')
$rotated[5] = $rotated[5].Replace('/MediaBox', '/Rotate 90 /CropBox [18 18 594 774] /MediaBox')
Write-PdfFixture ([IO.Path]::Combine($output, 'rotated-text.pdf')) $rotated
$objects[1] = '<< /Type /Pages /Kids [3 0 R 6 0 R 7 0 R 8 0 R] /Count 4 >>'
$objects[5] = $objects[2].Replace('[0 0 612 792]', '[0 0 792 400]')
$objects += $objects[2].Replace('[0 0 612 792]', '[0 0 612 200]')
$objects += $objects[2].Replace('[0 0 612 792]', '[0 0 400 1600]')
Write-PdfFixture ([IO.Path]::Combine($output, 'mixed-pages.pdf')) $objects
$outlineObjects = @($objects)
$outlineObjects[0] = '<< /Type /Catalog /Pages 2 0 R /Outlines 9 0 R /Dests << /inicio [3 0 R /Fit] >> >>'
$outlineTitle = 'Cap' + [char]0xED + 'tulo caf' + [char]0xE9
$titleHex = [BitConverter]::ToString([Text.Encoding]::BigEndianUnicode.GetBytes($outlineTitle)).Replace('-', '')
$outlineObjects += @(
    '<< /Type /Outlines /First 10 0 R /Last 14 0 R /Count 5 >>',
    "<< /Title <FEFF$titleHex> /Parent 9 0 R /Next 12 0 R /First 11 0 R /Last 11 0 R /Count 1 /Dest /inicio >>",
    '<< /Title (Subcapitulo) /Parent 10 0 R /A 15 0 R >>',
    '<< /Title (Web) /Parent 9 0 R /Prev 10 0 R /Next 13 0 R /A << /S /URI /URI (https://example.invalid) >> >>',
    '<< /Title (Destino invalido) /Parent 9 0 R /Prev 12 0 R /Next 14 0 R /Dest [999 /Fit] >>',
    '<< /Title (Script) /Parent 9 0 R /Prev 13 0 R /A << /S /JavaScript /JS (app.alert\(1\)) >> >>',
    '<< /S /GoTo /D [6 0 R /XYZ 150 200 null] >>'
)
Write-PdfFixture ([IO.Path]::Combine($output, 'outline.pdf')) $outlineObjects
$rotatedOutline = @($outlineObjects)
foreach ($index in @(2, 5, 6, 7)) { $rotatedOutline[$index] = $rotatedOutline[$index].Replace('/MediaBox', '/Rotate 90 /CropBox [18 18 380 380] /MediaBox') }
Write-PdfFixture ([IO.Path]::Combine($output, 'rotated-outline.pdf')) $rotatedOutline
foreach ($variant in @('null 200', '150 null')) {
    $nullableOutline = @($outlineObjects)
    $nullableOutline[14] = $nullableOutline[14].Replace('150 200', $variant)
    Write-PdfFixture ([IO.Path]::Combine($output, ('outline-' + $variant.Replace(' ', '-') + '.pdf'))) $nullableOutline
}
$cyclic = @($outlineObjects)
$cyclic[11] = $cyclic[11].Replace('/Next 13 0 R', '/Next 10 0 R')
Write-PdfFixture ([IO.Path]::Combine($output, 'outline-cycle.pdf')) $cyclic
$longTitle = @($outlineObjects)
$longTitle[9] = $longTitle[9].Replace("<FEFF$titleHex>", '(' + ('A' * 2000) + ')')
Write-PdfFixture ([IO.Path]::Combine($output, 'outline-long-title.pdf')) $longTitle
$deep = @($objects)
$deep[0] = '<< /Type /Catalog /Pages 2 0 R /Outlines 9 0 R >>'
$deep += '<< /Type /Outlines /First 10 0 R /Last 10 0 R /Count 80 >>'
for ($i = 0; $i -lt 80; $i++) {
    $parent = if ($i -eq 0) { 9 } else { 9 + $i }
    $child = if ($i -lt 79) { "/First $(11 + $i) 0 R /Last $(11 + $i) 0 R /Count 1" } else { '' }
    $deep += "<< /Title (Level $i) /Parent $parent 0 R $child /Dest [3 0 R /Fit] >>"
}
Write-PdfFixture ([IO.Path]::Combine($output, 'outline-deep.pdf')) $deep
$wide = @($objects)
$wide[0] = '<< /Type /Catalog /Pages 2 0 R /Outlines 9 0 R >>'
$wide += '<< /Type /Outlines /First 10 0 R /Last 4109 0 R /Count 4100 >>'
for ($i = 0; $i -lt 4100; $i++) {
    $next = if ($i -lt 4099) { "/Next $(11 + $i) 0 R" } else { '' }
    $wide += "<< /Title (Entry $i) /Parent 9 0 R $next /Dest [3 0 R /Fit] >>"
}
Write-PdfFixture ([IO.Path]::Combine($output, 'outline-wide.pdf')) $wide
$exe = [IO.Path]::Combine($root, 'build', 'pdf-smoke.exe')
& $exe $fixture
if ($LASTEXITCODE -ne 0) { throw 'Falló apertura/renderizado/búsqueda.' }
& $exe ([IO.Path]::Combine($output, 'missing.pdf'))
if ($LASTEXITCODE -ne 1) { throw 'Falló el manejo de archivo inexistente.' }
Write-Host 'Pruebas de apertura, renderizado, búsqueda y archivo inexistente: OK.'
Write-Host 'Contraseñas y permisos: scripts/test-reader.ps1 incluye fixtures cifrados sintéticos.'
