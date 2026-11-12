using System.Diagnostics;
using KillerPdf.Engine.Documents;
using KillerPdf.Engine.Rendering;

// Diagnostic only: use the already-installed engine, never modify the source PDF.
if (args.Length == 0) { Console.Error.WriteLine("Usage: KillerBenchmark file.pdf"); return 2; }
var clock = Stopwatch.StartNew();
using var stream = File.OpenRead(args[0]);
var openedFile = clock.Elapsed.TotalMilliseconds;
var document = PdfDocument.Open(stream);
var opened = clock.Elapsed.TotalMilliseconds;
var renderer = new PdfPageRenderer(document, null);
var image = renderer.Render(0, new PdfRenderOptions(800, 1000), default);
var rendered = clock.Elapsed.TotalMilliseconds;
int dark = 0;
foreach (var pixel in image.Pixels.Span) { if (pixel < 100) dark++; }
Console.WriteLine($"engine=KillerPDF file_ms={openedFile:F3} parse_ms={opened-openedFile:F3} render_ms={rendered-opened:F3} first_page_ms={rendered:F3} width={image.Width} height={image.Height} dark_bytes={dark}");
return 0;
