$ErrorActionPreference = "Continue"
$bfc = Join-Path $PSScriptRoot "..\build\Release\bfc.exe"
$lib = Join-Path $PSScriptRoot "..\..\libraries\stdlib"
$results = @()
Get-ChildItem -LiteralPath $PSScriptRoot -Filter *.bf | ForEach-Object {
    $name = $_.BaseName
    $exe = Join-Path $PSScriptRoot "$name.exe"
    & $bfc $_.FullName -o $exe -I $lib 2>&1 | Out-Null
    if (-not (Test-Path $exe)) {
        $results += "[FAIL] $name : compilation"
        return
    }
    $sw = [Diagnostics.Stopwatch]::StartNew()
    $output = & $exe 2>&1
    $sw.Stop()
    $results += ("{0,-18} {1,10:N2} ms  => {2}" -f $name, $sw.Elapsed.TotalMilliseconds, (($output | Out-String).Trim() -replace "`r`n"," | "))
}
$results | ForEach-Object { Write-Output $_ }
$results | Set-Content -LiteralPath (Join-Path $PSScriptRoot "results.txt") -Encoding UTF8
Write-Host "Benchmarks written to results.txt"