# Benchmark script for BasicForge
# Measures compile time, executable size, and runtime performance

$ErrorActionPreference = "Stop"

Write-Host "=== BasicForge Performance Benchmarks ===" -ForegroundColor Cyan

# Benchmark 1: Compile time
Write-Host "`n1. Compile Time Benchmark" -ForegroundColor Yellow
$sources = @(
    "examples\milestone3\01_hello_world.bf",
    "examples\milestone3\test_math.bf",
    "examples\milestone3\test_strings.bf",
    "examples\milestone3\test_generic.bf"
)

foreach ($src in $sources) {
    if (Test-Path $src) {
        $name = [System.IO.Path]::GetFileNameWithoutExtension($src)
        $sw = [System.Diagnostics.Stopwatch]::StartNew()
        & bfc.exe $src -o "bench_$name.exe" 2>&1 | Out-Null
        $sw.Stop()
        Write-Host "  $($name): $($sw.ElapsedMilliseconds)ms" -ForegroundColor Gray
    }
}

# Benchmark 2: Executable size
Write-Host "`n2. Executable Size Benchmark" -ForegroundColor Yellow
$exes = Get-ChildItem -Path . -Filter "bench_*.exe" -ErrorAction SilentlyContinue
foreach ($exe in $exes) {
    $size = $exe.Length / 1KB
    Write-Host "  $($exe.Name): $([math]::Round($size, 1))KB" -ForegroundColor Gray
}

# Benchmark 3: Runtime performance
Write-Host "`n3. Runtime Performance Benchmark" -ForegroundColor Yellow
if (Test-Path "bench_test_math.exe") {
    $sw = [System.Diagnostics.Stopwatch]::StartNew()
    & ".\bench_test_math.exe" | Out-Null
    $sw.Stop()
    Write-Host "  test_math: $($sw.ElapsedMilliseconds)ms" -ForegroundColor Gray
}

# Benchmark 4: Memory usage
Write-Host "`n4. Memory Usage Benchmark" -ForegroundColor Yellow
$proc = Start-Process -FilePath ".\bench_01_hello_world.exe" -PassThru -NoNewWindow
Start-Sleep -Milliseconds 100
if (-not $proc.HasExited) {
    $mem = $proc.WorkingSet64 / 1MB
    Write-Host "  Memory: $([math]::Round($mem, 1))MB" -ForegroundColor Gray
    Stop-Process -Id $proc.Id -Force -ErrorAction SilentlyContinue
}

# Cleanup
Write-Host "`nCleaning up benchmark files..." -ForegroundColor Gray
Remove-Item "bench_*.exe" -ErrorAction SilentlyContinue
Remove-Item "bench_*.obj" -ErrorAction SilentlyContinue
Remove-Item "bench_*.c" -ErrorAction SilentlyContinue

Write-Host "`n=== Benchmarks Complete ===" -ForegroundColor Green