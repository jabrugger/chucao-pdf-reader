param([string]$Source)
$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Drawing
$root = [IO.Path]::GetFullPath([IO.Path]::Combine($PSScriptRoot, '..'))
$folder = [IO.Path]::Combine($root, 'assets', 'branding')
[IO.Directory]::CreateDirectory($folder) | Out-Null
$original = [IO.Path]::Combine($folder, 'chucao-original.jpg')
if ($Source) {
    $sourcePath = [IO.Path]::GetFullPath($Source)
    if ($sourcePath -ne $original) { [IO.File]::Copy($sourcePath, $original, $true) }
}
if (![IO.File]::Exists($original)) { throw 'Falta la imagen original del chucao.' }
$image = [Drawing.Image]::FromFile($original)
function Resize-Brand([int]$size) {
    $bitmap = [Drawing.Bitmap]::new($size, $size, [Drawing.Imaging.PixelFormat]::Format32bppArgb)
    $graphics = [Drawing.Graphics]::FromImage($bitmap)
    try {
        $graphics.Clear([Drawing.Color]::White)
        $graphics.InterpolationMode = [Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
        $graphics.PixelOffsetMode = [Drawing.Drawing2D.PixelOffsetMode]::HighQuality
        $graphics.DrawImage($image, [Drawing.Rectangle]::new(0, 0, $size, $size))
    } finally { $graphics.Dispose() }
    return $bitmap
}
try {
    $large = Resize-Brand 1024
    try { $large.Save([IO.Path]::Combine($folder, 'chucao.png'), [Drawing.Imaging.ImageFormat]::Png) } finally { $large.Dispose() }
    $welcomeBitmap = Resize-Brand 256
    try {
        $welcomeBitmap.Save([IO.Path]::Combine($folder, 'chucao-icon-256.png'), [Drawing.Imaging.ImageFormat]::Png)
        $welcomeBitmap.Save([IO.Path]::Combine($folder, 'chucao-home.bmp'), [Drawing.Imaging.ImageFormat]::Bmp)
    } finally { $welcomeBitmap.Dispose() }
    $sizes = @(16, 20, 24, 32, 40, 48, 64, 96, 128, 256)
    $frames = foreach ($size in $sizes) {
        $bitmap = Resize-Brand $size
        $stream = [IO.MemoryStream]::new()
        try {
            $bitmap.Save($stream, [Drawing.Imaging.ImageFormat]::Png)
            [PSCustomObject]@{size=$size; bytes=$stream.ToArray()}
        } finally { $stream.Dispose(); $bitmap.Dispose() }
    }
    $output = [IO.File]::Create([IO.Path]::Combine($folder, 'chucao.ico'))
    $writer = [IO.BinaryWriter]::new($output)
    try {
        $writer.Write([UInt16]0); $writer.Write([UInt16]1); $writer.Write([UInt16]$frames.Count)
        $offset = 6 + 16 * $frames.Count
        foreach ($frame in $frames) {
            $dimension = if ($frame.size -eq 256) { 0 } else { $frame.size }
            $writer.Write([byte]$dimension); $writer.Write([byte]$dimension)
            $writer.Write([byte]0); $writer.Write([byte]0)
            $writer.Write([UInt16]1); $writer.Write([UInt16]32)
            $writer.Write([UInt32]$frame.bytes.Length); $writer.Write([UInt32]$offset)
            $offset += $frame.bytes.Length
        }
        foreach ($frame in $frames) { $writer.Write([byte[]]$frame.bytes) }
    } finally { $writer.Dispose() }
} finally { $image.Dispose() }
Write-Host 'Identidad preparada: PNG, bitmap de bienvenida e ICO de 10 tamaños.'
