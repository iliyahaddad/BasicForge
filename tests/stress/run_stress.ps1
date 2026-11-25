$ErrorActionPreference = "Continue"
$bfc = Join-Path $PSScriptRoot "..\..\compiler\build\Release\bfc.exe"
$lib = Join-Path $PSScriptRoot "..\..\libraries\stdlib"
$tests = @("stress_many_functions.bf", "stress_large_array.bf")
foreach ($t in $tests) {
    $src = Join-Path $PSScriptRoot $t
    $exe = Join-Path $PSScriptRoot ($t -replace '\.bf$','.exe')
    Write-Host "[stress] $t"
    & $bfc $src -o $exe -I $lib 2>&1 | Out-Null
    if (Test-Path $exe) {
        $job = Start-Job -ScriptBlock { param($p) & $p 2>&1 } -ArgumentList $exe
        if (Wait-Job $job -Timeout 60) {
            $o = Receive-Job $job
            Write-Host "  PASS => $o"
        } else {
            Stop-Job $job
            Write-Host "  TIMEOUT"
        }
        Remove-Job $job -Force
    } else {
        Write-Host "  COMPILE FAIL"
    }
}